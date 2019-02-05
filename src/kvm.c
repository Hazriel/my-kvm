#include "kvm.h"
#include "options.h"

#include <asm/bootparam.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static ssize_t read_all(int fd, void *buf, size_t len) {
  ssize_t rd = 0;
  char *cbuf = buf;
  while (len > 0) {
    ssize_t r = read(fd, cbuf, len);
    if (r == -1)
      return -1;
    rd += r;
    len -= r;
    cbuf += r;
  }
  return rd;
}

static void copy_sample_code(struct kvm *kvm) {
  const uint8_t code[] = {
    0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */
    0x00, 0xd8,       /* add %bl, %al */
    0x04, '0',        /* add $'0', %al */
    0xee,             /* out %al, (%dx) */
    0xb0, '\n',       /* mov $'\n', %al */
    0xee,             /* out %al, (%dx) */
    0xf4,             /* hlt */
  };

  memcpy(kvm->mem, code, sizeof(code));
}

static int init_kvm_userspace_mem(struct kvm *kvm, size_t size) {
  void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS, -1 , 0);
  if (mem == MAP_FAILED)
    return -1;
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
  // TODO: Remove this
  copy_sample_code(kvm);
  return 0;
}

static void set_boot_params(struct kvm *kvm, struct boot_params *params) {

}

struct kvm* init_kvm_struct(int kvm_fd, int vm_fd, struct kvm_options *opts) {
  struct kvm *kvm = malloc(sizeof(struct kvm));
  if (!kvm)
    return NULL;
  kvm->kvm_fd = kvm_fd;
  kvm->vm_fd = vm_fd;
  kvm->initrd_file = opts->initrd;
  kvm->kernel_filename = opts->bz_im;

  // TODO: append 'earlyprintk=serial' to cmdline
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

  // allocate memory region for vm
  if (init_kvm_userspace_mem(kvm, 1 << 30) < 0) {
    free_kvm_struct(kvm);
    return NULL;
  }
  
  return kvm;
}

void free_kvm_struct(struct kvm *kvm) {
  if (kvm->kernel_cmd_line != NULL)
    free(kvm->kernel_cmd_line);
  free(kvm);
}

static const char *bzimage_magic = "HdrS";

int load_bzimage(struct kvm *kvm) {
  // load and check the bzImage
  struct boot_params boot;
  int bz_fd = open(kvm->kernel_filename, O_RDONLY);
  if (bz_fd < 0)
    return -1;
  if (read_all(bz_fd, &boot, sizeof(boot)) != sizeof(boot))
    return -1;
  if (memcmp(&boot.hdr.header, bzimage_magic, strlen(bzimage_magic)))
    return -1;
  if (lseek(bz_fd, 0, SEEK_SET) < 0)
    return -1;
  if (boot.hdr.setup_sects == 0)
    boot.hdr.setup_sects = 4;

  size_t file_size = (boot.hdr.setup_sects + 1) * 512;
  return 0;
}
