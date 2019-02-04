#ifndef KVM_SETUP_H
#define KVM_SETUP_H

#define _GNU_SOURCE

int open_kvm_dev();
int create_vm(int kvm_fd);
void init_kvm(int kvm_fd, int vm_fd);

#endif /* ifndef KVM_SETUP_H */
