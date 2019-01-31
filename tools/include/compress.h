/******************************************************************************\
* SOUBOR: compress.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef COMPRESS_H
#define COMPRESS_H

#include <cstdint>

void print_usage(const char *argv0);
bool parse_args(int argc, char *argv[], const char *&input_file_mask, const char *&output_file_name, uint8_t &quality);

#endif
