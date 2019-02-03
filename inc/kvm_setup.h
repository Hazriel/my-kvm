#ifndef KVM_SETUP_H
#define KVM_SETUP_H

#define _GNU_SOURCE

#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/kvm.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

int open_kvm_dev();
int create_vm(int kvm_fd);
int create_vcpu(int vm_fd);
void* init_useless_code();

#endif /* ifndef KVM_SETUP_H */
