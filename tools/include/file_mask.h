#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

class FileMask {
public:
  explicit FileMask(std::string_view input_file_mask);

  std::string operator [](size_t index) const;

  [[nodiscard]] size_t count() const;

private:
  std::string m_filename_mask;
  std::vector<size_t> m_mask_indexes;
};
