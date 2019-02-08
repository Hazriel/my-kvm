#include "io.h"

#include <stdio.h>
#include <inttypes.h>

static void dump_io(struct kvm_run *run) {
  printf("io.direction: %s\n", (run->io.direction == KVM_EXIT_IO_IN ? "IN" : "OUT"));
  printf("io.port: %04x\n", run->io.port);
}

static void handle_io_port_3f8 (struct kvm_run *run) {
  for (size_t i = 0; i < run->io.size; ++i)
    putchar(*(((char *) run) + run->io.data_offset + i));
}

static void handle_io_port_3fd (struct kvm_run *run) {
  uint8_t *ptr = ((uint8_t *) run) + run->io.data_offset;
  *ptr = 0x20;
}

void handle_kvm_io(struct kvm_run *run, struct uart_registers *uart) {
  if (run->io.direction == KVM_EXIT_IO_OUT) {
    if (run->io.port == 0x3f8 && run->io.size > 0)
      handle_io_port_3f8(run);
    else if (run->io.port == 0x3f9 && run->io.size > 0)
      uart->ier = *(((char *) run) + run->io.data_offset);
  } else if (run->io.direction == KVM_EXIT_IO_IN) {
    if (run->io.port == 0x3fd) 
      handle_io_port_3fd(run);
    else if (run->io.port == 0x3f9){
      uint8_t *ptr = ((uint8_t *) run) + run->io.data_offset;
      *ptr = uart->ier;
    }
  }
}
