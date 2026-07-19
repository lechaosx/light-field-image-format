/******************************************************************************\
* SOUBOR: file_mask.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <format>
#include <string_view>

#include <file_mask.h>

FileMask::FileMask(std::string_view input_file_mask): m_filename_mask{input_file_mask}, m_mask_indexes{} {
  for (size_t i = 0; i < m_filename_mask.size(); i++) {
    if (m_filename_mask[i] == '#') {
      m_mask_indexes.push_back(i);
    }
  }
}

std::string FileMask::operator [](size_t index) const {
  std::string file_name {m_filename_mask};
  std::string image_number = std::format("{:0{}}", index, m_mask_indexes.size());

  for (size_t i = 0; i < m_mask_indexes.size(); i++) {
    file_name[m_mask_indexes[i]] = image_number[i];
  }

  return file_name;
}

size_t FileMask::count() {
  size_t result = 1;
  for (size_t i = 0; i < m_mask_indexes.size(); i++) result *= 10;
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
