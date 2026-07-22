#pragma once

#include <string_view>

namespace file_format {

inline constexpr std::string_view wavelet_2d_magic {"LFIF-2D"};
inline constexpr std::string_view wavelet_4d_magic {"LFWF-4D"};

[[nodiscard]] constexpr bool is_wavelet_2d(std::string_view magic) {
  return magic == wavelet_2d_magic;
}

[[nodiscard]] constexpr bool is_wavelet_4d(std::string_view magic) {
  return magic == wavelet_4d_magic;
}

} // namespace file_format
