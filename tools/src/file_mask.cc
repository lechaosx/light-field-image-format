#include "file_mask.h"

#include <limits>
#include <stdexcept>

FileMask::FileMask(std::string_view input_file_mask)
    : m_filename_mask{input_file_mask} {
  for (size_t i = 0; i < m_filename_mask.size(); ++i) {
    if (m_filename_mask[i] == '#') {
      m_mask_indexes.push_back(i);
    }
  }
}

std::string FileMask::operator [](size_t index) const {
  if (m_mask_indexes.empty()) {
    if (index == 0) {
      return m_filename_mask;
    }
    throw std::out_of_range("file mask cannot represent index");
  }

  std::string image_number = std::to_string(index);
  if (image_number.size() > m_mask_indexes.size()) {
    throw std::out_of_range("file mask cannot represent index");
  }
  image_number.insert(
      image_number.begin(), m_mask_indexes.size() - image_number.size(), '0');

  std::string file_name = m_filename_mask;
  for (size_t i = 0; i < m_mask_indexes.size(); ++i) {
    file_name[m_mask_indexes[i]] = image_number[i];
  }
  return file_name;
}

size_t FileMask::count() const {
  size_t count = 1;
  for (size_t digit = 0; digit < m_mask_indexes.size(); ++digit) {
    if (count > std::numeric_limits<size_t>::max() / 10) {
      throw std::length_error("file mask is too large");
    }
    count *= 10;
  }
  return count;
}
