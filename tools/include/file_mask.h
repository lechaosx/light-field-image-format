#pragma once

#include <string>
#include <string_view>

struct FileMask {
  std::string pattern;
};

FileMask    make_file_mask(std::string_view input_file_mask);
std::string file_mask_name(const FileMask &m, size_t index);
size_t      file_mask_count(const FileMask &m);
