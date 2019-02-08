#ifndef IO_H
#define IO_H

#include <linux/kvm.h>

struct uart_registers {
  unsigned char thr;
  unsigned char rbr;
  unsigned char ddl;
  unsigned char ier;
  unsigned char dlh;
  unsigned char fcr;
  unsigned char lcr;
  unsigned char mcr;
  unsigned char lsr;
  unsigned char msr;
  unsigned char sr;
};

void handle_kvm_io(struct kvm_run *run, struct uart_registers *uart);

#endif /* ifndef IO_H */
