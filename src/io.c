#include "io.h"

#include <stdio.h>
#include <inttypes.h>

static void handle_io_port_3f8(struct kvm_run *run) {
  if (run->io.direction == KVM_EXIT_IO_OUT) {
    for (size_t i = 0; i < run->io.size; ++i)
      putchar(*(((char *) run) + run->io.data_offset + i));
  }
}

static void handle_io_port_3fd(struct kvm_run *run) {
  if (run->io.direction == KVM_EXIT_IO_IN) {
    uint8_t *ptr = ((uint8_t *) run) + run->io.data_offset;
    *ptr = 0x20;
  }
}

static void handle_io_port_3f9(struct kvm_run *run, struct uart_registers *uart) {
  if (run->io.direction == KVM_EXIT_IO_OUT) {
      uart->ier = *(((char *) run) + run->io.data_offset);
  } else {
      uint8_t *ptr = ((uint8_t *) run) + run->io.data_offset;
      *ptr = uart->ier;
  }
}

void handle_kvm_io(struct kvm_run *run, struct uart_registers *uart) {
  switch (run->io.port) {
    case 0x3f8:
      handle_io_port_3f8(run);
      break;
    case 0x3f9:
      handle_io_port_3f9(run, uart);
      break;
    case 0x3fd:
      handle_io_port_3fd(run);
      break;
    default:
      break;
  }
}
