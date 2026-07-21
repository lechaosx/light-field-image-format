#include <charconv>
#include <cstring>
#include <print>

#include <compress.h>

void print_usage(const char *argv0) {
  std::println(stderr, "Usage:");
  std::println(stderr, "{} -i <file-mask> -o <file> -d <distortion> {{-p}} {{-s}}", argv0);
}

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, uint8_t &distortion, bool &predict, bool &shift) {
  input_file_mask  = nullptr;
  output_file_name = nullptr;
  distortion       = 0;
  predict          = false;
  shift            = false;

  bool dist_set    = false;

  char opt;
  while ((opt = getopt(argc, argv, "i:o:d:ps")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
        break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
        break;

      case 'd':
        if (!dist_set) {
          uint64_t dist {};
          auto [ptr, ec] = std::from_chars(optarg, optarg + std::strlen(optarg), dist);
          if (ec == std::errc{}) {
            distortion = dist;
            dist_set = true;
          }
          continue;
        }
        break;

      case 'p':
        if (!predict) {
          predict = true;
        }
        continue;

      case 's':
        if (!shift) {
          shift = true;
        }
        continue;

      default:
        break;
    }

    print_usage(argv[0]);
    return false;
  }

  if ((!input_file_mask) || (!output_file_name)) {
    print_usage(argv[0]);
    return false;
  }

  return true;
}
