#include "kvm.h"
#include "options.h"

#include <asm/bootparam.h>
#include <error.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static ssize_t read_all(int fd, void *buf, size_t len) {
  ssize_t rd = 0;
  char *cbuf = buf;
  while (len > 0) {
    ssize_t r = read(fd, cbuf, len);
    if (r <= 0) {
      if (rd > 0)
        return rd;
      return -1;
    }
    rd += r;
    len -= r;
    cbuf += r;
  }
  return rd;
}

static void* guest_flat_to_host(struct kvm *kvm, uint64_t off) {
  return ((char *) kvm->mem) + off;
}

static int init_kvm_userspace_mem(struct kvm *kvm, size_t size) {
  void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1 , 0);
  if (mem == MAP_FAILED) {
    fprintf(stderr, "my-kvm: could allocate memory for kvm userspace.\n");
    return -1;
  }
  struct kvm_userspace_memory_region region = {
    .slot = 0,
    .guest_phys_addr = 0,
    .memory_size = size,
    .userspace_addr = (uint64_t) mem
  };
  if (ioctl(kvm->vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
    munmap(mem, size);
    return -1;
  }
  kvm->mem_size = size;
  kvm->mem = mem;

  return 0;
}

static void set_boot_params(struct boot_params *params) {
  params->hdr.type_of_loader = 0xff;
  params->hdr.loadflags |= LOADFLAG_CAN_USE_HEAP | LOADFLAG_KEEP_SEGMENT
    | LOADFLAG_LOADED_HIGH;
  params->hdr.heap_end_ptr = 0xfe00;
  params->hdr.cmd_line_ptr = BOOT_CMDLINE_OFFSET;
}

struct kvm* init_kvm_struct(int kvm_fd, int vm_fd, struct kvm_options *opts) {
  struct kvm *kvm = malloc(sizeof(struct kvm));
  if (!kvm)
    return NULL;
  kvm->kvm_fd = kvm_fd;
  kvm->vm_fd = vm_fd;
  kvm->initrd_file = opts->initrd;
  kvm->kernel_filename = opts->bz_im;

  int cmd_line_len = 0;
  for (int i = 0; opts->kernel[i] != NULL; ++i) {
    cmd_line_len += strlen(opts->kernel[i]);
    cmd_line_len++; // size for space between each word
  }
  char *cmd_line = calloc(cmd_line_len, sizeof(char));
  if (!cmd_line) {
    free_kvm_struct(kvm);
    return NULL;
  }

  for (int i = 0; opts->kernel[i] != NULL; ++i) {
    cmd_line = strcat(cmd_line, opts->kernel[i]);
    if (opts->kernel[i + 1] != NULL)
      cmd_line = strcat(cmd_line, " ");
  }

  kvm->kernel_cmd_line = cmd_line;

  if (init_kvm_userspace_mem(kvm, opts->ram_size) < 0) {
    free_kvm_struct(kvm);
    return NULL;
  }
  
  return kvm;
}

void free_kvm_struct(struct kvm *kvm) {
  if (kvm->kernel_cmd_line != NULL)
    free(kvm->kernel_cmd_line);
  if (kvm->mem && kvm->mem_size > 0)
    munmap(kvm->mem, kvm->mem_size);
  free(kvm);
}

static const char *bzimage_magic = "HdrS";

static int close_error(int fd) {
  close(fd);
  return -1;
}

static int load_initrd(struct kvm *kvm, struct boot_params *boot) {
  int fd = open(kvm->initrd_file, O_RDONLY);
  if (fd < 0)
    return -1;
  struct stat stat;
  if (fstat(fd, &stat) < 0)
    return close_error(fd);
  unsigned long addr = boot->hdr.initrd_addr_max & ~0xfffff;
  while (1) {
    if (addr < BZ_KERNEL_START) {
      fprintf(stderr, "./my-kvm: not enough memory to load initrd.\n");
      return close_error(fd);
    }
    else if (addr < (kvm->mem_size - stat.st_size))
      break;
    addr -= 0x100000;
  }
  char *ptr = guest_flat_to_host(kvm, addr);
  if (read_all(fd, ptr, stat.st_size) != stat.st_size)
    return close_error(fd);
  boot->hdr.ramdisk_image = addr;
  boot->hdr.ramdisk_size = stat.st_size;
  return 0;
}

static int add_available_memory_region(struct kvm *kvm, struct boot_params *boot) {
  struct boot_e820_entry entry;
  struct boot_e820_entry *entry_table = boot->e820_table;

  entry.addr = 0x0;
  entry.size = BZ_KERNEL_START - 1;
  entry.type = 1;

  entry_table[0] = entry;

  entry.addr = BZ_KERNEL_START;
  entry.size = kvm->mem_size - BZ_KERNEL_START;
  entry.type = 1;

  entry_table[1] = entry;
  boot->e820_entries = 2;

  return 0;
}

int load_bzimage(struct kvm *kvm) {
  // load and check the bzImage
  struct boot_params boot;
  int bz_fd = open(kvm->kernel_filename, O_RDONLY);
  if (bz_fd < 0)
    return -1;
  if (read_all(bz_fd, &boot, sizeof(boot)) != sizeof(boot))
    return close_error(bz_fd);
  if (memcmp(&boot.hdr.header, bzimage_magic, strlen(bzimage_magic)))
    return close_error(bz_fd);
  if (lseek(bz_fd, 0, SEEK_SET) < 0)
    return close_error(bz_fd);
  if (boot.hdr.setup_sects == 0)
    boot.hdr.setup_sects = 4;

  size_t file_size = (boot.hdr.setup_sects + 1) * 512;

  if (lseek(bz_fd, file_size, SEEK_SET) < 0)
    return close_error(bz_fd);

  // then read vmlinux
  char *ptr = guest_flat_to_host(kvm, BZ_KERNEL_START);
  ssize_t vmlinux_size = read_all(bz_fd, ptr, kvm->mem_size - BZ_KERNEL_START);
  if (vmlinux_size < 0)
    return close_error(bz_fd);

  // copy the kernel cmd line
  ptr = guest_flat_to_host(kvm, BOOT_CMDLINE_OFFSET);
  if (kvm->kernel_cmd_line) {
    size_t cmd_line_size = strlen(kvm->kernel_cmd_line) + 1;
    if (cmd_line_size > boot.hdr.cmdline_size)
      cmd_line_size = boot.hdr.cmdline_size;
    memset(ptr, 0, boot.hdr.cmdline_size);
    memcpy(ptr, kvm->kernel_cmd_line, cmd_line_size - 1);
  }
  struct boot_params *guest_boot = guest_flat_to_host(kvm, BOOT_PARAMS_OFFSET);
  guest_boot->hdr = boot.hdr;
  set_boot_params(guest_boot);

  if (kvm->initrd_file && load_initrd(kvm, guest_boot) < 0)
    return close_error(bz_fd);

  if (add_available_memory_region(kvm, guest_boot) < 0)
    return close_error(bz_fd);
  close(bz_fd);
  return 0;
}
