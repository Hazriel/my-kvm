#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <sys/mman.h>

#include "kvm_setup.h"
#include "options.h"

int main(int argc, char *argv[])
{
  kvm_options_t *opts = parse_kvm_options(argc, argv);
  check_args(opts, argv);
  printf("bzImage is at %s\n", opts->bz_im);
  printf("initrd is at %s\n", opts->initrd);
  printf("ram is at %s\n", opts->ram);
  if (opts->kernel != NULL) {
    for (int i = 0; opts->kernel[i] != NULL; ++i)
      printf("additional arg: %s\n", opts->kernel[i]);
  }
  printf("====================================");

  int kvm_fd = open_kvm_dev();
  int vm_fd = create_vm(kvm_fd);
  void *code_mem = init_useless_code();

  struct kvm_userspace_memory_region region = {
    .slot = 0,
    .guest_phys_addr = 0x1000,
    .memory_size = 0x1000,
    .userspace_addr = (uint64_t) code_mem
  };

  ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);

  int vcpu_fd = create_vcpu(vm_fd);
  int mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
  if (mmap_size == -1)
    err(1, "couldn't fetch the mmap size for vm.");
  struct kvm_run *run = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd, 0);

  struct kvm_sregs sregs;
  ioctl(vcpu_fd, KVM_GET_SREGS, &sregs);
  sregs.cs.base = 0;
  sregs.cs.selector = 0;
  ioctl(vcpu_fd, KVM_SET_SREGS, &sregs);

  struct kvm_regs regs = {
    .rip = 0x1000,
    .rax = 2,
    .rbx = 2,
    .rflags = 0x2
  };
  ioctl(vcpu_fd, KVM_SET_REGS, &regs);

  while (1) {
    ioctl(vcpu_fd, KVM_RUN, NULL);
    switch (run->exit_reason) {
      case KVM_EXIT_HLT:
        printf("KVM_EXIT_HLT\n");
        return 0;
      case KVM_EXIT_IO:
        if (run->io.direction == KVM_EXIT_IO_OUT &&
            run->io.size == 1 &&
            run->io.port == 0x3f8 &&
            run->io.count == 1)
          putchar(*(((char *)run) + run->io.data_offset));
        else
          errx(1, "unhandled KVM_EXIT_IO");
        break;
      case KVM_EXIT_FAIL_ENTRY:
        errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
            (unsigned long long)run->fail_entry.hardware_entry_failure_reason);
      case KVM_EXIT_INTERNAL_ERROR:
        errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x",
            run->internal.suberror);
    }
  }

  return 0;
}
