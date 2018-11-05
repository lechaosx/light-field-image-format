#ifndef DCT_H
#define DCT_H

#include "constmath.h"

#include <cstdint>
#include <cmath>
#include <functional>

using namespace std;

template <uint8_t D>
inline void fdct(function<float(uint64_t)> &&input, function<float &(uint64_t)> &&output) {
  float tmp[constpow(8, D)] {};
  for (uint8_t slice = 0; slice < 8; slice++) {
    fdct<D-1>([&](uint64_t index) -> float { return input(slice*constpow(8, D-1) + index); }, [&](uint64_t index) -> float & { return tmp[slice*constpow(8, D-1) + index]; });
  }
  for (uint64_t noodle = 0; noodle < constpow(8, D-1); noodle++) {
    fdct<1>([&](uint8_t index) -> float { return tmp[index*constpow(8, D-1) + noodle]; }, [&](uint8_t index) -> float & { return output(index*constpow(8, D-1) + noodle); });
  }
}

template <>
inline void fdct<1>(function<float(uint64_t)> &&input, function<float &(uint64_t)> &&output) {
  for (uint8_t x = 0; x < 8; x++) {
    output(0) += input(x);
  }

  output(0) /= sqrt(2);

  for (uint8_t u = 1; u < 8; u++) {
    for (uint8_t x = 0; x < 8; x++) {
      output(u) += input(x) * cos(((2 * x + 1) * u * M_PI ) / 16);
    }
  }
}

template <uint8_t D>
inline void idct(function<float(uint64_t)> &&input, function<float &(uint64_t)> &&output) {
  float tmp[constpow(8, D)] {};
  for (uint8_t slice = 0; slice < 8; slice++) {
    idct<D-1>([&](uint64_t index) -> float { return input(slice*constpow(8, D-1) + index); }, [&](uint64_t index) -> float & { return tmp[slice*constpow(8, D-1) + index]; });
  }
  for (uint64_t noodle = 0; noodle < constpow(8, D-1); noodle++) {
    idct<1>([&](uint8_t index) -> float { return tmp[index*constpow(8, D-1) + noodle]; }, [&](uint8_t index) -> float & { return output(index*constpow(8, D-1) + noodle); });
  }
}

template <>
inline void idct<1>(function<float(uint64_t)> &&input, function<float &(uint64_t)> &&output) {
  for (uint8_t x = 0; x < 8; x++) {
    output(x) = input(0) / sqrt(2);

    for (uint8_t u = 1; u < 8; u++) {
      output(x) += input(u) * cos(((2 * x + 1) * u * M_PI) / 16);
    }
  }
}

#endif