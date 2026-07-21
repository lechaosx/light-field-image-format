#pragma once

#include <algorithm>
#include <bit>
#include <iosfwd>
#include <span>

#include "quant_table.h"
#include "zigzag.h"
#include "endian.h"

using REFBLOCKUNIT = double;

template<size_t D>
using ReferenceBlock = DynamicBlock<REFBLOCKUNIT, D>;

template <size_t D>
using TraversalTable = DynamicBlock<size_t, D>;

template <size_t D>
void constructByReference(const ReferenceBlock<D> &reference, TraversalTable<D> &output) {
  DynamicBlock<std::pair<double, size_t>, D> srt(reference.size());

  for (size_t i = 0; i < reference.stride(D); i++) {
    srt[i].first += reference[i];
    srt[i].second = i;
  }

  //do NOT sort DC coefficient, thus +1 at the begining.
  std::ranges::stable_sort(std::span{&srt[1], srt.stride(D) - 1}, std::ranges::greater{}, &std::pair<double, size_t>::first);

  for (size_t i = 0; i < reference.stride(D); i++) {
    output[srt[i].second] = i;
  }
}

template <size_t D>
void constructByQuantTable(const QuantTable<D> &quant_table, TraversalTable<D> &output) {
  ReferenceBlock<D> dummy(output.size());

  for (size_t i = 0; i < dummy.stride(D); i++) {
    dummy[i] = quant_table[i];
    dummy[i] *= -1;
  }

  constructByReference(dummy, output);
}

template <size_t D>
void constructByRadius(TraversalTable<D> &output) {
  ReferenceBlock<D> dummy(output.size());

  for (const auto &pos : iterate_dimensions<D>(output.size())) {
    for (size_t i {}; i < D; i++) {
      dummy[pos] += pos[i] * pos[i];
    }

    dummy[pos] *= -1;
  }

  constructByReference(dummy, output);
}

template <size_t D>
void constructByDiagonals(TraversalTable<D> &output) {
  ReferenceBlock<D> dummy(output.size());

  for (const auto &pos : iterate_dimensions<D>(output.size())) {
    for (size_t i {}; i < D; i++) {
      dummy[pos] += pos[i];
    }

    dummy[pos] *= -1;
  }

  constructByReference(dummy, output);
}

template <size_t D>
void constructByHyperboloid(TraversalTable<D> &output) {
  ReferenceBlock<D> dummy(output.size());

  dummy.fill(1);

  for (const auto &pos : iterate_dimensions<D>(output.size())) {
    for (size_t i {}; i < D; i++) {
      dummy[pos] *= pos[i] + 1;
    }

    dummy[pos] *= -1;
  }

  constructByReference(dummy, output);
}

template <size_t D>
void constructZigzag(TraversalTable<D> &output) {
  output = zigzagTable<D>(output.size());
}

template <size_t D>
void writeTraversalToStream(const TraversalTable<D> &input, std::ostream &stream) {
  size_t max_bits = std::bit_width(input.stride(D) - 1);

  if (max_bits <= 8) {
    for (size_t i = 0; i < input.stride(D); i++) {
      writeValueToStream<uint8_t>(input[i], stream);
    }
  }
  else if (max_bits <= 16) {
    for (size_t i = 0; i < input.stride(D); i++) {
      writeValueToStream<uint16_t>(input[i], stream);
    }
  }
  else if (max_bits <= 32) {
    for (size_t i = 0; i < input.stride(D); i++) {
      writeValueToStream<uint32_t>(input[i], stream);
    }
  }
  else if (max_bits <= 64) {
    for (size_t i = 0; i < input.stride(D); i++) {
      writeValueToStream<uint64_t>(input[i], stream);
    }
  }
}


template <size_t D>
TraversalTable<D> readTraversalFromStream(const std::array<size_t, D> &BS, std::istream &stream) {
  TraversalTable<D> table(BS);

  size_t max_bits = std::bit_width(table.stride(D) - 1);

  if (max_bits <= 8) {
    for (size_t i = 0; i < table.stride(D); i++) {
      table[i] = readValueFromStream<uint8_t>(stream);
    }
  }
  else if (max_bits <= 16) {
    for (size_t i = 0; i < table.stride(D); i++) {
      table[i] = readValueFromStream<uint16_t>(stream);
    }
  }
  else if (max_bits <= 32) {
    for (size_t i = 0; i < table.stride(D); i++) {
      table[i] = readValueFromStream<uint32_t>(stream);
    }
  }
  else if (max_bits <= 64) {
    for (size_t i = 0; i < table.stride(D); i++) {
      table[i] = readValueFromStream<uint64_t>(stream);
    }
  }

  return table;
}
