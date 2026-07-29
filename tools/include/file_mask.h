/**
* @file file_mask.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for expanding the file mask into individual file names.
*/

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Class used to expang file mask into file names.
 */
class FileMask {
public:
  /**
   * @brief The constructor which takes file mask with characters '#'.
   * @param input_file_mask The mask which will be expanded.
   */
  explicit FileMask(std::string_view input_file_mask);

  /**
   * @brief The overloaded operator for indexing which performs expansion of a mask.
   * @param index The number to which the mask shall be expanded.
   * @return The expanded string.
   */
  std::string operator [](size_t index) const;

  /**
   * @brief The method which returns maximum number to which the mask can be expanded before overflowing.
   * @return The maximum expanding number.
   */
  size_t count() const;

private:
  std::string m_filename_mask;
  std::vector<size_t> m_mask_indexes;
};
