#include "kvm_cpu.h"

#include <err.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stropts.h>
#include <sys/mman.h>

static int init_vcpu_mem(int kvm_fd, struct kvm_cpu *vcpu) {
  vcpu->mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
  if (vcpu->mmap_size == -1)
    return -1;
  vcpu->run = mmap(NULL, vcpu->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu->fd, 0);
  if (vcpu->run == MAP_FAILED)
    return -1;
  return 0;
}

static int init_vcpu_regs(int kvm_fd, struct kvm_cpu *vcpu) {
  ioctl(vcpu->fd, KVM_GET_SREGS, &(vcpu->sregs));
  ioctl(vcpu->fd, KVM_GET_REGS, &(vcpu->regs));
  vcpu->sregs.cs.base = 0;
  vcpu->sregs.cs.selector = 0;
  vcpu->regs.rip = 0x1000;
  vcpu->regs.rax = 2;
  vcpu->regs.rbx = 2;
  vcpu->regs.rflags = 0x2;
  ioctl(vcpu->fd, KVM_SET_SREGS, &(vcpu->sregs));
  ioctl(vcpu->fd, KVM_SET_REGS, &(vcpu->regs));
  return 0;
}

struct kvm_cpu* create_vcpu(int kvm_fd, int vm_fd) {
  struct kvm_cpu *vcpu = malloc(sizeof(struct kvm_cpu));
  if (!vcpu)
    return NULL;
  int fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
  if (fd == -1) {
    free_vcpu(vcpu);
    return NULL;
  }

  vcpu->fd = fd;

  if (init_vcpu_mem(kvm_fd, vcpu) == -1) {
    free_vcpu(vcpu);
    return NULL;
  }

  if (init_vcpu_regs(kvm_fd, vcpu) == -1) {
    free_vcpu(vcpu);
    return NULL;
  }
  return vcpu;
}

void free_vcpu(struct kvm_cpu *vcpu) {
  if (vcpu)
    free(vcpu);
}
