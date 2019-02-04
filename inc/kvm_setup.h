#ifndef KVM_SETUP_H
#define KVM_SETUP_H

#define _GNU_SOURCE

int open_kvm_dev();
int create_vm(int kvm_fd);
int create_vcpu(int vm_fd);
void* init_useless_code();

#endif /* ifndef KVM_SETUP_H */
