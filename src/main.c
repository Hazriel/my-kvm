#include "kvm.h"
#include "kvm_cpu.h"
#include "kvm_setup.h"
#include "options.h"

#include <err.h>
#include <inttypes.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  check_args(opts, argv);

  int kvm_fd = open_kvm_dev();
  int vm_fd = create_vm(kvm_fd);
  init_kvm(kvm_fd, vm_fd);
  struct kvm *kvm = init_kvm_struct(kvm_fd, vm_fd, opts);
  struct kvm_cpu *vcpu = create_vcpu(kvm_fd, vm_fd);

  while (1) {
    ioctl(vcpu->fd, KVM_RUN, NULL);
    switch (vcpu->run->exit_reason) {
      case KVM_EXIT_HLT:
        printf("KVM_EXIT_HLT\n");
        return 0;
      case KVM_EXIT_IO:
        if (vcpu->run->io.size > 0) {
          printf("%s", ((char *)vcpu->run) + vcpu->run->io.data_offset);
        }
        else
          errx(1, "unhandled KVM_EXIT_IO");
        break;
      case KVM_EXIT_FAIL_ENTRY:
        errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
            (unsigned long long)vcpu->run->fail_entry.hardware_entry_failure_reason);
      case KVM_EXIT_INTERNAL_ERROR:
        errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x",
            vcpu->run->internal.suberror);
    }
  }

  return 0;
}
