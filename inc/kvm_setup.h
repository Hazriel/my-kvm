#ifndef KVM_SETUP_H
#define KVM_SETUP_H

#define _GNU_SOURCE

#include "kvm_cpu.h"
#include "options.h"

int open_kvm_dev();
int create_vm(int kvm_fd);
int enable_kvm_debug(struct kvm_cpu *cpu);

#endif /* ifndef KVM_SETUP_H */
