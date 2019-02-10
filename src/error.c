#include <stdio.h>

void kvm_print_error(const char *err) {
  if (err)
    fprintf(stderr, "my-kvm: %s.\n", err);
  else
    fprintf(stderr, "my-kvm: unspecified error.\n");
}
