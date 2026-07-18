/******************************************************************************\
* SOUBOR: file_mask.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <cmath>
#include <iomanip>

#include <file_mask.h>

FileMask::FileMask(const std::string &input_file_mask): m_filename_mask{input_file_mask}, m_mask_indexes{} {
  for (size_t i = 0; m_filename_mask[i] != '\0'; i++) {
    if (m_filename_mask[i] == '#') {
      m_mask_indexes.push_back(i);
    }
  }
}

std::string FileMask::operator [](size_t index) const {
  std::string file_name {m_filename_mask};

  std::stringstream image_number {};
  image_number << std::setw(m_mask_indexes.size()) << std::setfill('0') << std::to_string(index);

  for (size_t i = 0; i < m_mask_indexes.size(); i++) {
    file_name[m_mask_indexes[i]] = image_number.str()[i];
  }

  return file_name;
}

size_t FileMask::count() {
  return pow(10, m_mask_indexes.size());
}

size_t get_mask_names_count(const std::string &mask, char masking_char) {
  size_t cnt {};

  for (size_t i = 0; mask[i] != '\0'; i++) {
    if (mask[i] == masking_char) {
      cnt++;
    }
  }

  return pow(10, cnt);
}

std::string get_name_from_mask(const std::string &mask, char masking_char, size_t index) {
  std::vector<size_t> mask_indices {};
  std::stringstream   image_number {};
  std::string         name         { mask };

  for (size_t i = 0; mask[i] != '\0'; i++) {
    if (mask[i] == masking_char) {
      mask_indices.push_back(i);
    }
  }

  image_number << std::setw(mask_indices.size()) << std::setfill('0') << std::to_string(index);

  for (size_t i = 0; i < mask_indices.size(); i++) {
    name[mask_indices[i]] = image_number.str()[i];
  }

  return name;
}
