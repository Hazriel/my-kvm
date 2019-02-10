#ifndef OPTIONS_H
#define OPTIONS_H

#define _GNU_SOURCE

#include <stdlib.h>

#define UNIT_G  1 << 30
#define UNIT_M  1 << 20
#define UNIT_K  1 << 10

#define DEFAULT_RAM_SIZE UNIT_G

struct kvm_options {
  char *bz_im;
  char *initrd;
  char **kernel;
  size_t ram_size;
};

/**
 * Parse the command line looking for arguments that will be saved in a
 * struct kvm_options. If an error is detected, the program will exit with
 * an error code of 1.
 */
struct kvm_options* parse_kvm_options(int argc, char *argv[]);

/**
 * Checks the arguments in the struct kvm_options.
 * return -1 if options are invalid, 0 else
 */
int check_args(struct kvm_options *opts, char *argv[]);

void free_kvm_options(struct kvm_options *opt);

#endif /* ifndef OPTIONS_H */
