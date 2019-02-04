#ifndef OPTIONS_H
#define OPTIONS_H

#define _GNU_SOURCE

typedef struct kvm_options {
  char *bz_im;
  char *initrd;
  char *ram;
  char **kernel;
} kvm_options_t;

/**
 * Parse the command line looking for arguments that will be saved in a
 * struct kvm_options. If an error is detected, the program will exit with
 * an error code of 1.
 */
kvm_options_t* parse_kvm_options(int argc, char *argv[]);

/**
 * Checks the arguments in the struct kvm_options.
 * Exits the program with an error code of 1 if arguments are invalid.
 */
void check_args(kvm_options_t *opts, char *argv[]);

void free_kvm_options(kvm_options_t *opt);

#endif /* ifndef OPTIONS_H */
