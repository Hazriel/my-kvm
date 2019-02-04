#include "kvm_setup.h"

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/kvm.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static void* init_useless_code()
{
  const uint8_t code[] = {
    0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */
    0x00, 0xd8,       /* add %bl, %al */
    0x04, '0',        /* add $'0', %al */
    0xee,             /* out %al, (%dx) */
    0xb0, '\n',       /* mov $'\n', %al */
    0xee,             /* out %al, (%dx) */
    0xf4,             /* hlt */
  };

  void *code_mem = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (code_mem == NULL)
    err(1, "couldn't allocate memory for code.");

  memcpy(code_mem, code, sizeof(code));

  return code_mem;
}

void init_kvm(int kvm_fd, int vm_fd) {
  void *code = init_useless_code();
  struct kvm_userspace_memory_region region = {
    .slot = 0,
    .guest_phys_addr = 0x1000,
    .memory_size = 0x1000,
    .userspace_addr = (uint64_t) code
  };
  ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
}

int open_kvm_dev()
{
  int fd = open("/dev/kvm", O_RDWR);
  if (fd < 0)
    err(1, "unable to open /dev/kvm");
  return fd;
}

int create_vm(int kvm_fd)
{
  int fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
  if (fd < 0)
    errx(1, "unable to create vm");
  if (ioctl(fd, KVM_SET_TSS_ADDR, 0xfffbd000) == -1)
    errx(1, "unable to set tss addr");
  return fd;
}
