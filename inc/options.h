#ifndef OPTIONS_H
#define OPTIONS_H

#define _GNU_SOURCE

struct kvm_options {
  char *bz_im;
  char *initrd;
  char *ram;
  char **kernel;
};

/**
 * Parse the command line looking for arguments that will be saved in a
 * struct kvm_options. If an error is detected, the program will exit with
 * an error code of 1.
 */
struct kvm_options* parse_kvm_options(int argc, char *argv[]);

/**
 * Checks the arguments in the struct kvm_options.
 * Exits the program with an error code of 1 if arguments are invalid.
 */
void check_args(struct kvm_options *opts, char *argv[]);

void free_kvm_options(struct kvm_options *opt);

#endif /* ifndef OPTIONS_H */
