#include "options.h"

// Helper functions

static kvm_options_t* init_kvm_options() {
  kvm_options_t *opt = malloc(sizeof(kvm_options_t));
  if (opt == NULL)
    err(1, "couldn't allocate memory for kvm options");
  opt->bz_im = NULL;
  opt->initrd = NULL;
  opt->ram = NULL;
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

static void print_error_and_exit(int err, char *mes, char *bin) {
  if (mes != NULL && bin != NULL) {
    fprintf(stderr, "%s: %s\n", bin, mes);
  }
  exit(err);
}

/**
 * Here if we find that the argument contains an equal sign, then it is an
 * argument to be passed to the kernel. If not then it is the bzImage.
 */
static int is_bz_image(char *arg) {
  if (arg == NULL)
    return 0;
  for (int i = 0; arg[i] != 0; ++i) {
    if (arg[i] == '=')
      return 0;
  }
  return 1;
}

static void get_additional_args(int argc, char *argv[], kvm_options_t *opts) {
  if (optind < argc) {
    int opt_cnt = argc - optind;
    opts->kernel = malloc((opt_cnt) * sizeof(char*));
    if (opts->kernel == NULL) {
      free_kvm_options(opts);
      print_error_and_exit(1, "couldn't allocate memory for kvm options", argv[0]);
    }
    for (int i = 0; i < opt_cnt; ++i)
      opts->kernel[i] = NULL;
    int i = 0;
    while (optind < argc) {
      if (is_bz_image(argv[optind])) {
        if (opts->bz_im != NULL) {
          free_kvm_options(opts);
          print_error_and_exit(1, "bzImage already specified in args", argv[0]);
        }
        opts->bz_im = argv[optind++];
      } else {
        opts->kernel[i++] = argv[optind++];
      }
    }
  }
}

kvm_options_t* parse_kvm_options(int argc, char *argv[]) {
  kvm_options_t *opts = init_kvm_options();

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
        opts->ram = optarg;
        break;
      case '?':
        exit(1);
    }
  }

  get_additional_args(argc, argv, opts);

  return opts;
}

void check_args(kvm_options_t *opts, char *argv[]) {
  if (opts == NULL) {
    fprintf(stderr, "%s: no options specified", argv[0]);
    exit(1);
  }
  if (opts->bz_im == NULL) {
    fprintf(stderr, "%s: no bzImage specified", argv[0]);
    exit(1);
  }
}

void free_kvm_options(kvm_options_t *opt) {
  if (opt->kernel != NULL)
    free(opt->kernel);
  free(opt);
}
