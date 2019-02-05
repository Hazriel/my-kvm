#ifndef KVM_H
#define KVM_H

#include "options.h"

#include <inttypes.h>
#include <stdlib.h>

struct kvm {
  int kvm_fd;
  int vm_fd;
  int cpu_cnt;
  int *vcpus;

  size_t mem_size;
  void *mem;

  uint64_t ram_size;
  char *initrd_file;
  char *kernel_filename;
  char *kernel_cmd_line;
};

struct kvm* init_kvm_struct(int kvm_fd, int vm_fd, struct kvm_options *opts);
void free_kvm_struct(struct kvm *kvm);
int load_bzimage(struct kvm *kvm);

#endif /* ifndef KVM_H */
