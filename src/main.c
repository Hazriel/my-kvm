#include "io.h"
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
  if (!opts)
    return 1;
  if (check_args(opts, argv) < 0) {
    free_kvm_options(opts);
    return 1;
  }
  int kvm_fd = open_kvm_dev();
  int vm_fd = create_vm(kvm_fd);

  struct kvm *kvm = init_kvm_struct(kvm_fd, vm_fd, opts);

  if (kvm == NULL)
    errx(1, "couldn't initialize kvm struct");

  if (load_bzimage(kvm) < 0)
    errx(1, "error while initializing bzImage and initrd");

  struct kvm_cpu *vcpu = create_vcpu(kvm);
  if (!vcpu)
    errx(1, "error while initializing vcpu");

  struct uart_registers uart;

  while (1) {
    ioctl(vcpu->fd, KVM_RUN, NULL);
    switch (vcpu->run->exit_reason) {
      case KVM_EXIT_HLT:
        printf("KVM_EXIT_HLT\n");
        dump_vcpu_registers(vcpu);
        free_kvm_struct(kvm);
        return 0;
      case KVM_EXIT_IO:
        handle_kvm_io(vcpu->run, &uart);
        break;
      case KVM_EXIT_FAIL_ENTRY:
        errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
            (unsigned long long)vcpu->run->fail_entry.hardware_entry_failure_reason);
      case KVM_EXIT_INTERNAL_ERROR:
        errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x",
            vcpu->run->internal.suberror);
      case KVM_EXIT_DEBUG:
        dump_vcpu_registers(vcpu);
        break;
      case KVM_EXIT_SHUTDOWN:
        dump_vcpu_registers(vcpu);
        printf("KVM_EXIT_SHUTDOWN\n");
        free_kvm_struct(kvm);
        return 1;
      default:
        printf("Unhandle exit reason: %d\n", vcpu->run->exit_reason);
    }
  }

  return 0;
}
