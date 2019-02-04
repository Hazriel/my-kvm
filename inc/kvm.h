#ifndef KVM_H
#define KVM_H

struct kvm {
  int kvm_fd;
  int vm_fd;
  int cpu_cnt;
  int *vcpus;
  u64 ram_size;
  const char *initrd_file;
  const char *kernel_filename;
  const char *kernel_cmd_line;
}

struct *kvm init_kvm(void);

#endif /* ifndef KVM_H */
