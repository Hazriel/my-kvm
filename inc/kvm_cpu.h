#ifndef KVM_CPU_H
#define KVM_CPU_H

#include <linux/kvm.h>

struct kvm_cpu {
  int fd;
  struct kvm_regs regs;
  struct kvm_sregs sregs;
  int mmap_size;
  struct kvm_run *run;
};

struct kvm_cpu* create_vcpu(int kvm_fd, int vm_fd);
void free_vcpu(struct kvm_cpu *vcpu);

#endif /* ifndef KVM_CPU_H */
