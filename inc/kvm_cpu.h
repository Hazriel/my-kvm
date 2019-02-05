#ifndef KVM_CPU_H
#define KVM_CPU_H

#include "kvm.h"

#define SEG_EXECUTE_READ    0xA
#define SEG_READ_WRITE      0x2

#include <linux/kvm.h>

struct kvm_cpu {
  int fd;
  struct kvm_regs regs;
  struct kvm_sregs sregs;
  int mmap_size;
  struct kvm_run *run;
};

struct kvm_cpu* create_vcpu(struct kvm *kvm);
void free_vcpu(struct kvm_cpu *vcpu);
void dump_vcpu_registers(struct kvm_cpu *vcpu);

#endif /* ifndef KVM_CPU_H */
