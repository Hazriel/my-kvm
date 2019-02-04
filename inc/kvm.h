#ifndef KVM_H
#define KVM_H

struct kvm {
  int kvm_fd;
  int vm_fd;
  int cpu_cnt;
  int *cpus;
  u64 ram_size;
  const char *kernel_filename;
  const char *kernel_cmd_line;
  const char *initrd_file;
  const char *rootfs;
  const char *console;
}

#endif /* ifndef KVM_H */
