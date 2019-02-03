#ifndef OPTIONS_H
#define OPTIONS_H

#include <err.h>
#include <stdlib.h>

typedef struct kvm_options {
  char *bz_im;
} kvm_options_t;

kvm_options_t* init_kvm_options(char *bz_im);
void free_kvm_options(kvm_options_t *opt);

#endif /* ifndef OPTIONS_H */
