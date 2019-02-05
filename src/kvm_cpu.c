#include "kvm_cpu.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stropts.h>
#include <sys/mman.h>

static void set_sreg(struct kvm_segment *seg, uint64_t base, uint32_t limit,
    uint8_t g, uint8_t pr, uint8_t db, uint16_t sel, uint8_t stype) {
  seg->base = base;
  seg->limit = limit;
  seg->g = g;
  seg->present = pr;
  seg->db = db;
  seg->selector = sel;
  seg->type = stype;
}

static void set_sreg_base(struct kvm_segment *seg) {
  seg->base = 0;
  seg->limit = 0xffffffff;
  seg->g = 1;
  seg->present = 1;
}

static int init_vcpu_mem(int kvm_fd, struct kvm_cpu *vcpu) {
  vcpu->mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
  if (vcpu->mmap_size == -1)
    return -1;
  vcpu->run = mmap(NULL, vcpu->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu->fd, 0);
  if (vcpu->run == MAP_FAILED)
    return -1;
  return 0;
}

static void init_vcpu_regs(struct kvm_regs *regs) {
  regs->rflags = 0x2;
  regs->rip = BZ_KERNEL_START;
  regs->rsp = 0;
  regs->rbp = 0;
  regs->rdi = 0;
  regs->rbx = 0;
  regs->rsi = 0x10000;
}

static void init_vcpu_sregs(struct kvm_sregs *sregs) {
  set_sreg(&(sregs->cs), 0, 0xffffffff, 1, 1, 1, 0x10, SEG_EXECUTE_READ);
  set_sreg(&(sregs->ss), 0, 0xffffffff, 1, 1, 1, 0x18, SEG_READ_WRITE);
  set_sreg(&(sregs->ds), 0, 0xffffffff, 1, 1, 0, 0x18, SEG_READ_WRITE);
  set_sreg(&(sregs->es), 0, 0xffffffff, 1, 1, 0, 0x18, SEG_READ_WRITE);
  set_sreg_base(&(sregs->fs));
  set_sreg_base(&(sregs->gs));
  sregs->cr0 = 0;
}

static int init_vcpu_all_regs(struct kvm *kvm, struct kvm_cpu *vcpu) {
  ioctl(vcpu->fd, KVM_GET_SREGS, &(vcpu->sregs));
  ioctl(vcpu->fd, KVM_GET_REGS, &(vcpu->regs));
  init_vcpu_regs(&(vcpu->regs));
  init_vcpu_sregs(&(vcpu->sregs));
  ioctl(vcpu->fd, KVM_SET_SREGS, &(vcpu->sregs));
  ioctl(vcpu->fd, KVM_SET_REGS, &(vcpu->regs));
  return 0;
}

struct kvm_cpu* create_vcpu(struct kvm *kvm) {
  struct kvm_cpu *vcpu = calloc(1, sizeof(struct kvm_cpu));
  if (!vcpu)
    return NULL;
  int fd = ioctl(kvm->vm_fd, KVM_CREATE_VCPU, 0);
  if (fd == -1) {
    free_vcpu(vcpu);
    return NULL;
  }

  vcpu->fd = fd;

  if (init_vcpu_mem(kvm->kvm_fd, vcpu) == -1) {
    free_vcpu(vcpu);
    return NULL;
  }

  if (init_vcpu_all_regs(kvm, vcpu) == -1) {
    free_vcpu(vcpu);
    return NULL;
  }
  return vcpu;
}

void free_vcpu(struct kvm_cpu *vcpu) {
  if (vcpu) {
    if (vcpu->run)
      munmap(vcpu->run, vcpu->mmap_size);
    free(vcpu);
  }
}

void dump_vcpu_registers(struct kvm_cpu *vcpu) {
  unsigned long cr0, cr2, cr3, cr4, cr8,
                rax, rbx, rcx, rdx, rsi,
                rdi, rbp, r8, r9, r10, r11,
                r12, r13, r14, r15, rip, rsp,
                rflags;
  struct kvm_regs *regs = &(vcpu->regs);
  struct kvm_sregs *sregs = &(vcpu->sregs);
  
  rflags = regs->rflags;
  rip = regs->rip; rsp = regs->rsp;
  rax = regs->rax; rbx = regs->rbx; rcx = regs->rcx;
  rdx = regs->rdx; rsi = regs->rsi; rdi = regs->rdi;
  rbp = regs->rbp; r8 = regs->r8; r9 = regs->r9;
  r10 = regs->r10; r11 = regs->r11; r12 = regs->r12;
  r13 = regs->r13; r14 = regs->r14; r15 = regs->r15;

  cr0 = sregs->cr0; cr2 = sregs->cr2; cr3 = sregs->cr3;
  cr4 = sregs->cr4; cr8 = sregs->cr8;

  printf("\n Registers:\n");
  printf(  " ----------\n");
  printf(" rip: %016lx  rsp: %016lx flags: %016lx\n", rip, rsp, rflags);
  printf(" rax: %016lx  rbx: %016lx   rcx: %016lx\n", rax, rbx, rcx);
  printf(" rdx: %016lx  rsi: %016lx   rdi: %016lx\n", rdx, rsi, rdi);
  printf(" rbp: %016lx   r8: %016lx    r9: %016lx\n", rbp, r8, r9);
  printf(" r10: %016lx  r11: %016lx   r12: %016lx\n", r10, r11, r12);
  printf(" r13: %016lx  r14: %016lx   r15: %016lx\n", r13, r14, r15);

  printf("\n Segment registers:\n");
  printf(  " ------------------\n");
  printf(" cr0: %016lx  cr2: %016lx   cr3: %016lx\n", cr0, cr2, cr3);
  printf(" cr4: %016lx  cr8: %016lx\n", cr4, cr8);
}
