#pragma once

#include <string>
#include <string_view>
#include <vector>

struct FileMask {
  std::string          filename_mask;
  std::vector<size_t>  mask_indexes;
};

FileMask    make_file_mask(std::string_view input_file_mask);
std::string file_mask_name(const FileMask &m, size_t index);
size_t      file_mask_count(const FileMask &m);

size_t      get_mask_names_count(std::string_view mask, char masking_char);
std::string get_name_from_mask(std::string_view mask, char masking_char, size_t index);
