// Move-to-front transform, preserved from liblfif/include/components/mtf.h
// (removed in "cleanups: dead code removal ..."). Never wired into the codec
// pipeline, but a working companion to the live BWT and worth keeping runnable.
// Kept verbatim except for this note; test_mtf.cc pins its invertibility.

#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <map>
#include <algorithm>

template<typename F>
void moveToFront(std::vector<uint64_t> &dictionary, std::vector<int64_t> &data, F &&update) {
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] >= dictionary.size()) {
      size_t dict_end = dictionary.size();

      dictionary.resize(data[i] + 1);

      for (size_t i = dict_end; i < dictionary.size(); i++) {
        dictionary[i] = i;
      }
    }

    auto it = std::find(std::begin(dictionary), std::end(dictionary), data[i]);

    size_t k = std::distance(std::begin(dictionary), it);

    data[i] = k;

    update(dictionary, k);
  }
}

template<typename F>
void moveFromFront(std::vector<uint64_t> &dictionary, std::vector<int64_t> &data, F &&update) {
  for (size_t i {}; i < data.size(); i++) {
    uint64_t k = data[i];

    if (k >= dictionary.size()) {
      size_t dict_end = dictionary.size();

      dictionary.resize(k + 1);

      for (size_t i = dict_end; i < dictionary.size(); i++) {
        dictionary[i] = i;
      }
    }

    data[i] = dictionary[k];
    update(dictionary, k);
  }
}

inline void updateBase(std::vector<uint64_t> &dictionary, size_t i) {
  auto it = std::begin(dictionary) + i;
  std::rotate(std::begin(dictionary), it, std::next(it));
}

inline void updateM1ff(std::vector<uint64_t> &dictionary, size_t i) {
  auto it = std::begin(dictionary) + i;
  if (it <= std::next(std::begin(dictionary))) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }
  else {
    std::rotate(std::next(std::begin(dictionary)), it, std::next(it));
  }
}

inline void updateM1ff2(std::vector<uint64_t> &dictionary, size_t i, uint64_t prev[2]) {
  auto it = std::begin(dictionary) + i;

  uint64_t c = *it;

  if (it <= std::next(std::begin(dictionary))) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }
  else if (prev[0] == *std::begin(dictionary) || prev[1] == *std::begin(dictionary)) {
    std::rotate(std::next(std::begin(dictionary)), it, std::next(it));
  }
  else {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }

  prev[1] = prev[0];
  prev[0] = c;
}

inline void updateSticky(std::vector<uint64_t> &dictionary, size_t i, double v) {
  auto it = std::begin(dictionary) + i;

  size_t new_pos = std::round(i * v);
  std::rotate(std::begin(dictionary) + new_pos, it, std::next(it));
}

inline void update_k(std::vector<uint64_t> &dictionary, size_t i, size_t k) {
  auto it = std::begin(dictionary) + i;

  size_t new_pos = std::max<int64_t>(0, static_cast<int64_t>(i) - static_cast<int64_t>(k));
  std::rotate(std::begin(dictionary) + new_pos, it, std::next(it));
}

inline void updateC(std::vector<uint64_t> &dictionary, size_t i, size_t c, std::map<uint64_t, size_t> &counts) {
  auto it = std::begin(dictionary) + i;

  if (counts[*it] >= c) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }

  counts[*it]++;
}
