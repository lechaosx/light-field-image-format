/******************************************************************************\
* SOUBOR: decompress.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <cstdint>

void print_usage(const char *argv0);
bool parse_args(int argc, char *argv[], const char *&input_file_name, const char *&output_file_mask);

#endif
