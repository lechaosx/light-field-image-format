#include <format>
#include <string_view>

#include <file_mask.h>

FileMask make_file_mask(std::string_view input_file_mask) {
  FileMask m;
  m.filename_mask = input_file_mask;
  for (size_t i = 0; i < m.filename_mask.size(); i++) {
    if (m.filename_mask[i] == '#') {
      m.mask_indexes.push_back(i);
    }
  }
  return m;
}

std::string file_mask_name(const FileMask &m, size_t index) {
  std::string file_name { m.filename_mask };
  std::string image_number = std::format("{:0{}}", index, m.mask_indexes.size());
  for (size_t i = 0; i < m.mask_indexes.size(); i++) {
    file_name[m.mask_indexes[i]] = image_number[i];
  }
  return file_name;
}

size_t file_mask_count(const FileMask &m) {
  size_t result = 1;
  for (size_t i = 0; i < m.mask_indexes.size(); i++) result *= 10;
  return result;
}

size_t get_mask_names_count(std::string_view mask, char masking_char) {
  size_t cnt {};
  for (size_t i = 0; i < mask.size(); i++) {
    if (mask[i] == masking_char) {
      cnt++;
    }
  }
  size_t result = 1;
  for (size_t i = 0; i < cnt; i++) result *= 10;
  return result;
}

std::string get_name_from_mask(std::string_view mask, char masking_char, size_t index) {
  std::vector<size_t> mask_indices {};
  std::string         name         { mask };

  for (size_t i = 0; i < mask.size(); i++) {
    if (mask[i] == masking_char) {
      mask_indices.push_back(i);
    }
  }

  std::string image_number = std::format("{:0{}}", index, mask_indices.size());

  for (size_t i = 0; i < mask_indices.size(); i++) {
    name[mask_indices[i]] = image_number[i];
  }

  return name;
}
