#ifndef KVM_H
#define KVM_H

#define LOADFLAG_LOADED_HIGH    0x1
#define LOADFLAG_KASLR_FLAG     0x1 << 1
#define LOADFLAG_QUIET_FLAG     0x1 << 5
#define LOADFLAG_KEEP_SEGMENT   0x1 << 6
#define LOADFLAG_CAN_USE_HEAP   0x1 << 7

#define BOOT_LOADER_SELECTOR    0x1000
#define BOOT_CMDLINE_OFFSET     0x20000
#define BZ_KERNEL_START         0x100000UL

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

int load_bzimage(struct kvm *kvm);
struct kvm* init_kvm_struct(int kvm_fd, int vm_fd, struct kvm_options *opts);
void free_kvm_struct(struct kvm *kvm);
void* guest_real_to_host(struct kvm *kvm, uint16_t sel, uint16_t off);

#endif /* ifndef KVM_H */
