#pragma once

#include <getopt.h>

#include <cstdint>
#include <cstddef>

void print_usage(const char *argv0);

bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, uint8_t &distortion, bool &predict, bool &shift);
