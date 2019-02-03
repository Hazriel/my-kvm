#include "options.h"

kvm_options_t* init_kvm_options(char *bz_im) {
  kvm_options_t *opt = malloc(sizeof(kvm_options_t));
  if (opt == NULL)
    err(1, "couldn't allocate memory for kvm options.");
  opt->bz_im = bz_im;
  return opt;
}

void free_kvm_options(kvm_options_t *opt) {
  free(opt);
}
