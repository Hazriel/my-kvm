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
  if (ioctl(fd, KVM_SET_TSS_ADDR, 0xffffd000) < 0)
    errx(1, "unable to set tss addr");
  uint64_t map_addr = 0xffffc000;
  if (ioctl(fd, KVM_SET_IDENTITY_MAP_ADDR, &map_addr) < 0)
    errx(1, "unable to set identity map addr");
  if (ioctl(fd, KVM_CREATE_IRQCHIP) < 0)
    errx(1, "unable to create irq chip");

  struct kvm_pit_config pit = {
    .flags = KVM_PIT_SPEAKER_DUMMY
  };
  if (ioctl(fd, KVM_CREATE_PIT2, &pit) < 0)
    errx(1, "unable to create pit config");
  return fd;
}

int enable_kvm_debug(struct kvm_cpu *cpu) {
  struct kvm_guest_debug debug;
  debug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP;
  return ioctl(cpu->fd, KVM_SET_GUEST_DEBUG, &debug);
}
