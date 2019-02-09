#include "options.h"

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

// Helper functions

static struct kvm_options* init_kvm_options() {
  struct kvm_options *opt = malloc(sizeof(struct kvm_options));
  if (opt == NULL)
    errx(1, "couldn't allocate memory for kvm options");
  opt->bz_im = NULL;
  opt->initrd = NULL;
  opt->ram_size = 0;
  opt->kernel = NULL;
  return opt;
}

static void print_help(char *bin_name) {
  printf("usage: %s [options] bzImage [args]\n", bin_name);
  printf("options:\n");
  printf("\t-h, --help              Display this help message and exit.\n");
  printf("\t-i file, --initrd file  Use file as initial ram disk.\n");
  printf("\t-m ram, --memory ram    Set guest startup RAM to $ram megabytes.\n");
}

static void clean_error_exit(int err, char *mes, struct kvm_options *opts) {
  if (opts != NULL)
    free_kvm_options(opts);
  errx(err, "%s", mes);
}

static void get_additional_args(int argc, char *argv[], struct kvm_options *opts) {
  if (optind < argc) {
    int opt_cnt = argc - optind;
    opts->kernel = malloc((opt_cnt) * sizeof(char*));
    if (opts->kernel == NULL) {
      clean_error_exit(1, "bzImage already specified in args", opts);
    }
    for (int i = 0; i < opt_cnt; ++i)
      opts->kernel[i] = NULL;
    int i = 0;
    while (optind < argc) {
      if (opts->bz_im == NULL) {
        opts->bz_im = argv[optind++];
      } else {
        opts->kernel[i++] = argv[optind++];
      }
    }
  }
}

static size_t ram_times_unit(size_t nb, size_t unit) {
  return nb * unit;
}

/*
 * Parse the string passed in argument and returns the size of the ram
 * in bytes. If there is an error, return 0 as a VM with 0 bytes of ram
 * makes no sense.
 */
static size_t get_ram_size(char *ram) {
  if (!ram)
    return 0;
  char unit = 0;
  char nb_buf[128] = { 0 };
  // Consider that if the string is greater than 128 digits, it is an error
  if (strlen(ram) > sizeof(nb_buf))
    return 0;
  for (size_t i = 0; ram[i] != 0; ++i) {
    if (ram[i] >= '0' && ram[i] <= '9') {
      nb_buf[i] = ram[i];
    } else {
      unit = ram[i];
      if (ram[i + 1] != 0) {
        fprintf(stderr, "./my-kvm: invalid unit for ram size.\n");
        return 0;
      }
    }
  }
  // Now get the number as an int
  int res = atoi(nb_buf);
  if (res < 0) {
    fprintf(stderr, "./my-kvm: ram can not be a negative value.\n");
    return 0;
  }

  if (!unit)
    return res;

  switch (unit) {
    case 'G':
      return ram_times_unit(res, UNIT_G);
    case 'M':
      return ram_times_unit(res, UNIT_M);
    case 'K':
      return ram_times_unit(res, UNIT_K);
    default:
      fprintf(stderr, "./my-kvm: invalid unit for ram size.\n");
      return 0;
  }
}

struct kvm_options* parse_kvm_options(int argc, char *argv[]) {
  struct kvm_options *opts = init_kvm_options();

  static struct option options[] = {
    { "help", no_argument, 0, 'h' },
    { "initrd", required_argument, 0, 'i' },
    { "memory", required_argument, 0, 'm' },
    { 0, 0, 0, 0 }
  };

  int c = 0;
  int opt_idx = 0;
  while ((c = getopt_long(argc, argv, "hi:m:", options, &opt_idx)) != -1) {
    switch (c) {
      case 'h':
        print_help(argv[0]);
        free_kvm_options(opts);
        exit(0);
      case 'i':
        opts->initrd = optarg;
        break;
      case 'm':
        opts->ram_size = get_ram_size(optarg);
        if (opts->ram_size == 0)
          clean_error_exit(1, "error specifying ram size", opts);
        break;
      case '?':
        clean_error_exit(1, "unknown option", opts);
    }
  }

  if (opts->ram_size == 0)
    opts->ram_size = DEFAULT_RAM_SIZE;

  get_additional_args(argc, argv, opts);

  return opts;
}

void check_args(struct kvm_options *opts, char *argv[]) {
  if (opts == NULL) {
    clean_error_exit(1, "no options specified", opts);
  }

  if (opts->bz_im == NULL) {
    clean_error_exit(1, "no bzImage specified", opts);
  }

  FILE *file = fopen(opts->bz_im, "r+");
  if (file == NULL) {
    clean_error_exit(1, "bzImage does not exist", opts);
  }
  fclose(file);

  if (opts->initrd != NULL) {
    file = fopen(opts->initrd, "r+");
    if (file == NULL) {
      clean_error_exit(1, "initrd does not exist", opts);
    }
    fclose(file);
  }
}

void free_kvm_options(struct kvm_options *opt) {
  if (opt->kernel != NULL)
    free(opt->kernel);
  free(opt);
}
