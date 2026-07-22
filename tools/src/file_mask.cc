/******************************************************************************\
* SOUBOR: file_mask.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "file_mask.h"

#include <limits>
#include <stdexcept>

namespace {

size_t expansionCount(size_t digits) {
  size_t count = 1;
  for (size_t digit = 0; digit < digits; ++digit) {
    if (count > std::numeric_limits<size_t>::max() / 10) {
      return 0;
    }
    count *= 10;
  }
  return count;
}

std::string expandMask(
  const std::string &mask,
  const std::vector<size_t> &mask_indexes,
  size_t index
) {
  if (mask_indexes.empty()) {
    if (index == 0) {
      return mask;
    }
    throw std::out_of_range("file mask cannot represent index");
  }

  std::string image_number = std::to_string(index);
  if (image_number.size() > mask_indexes.size()) {
    throw std::out_of_range("file mask cannot represent index");
  }
  image_number.insert(image_number.begin(), mask_indexes.size() - image_number.size(), '0');

  std::string file_name = mask;
  for (size_t i = 0; i < mask_indexes.size(); ++i) {
    file_name[mask_indexes[i]] = image_number[i];
  }
  return file_name;
}

}

FileMask::FileMask(const std::string &input_file_mask): m_filename_mask{input_file_mask}, m_mask_indexes{} {
  for (size_t i = 0; i < m_filename_mask.size(); ++i) {
    if (m_filename_mask[i] == '#') {
      m_mask_indexes.push_back(i);
    }
  }
}

std::string FileMask::operator [](size_t index) const {
  return expandMask(m_filename_mask, m_mask_indexes, index);
}

size_t FileMask::count() const {
  return expansionCount(m_mask_indexes.size());
}

size_t get_mask_names_count(const std::string &mask, char masking_char) {
  size_t cnt {};

  for (char character : mask) {
    if (character == masking_char) {
      cnt++;
    }
  }

  return expansionCount(cnt);
}

std::string get_name_from_mask(const std::string &mask, char masking_char, size_t index) {
  std::vector<size_t> mask_indices {};

  for (size_t i = 0; i < mask.size(); ++i) {
    if (mask[i] == masking_char) {
      mask_indices.push_back(i);
    }
  }

  return expandMask(mask, mask_indices, index);
}
