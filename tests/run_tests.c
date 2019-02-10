#include "options.h"

#include <stdio.h>

static void print_test_result(const char *name, int res, int expected) {
  printf("%s : %s\n", name, res == expected ? "[OK]" : "[KO]");
}

static int invalid_bz_image() {
  int argc = 2;
  char *argv[] = {"my-kvm", "nnthauxt"};
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  return opts == NULL ? 0 : 1;
}

static int invalid_bz_image2() {
  int argc = 4;
  char *argv[] = {"my-kvm", "-m", "2G",  "bullshit"};
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  return opts == NULL ? 0 : 1;
}

static int invalid_bz_image_magic() {
  int argc = 4;
  char *argv[] = {"my-kvm", "-m", "2G",  "Makefile"};
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  return opts == NULL ? 0 : 1;
}

static int invalid_initrd(char *bzimage) {
  int argc = 4;
  char *argv[] = {"my-kvm", "--initrd", "bullshit",  bzimage};
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  return opts == NULL ? 0 : 1;
}

static int invalid_mem_size(char *bzimage) {
  int argc = 4;
  char *argv[] = {"my-kvm", "-m", "1298779P",  bzimage};
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  return opts == NULL ? 0 : 1;
}

static int invalid_mem_size2(char *bzimage) {
  int argc = 4;
  char *argv[] = {"my-kvm", "-m", "-1234M",  bzimage};
  struct kvm_options *opts = parse_kvm_options(argc, argv);
  return opts == NULL ? 0 : 1;
}

int main(int argc, char *argv[])
{
  struct kvm_options *opts = parse_kvm_options(argc, argv);

  if (!opts || !opts->bz_im) {
    fprintf(stderr, "Invalid or no options given.\n");
    return 1;
  }

  print_test_result("invalid_bz_image", invalid_bz_image(), 1);
  print_test_result("invalid_bz_image2", invalid_bz_image2(), 1);
  print_test_result("invalid_bz_image_magic", invalid_bz_image_magic(), 1);
  print_test_result("invalid_initrd", invalid_initrd(opts->bz_im), 1);
  print_test_result("invalid_mem_size", invalid_mem_size(opts->bz_im), 1);
  print_test_result("invalid_mem_size2", invalid_mem_size2(opts->bz_im), 1);

  return 0;
}
