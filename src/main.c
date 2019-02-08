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

static void dump_io(struct kvm_cpu *vcpu) {
  printf("io.direction: %s\n", (vcpu->run->io.direction == KVM_EXIT_IO_IN ? "IN" : "OUT"));
  printf("io.size: %d\n", vcpu->run->io.size);
  printf("io.port: %04x\n", vcpu->run->io.port);
  printf("io.count: %d\n", vcpu->run->io.count);
  printf("io.data_offset: %llx\n", vcpu->run->io.data_offset);
}

int main(int argc, char *argv[])
{
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  check_args(opts, argv);
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

  //if (enable_kvm_debug(vcpu) < 0)
  //  errx(1, "could enable vcpu debug");

  while (1) {
    ioctl(vcpu->fd, KVM_RUN, NULL);
    switch (vcpu->run->exit_reason) {
      case KVM_EXIT_HLT:
        printf("KVM_EXIT_HLT\n");
        dump_vcpu_registers(vcpu);
        return 0;
      case KVM_EXIT_IO:
        if (vcpu->run->io.port == 0x3fd
            && vcpu->run->io.direction == KVM_EXIT_IO_IN) {
          // write 0x20 at data_offset
          uint8_t *ptr = ((uint8_t *) vcpu->run) + vcpu->run->io.data_offset;
          *ptr = 0x20;
        } else if (vcpu->run->io.port == 0x3f8
                   && vcpu->run->io.direction == KVM_EXIT_IO_OUT
                   && vcpu->run->io.size > 0) {
          // read character from mem and print it
          for (size_t i = 0; i < vcpu->run->io.size; ++i) {
            putchar(*(((char *) vcpu->run) + vcpu->run->io.data_offset + i));
          }
        } 
        ///else {
        ///  dump_io(vcpu);
        ///}
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
        return 1;
      default:
        printf("Unhandle exit reason: %d\n", vcpu->run->exit_reason);
    }
  }

  return 0;
}
