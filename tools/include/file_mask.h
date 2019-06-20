/**
* @file file_mask.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for expanding the file mask into individual file names.
*/

#ifndef FILE_MASK_H
#define FILE_MASK_H

#include <vector>
#include <string>

/**
 * @brief Class used to expang file mask into file names.
 */
class FileMask {
public:
  /**
   * @brief The constructor which takes file mask with characters '#'.
   * @param input_file_mask The mask which will be expanded.
   */
  FileMask(const std::string &input_file_mask);

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
  size_t count();

private:
  std::string m_filename_mask;
  std::vector<size_t> m_mask_indexes;
};

size_t get_mask_names_count(const std::string &mask, char masking_char);
std::string get_name_from_mask(const std::string &mask, char masking_char, size_t index);

#endif
