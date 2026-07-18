/******************************************************************************\
* SOUBOR: decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"

#include <getopt.h>
#include <print>

void print_usage(const char *argv0) {
  std::println(stderr, "Usage:");
  std::println(stderr, "{} -i <file> -o <file-mask>", argv0);
}

bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_mask) {
  char opt;
  while ((opt = getopt(argc, argv, "i:o:q:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_name) {
          input_file_name = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_mask) {
          output_file_mask = optarg;
          continue;
        }
        break;

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((!input_file_name) || (!output_file_mask)) {
    print_usage(argv[0]);
    return false;
  }

  return true;
}
