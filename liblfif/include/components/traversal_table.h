/**
* @file traversal_table.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for generating linearization matrices.
*/

#ifndef TRAVERSAL_TABLE_H
#define TRAVERSAL_TABLE_H

#include "quant_table.h"
#include "zigzag.h"
#include "endian_t.h"

#include <iosfwd>

using REFBLOCKUNIT = double; /**< @brief Type intended to be used in reference block.*/

/**
 * @brief Reference block type.
 */
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
  stable_sort(&srt[0] + 1, &srt[srt.stride(D)], [](auto &left, auto &right) {
    return left.first > right.first;
  });

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

  iterate_dimensions<D>(output.size(), [&](const std::array<size_t, D> &pos) {
    for (size_t i {}; i < D; i++) {
      dummy[pos] += pos[i] * pos[i];
    }

    dummy[pos] *= -1;
  });

  constructByReference(dummy, output);
}

template <size_t D>
void constructByDiagonals(TraversalTable<D> &output) {
  ReferenceBlock<D> dummy(output.size());

  iterate_dimensions<D>(output.size(), [&](const std::array<size_t, D> &pos) {
    for (size_t i {}; i < D; i++) {
      dummy[pos] += pos[i];
    }

    dummy[pos] *= -1;
  });

  constructByReference(dummy, output);
}

template <size_t D>
void constructByHyperboloid(TraversalTable<D> &output) {
  ReferenceBlock<D> dummy(output.size());

  dummy.fill(1);

  iterate_dimensions<D>(output.size(), [&](const std::array<size_t, D> &pos) {
    for (size_t i {}; i < D; i++) {
      dummy[pos] *= pos[i] + 1;
    }

    dummy[pos] *= -1;
  });

  constructByReference(dummy, output);
}

template <size_t D>
void constructZigzag(TraversalTable<D> &output) {
  output = zigzagTable<D>(output.size());
}

template <size_t D>
void writeTraversalToStream(const TraversalTable<D> &input, std::ostream &stream) {
  size_t max_bits = ceil(log2(input.stride(D)));

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

  size_t max_bits = ceil(log2(table.stride(D)));

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

#endif
