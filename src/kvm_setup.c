#include "kvm_setup.h"

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/kvm.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

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
