#ifndef KVM_SETUP_H
#define KVM_SETUP_H

#define _GNU_SOURCE

#include "options.h"

int open_kvm_dev();
int create_vm(int kvm_fd);

#endif /* ifndef KVM_SETUP_H */
