#include "kvm_cpu.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stropts.h>
#include <sys/mman.h>

#define set_segment_selector(Seg, Base, Limit, G, P) \
  do { \
    Seg.base = Base; \
    Seg.limit = Limit; \
    Seg.g = G; \
    Seg.present = P; \
  } while (0)

static int init_vcpu_mem(int kvm_fd, struct kvm_cpu *vcpu) {
  vcpu->mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
  if (vcpu->mmap_size == -1)
    return -1;
  vcpu->run = mmap(NULL, vcpu->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu->fd, 0);
  if (vcpu->run == MAP_FAILED)
    return -1;
  return 0;
}

static int set_cpuid(struct kvm_cpu *vcpu) {
  struct {
    struct kvm_cpuid cpuid;
    struct kvm_cpuid_entry entries[4];
  } cpuid_info;
  cpuid_info.cpuid.nent = 4;
  cpuid_info.cpuid.entries[0].function = 0;
  cpuid_info.cpuid.entries[0].eax = 1;
  cpuid_info.cpuid.entries[0].ebx = 0;
  cpuid_info.cpuid.entries[0].ecx = 0;
  cpuid_info.cpuid.entries[0].edx = 0;
  cpuid_info.cpuid.entries[1].function = 1;
  cpuid_info.cpuid.entries[1].eax = 0x400;
  cpuid_info.cpuid.entries[1].ebx = 0;
  cpuid_info.cpuid.entries[1].ecx = 0;
  cpuid_info.cpuid.entries[1].edx = 0x701b179;
  cpuid_info.cpuid.entries[2].function = 0x80000000;
  cpuid_info.cpuid.entries[2].eax = 0x80000001;
  cpuid_info.cpuid.entries[2].ebx = 0;
  cpuid_info.cpuid.entries[2].ecx = 0;
  cpuid_info.cpuid.entries[2].edx = 0;
  cpuid_info.cpuid.entries[3].function = 0x80000001;
  cpuid_info.cpuid.entries[3].eax = 0;
  cpuid_info.cpuid.entries[3].ebx = 0;
  cpuid_info.cpuid.entries[3].ecx = 0;
  cpuid_info.cpuid.entries[3].edx = 0x20100800;

  return ioctl(vcpu->fd, KVM_SET_CPUID2, &cpuid_info.cpuid);
}

static void init_vcpu_regs(struct kvm_regs *regs) {
  regs->rflags = 0x2;
  regs->rip = BZ_KERNEL_START;
  regs->rbp = 0;
  regs->rdi = 0;
  regs->rbx = 0;
  regs->rsi = BOOT_PARAMS_OFFSET;
}

static void init_vcpu_sregs(struct kvm_sregs *sregs) {
  set_segment_selector(sregs->cs, 0, ~0, 1, 1);
  set_segment_selector(sregs->ss, 0, ~0, 1, 1);
  set_segment_selector(sregs->ds, 0, ~0, 1, 1);
  set_segment_selector(sregs->es, 0, ~0, 1, 1);
  set_segment_selector(sregs->fs, 0, ~0, 1, 1);
  set_segment_selector(sregs->gs, 0, ~0, 1, 1);

  sregs->cs.db = 1;
  sregs->cs.selector = 0x10;
  sregs->cs.type = SEG_EXECUTE_READ;

  sregs->ss.db = 1;
  sregs->ss.selector = 0x18;
  sregs->ss.type = SEG_READ_WRITE;

  sregs->ds.selector = 0x18;
  sregs->ds.type = SEG_READ_WRITE;

  sregs->es.selector = 0x18;
  sregs->es.type = SEG_READ_WRITE;

  sregs->cr0 = 0x1;
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

  if (init_vcpu_mem(kvm->kvm_fd, vcpu) < 0) {
    free_vcpu(vcpu);
    return NULL;
  }

  if (set_cpuid(vcpu) < 0) {
    free_vcpu(vcpu);
    return NULL;
  }

  if (init_vcpu_all_regs(kvm, vcpu) < 0) {
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

static void dump_segment(struct kvm_segment *seg, const char *name) {
  printf(" %8s    %04hx   %016lx   %08x  %02hhx  %x   %x   %x  %x  %x  %x  %x\n",
      name, (uint16_t) seg->selector, (uint64_t) seg->base, (uint32_t) seg->limit,
      (uint8_t) seg->type, seg->present, seg->dpl, seg->db, seg->s, seg->l, seg->g,
      seg->avl);
}

void dump_vcpu_registers(struct kvm_cpu *vcpu) {
  unsigned long cr0, cr2, cr3, cr4, cr8,
                rax, rbx, rcx, rdx, rsi,
                rdi, rbp, r8, r9, r10, r11,
                r12, r13, r14, r15, rip, rsp,
                rflags;
  update_regs(vcpu);
  struct kvm_regs *regs = &vcpu->regs;
  struct kvm_sregs *sregs = &vcpu->sregs;
  
  rflags = regs->rflags;
  rip = regs->rip; rsp = regs->rsp;
  rax = regs->rax; rbx = regs->rbx; rcx = regs->rcx;
  rdx = regs->rdx; rsi = regs->rsi; rdi = regs->rdi;
  rbp = regs->rbp; r8 = regs->r8; r9 = regs->r9;
  r10 = regs->r10; r11 = regs->r11; r12 = regs->r12;
  r13 = regs->r13; r14 = regs->r14; r15 = regs->r15;

  cr0 = sregs->cr0; cr2 = sregs->cr2; cr3 = sregs->cr3;
  cr4 = sregs->cr4; cr8 = sregs->cr8;

  printf("\n =========================================================== \n");
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

  printf("\n Segment registers:\n");
  printf(  " ------------------\n");
  printf(" %s %s %11s %14s %6s %s %s %s  %s  %s  %s %s\n", "register", "selector",
      "base", "limit", "type", "pr", "dpl", "db", "s", "l", "g", "avl");
  dump_segment(&sregs->cs, "cs");
  dump_segment(&sregs->ss, "ss");
  dump_segment(&sregs->ds, "ds");
  dump_segment(&sregs->es, "es");
  dump_segment(&sregs->fs, "fs");
  dump_segment(&sregs->gs, "gs");
  dump_segment(&sregs->tr, "tr");
  dump_segment(&sregs->ldt, "ldt");
}

void update_regs(struct kvm_cpu *vcpu) {
  ioctl(vcpu->fd, KVM_GET_REGS, &vcpu->regs);
  ioctl(vcpu->fd, KVM_GET_SREGS, &vcpu->sregs);
}
