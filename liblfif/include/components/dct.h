#pragma once

#include <cmath>
#include <numbers>
#include <array>
#include <span>

#include "block.h"
#include "meta.h"

template<size_t D>
class DCTCoefs;

template <>
class DCTCoefs<1> {
  DynamicBlock<float, 2> coefs;

public:
  DCTCoefs(std::span<const size_t, 1> size): coefs({size[0], size[0]}) {
    const auto c1 = std::sqrt(2.f / size[0]);
    const auto c2 = 1.f / std::sqrt(2.f);
    const auto c3 = 1.f / (2.f * size[0]);

    for (size_t u = 0; u < size[0]; u++) {
      for (size_t x = 0; x < size[0]; x++) {
        coefs[{x, u}] = std::cos(((2 * x + 1) * u * std::numbers::pi) * c3) * c1;
      }
    }

    for (size_t x = 0; x < size[0]; ++x) {
      coefs[{x, 0}] *= c2;
    }
  }

  [[nodiscard]] float operator[](const std::array<size_t, 2> &pos) const {
    return coefs[pos];
  }
};

template <size_t D>
class DCTCoefs {
public:
  DCTCoefs<1> current;
  DCTCoefs<D - 1> next;

  DCTCoefs(std::span<const size_t, D> size): current(size.template last<1>()), next(size.template first<D - 1>()) {}
};

template <size_t D, LinearRef<float> Block>
void fdct(const std::array<size_t, D> &block_size, const DCTCoefs<D> &coefs, Block &&block) {
  if constexpr (D == 1) {
    DynamicBlock<float, 1> inputs({block_size[0]});

    for (size_t x = 0; x < block_size[0]; x++) {
      inputs[x] = block(x);
    }

    for (size_t u = 0; u < block_size[0]; u++) {
      block(u) = 0.f;
      for (size_t x = 0; x < block_size[0]; x++) {
        block(u) += inputs[x] * coefs[{x, u}];
      }
    }
  } else {
    std::array<size_t, D - 1> subblock_size;
    std::ranges::copy(std::span{block_size}.template first<D - 1>(), subblock_size.begin());

    const auto stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      fdct<D - 1>(subblock_size, coefs.next, [&](size_t index) -> auto & {
        return block(slice * stride + index);
      });
    }

    for (size_t noodle = 0; noodle < stride; noodle++) {
      fdct<1>(std::array<size_t, 1>{block_size[D - 1]}, coefs.current, [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      });
    }
  }
}

template <size_t D, LinearRef<float> Block>
void idct(const std::array<size_t, D> &block_size, const DCTCoefs<D> &coefs, Block &&block) {
  if constexpr (D == 1) {
    DynamicBlock<float, 1> inputs({block_size[0]});

    for (size_t x = 0; x < block_size[0]; x++) {
      inputs[x] = block(x);
      block(x) = 0.f;
    }

    for (size_t u = 0; u < block_size[0]; u++) {
      for (size_t x = 0; x < block_size[0]; x++) {
        block(x) += inputs[u] * coefs[{x, u}];
      }
    }
  } else {
    std::array<size_t, D - 1> subblock_size;
    std::ranges::copy(std::span{block_size}.template first<D - 1>(), subblock_size.begin());

    const auto stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      idct<D - 1>(subblock_size, coefs.next, [&](size_t index) -> auto & {
        return block(slice * stride + index);
      });
    }

    for (size_t noodle = 0; noodle < stride; noodle++) {
      idct<1>(std::array<size_t, 1>{block_size[D - 1]}, coefs.current, [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      });
    }
  }
}
