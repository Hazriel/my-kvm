#include "kvm.h"
#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct kvm* init_kvm_struct(int kvm_fd, int vm_fd, struct kvm_options *opts) {
  struct kvm *kvm = malloc(sizeof(struct kvm));
  if (!kvm)
    return NULL;
  kvm->kvm_fd = kvm_fd;
  kvm->vm_fd = vm_fd;
  kvm->initrd_file = opts->initrd;
  kvm->kernel_filename = opts->bz_im;

  int cmd_line_len = 0;
  for (int i = 0; opts->kernel[i] != NULL; ++i) {
    cmd_line_len += strlen(opts->kernel[i]);
    cmd_line_len++; // Add a space between each word
  }
  char *cmd_line = calloc(cmd_line_len, sizeof(char));
  if (!cmd_line) {
    free(kvm);
    return NULL;
  }

  for (int i = 0; opts->kernel[i] != NULL; ++i) {
    cmd_line = strcat(cmd_line, opts->kernel[i]);
    if (opts->kernel[i + 1] != NULL)
      cmd_line = strcat(cmd_line, " ");
  }

  kvm->kernel_cmd_line = cmd_line;
  return kvm;
}

void free_kvm_struct(struct kvm *kvm) {
  if (kvm->kernel_cmd_line != NULL)
    free(kvm->kernel_cmd_line);
  free(kvm);
}
