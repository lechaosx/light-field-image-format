/**
* @file mtf.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 1. 9. 2020
* @brief
*/

#ifndef MTF_H
#define MTF_H

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iterator>
#include <map>
#include <stdexcept>
#include <vector>

template<typename F>
void moveToFront(std::vector<uint64_t> &dictionary, std::vector<int64_t> &data, F &&update) {
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] < 0) {
      throw std::invalid_argument("MTF symbols must be nonnegative");
    }
    const uint64_t symbol = static_cast<uint64_t>(data[i]);
    if (symbol >= dictionary.size()) {
      if (symbol >= dictionary.max_size()) {
        throw std::length_error("MTF dictionary is too large");
      }
      size_t dict_end = dictionary.size();

      dictionary.resize(static_cast<size_t>(symbol) + 1);

      for (size_t i = dict_end; i < dictionary.size(); i++) {
        dictionary[i] = i;
      }
    }

    auto it = std::find(std::begin(dictionary), std::end(dictionary), symbol);
    if (it == std::end(dictionary)) {
      throw std::invalid_argument("MTF dictionary does not contain symbol");
    }

    size_t k = std::distance(std::begin(dictionary), it);

    data[i] = k;

    update(dictionary, k);
  }
}

template<typename F>
void moveFromFront(std::vector<uint64_t> &dictionary, std::vector<int64_t> &data, F &&update) {
  for (size_t i {}; i < data.size(); i++) {
    if (data[i] < 0) {
      throw std::invalid_argument("MTF indexes must be nonnegative");
    }
    uint64_t k = static_cast<uint64_t>(data[i]);

    if (k >= dictionary.size()) {
      if (k >= dictionary.max_size()) {
        throw std::length_error("MTF dictionary is too large");
      }
      size_t dict_end = dictionary.size();

      dictionary.resize(static_cast<size_t>(k) + 1);

      for (size_t i = dict_end; i < dictionary.size(); i++) {
        dictionary[i] = i;
      }
    }

    data[i] = dictionary[k];
    update(dictionary, k);
  }
}

void updateBase(std::vector<uint64_t> &dictionary, size_t i) {
  auto it = std::begin(dictionary) + i;
  std::rotate(std::begin(dictionary), it, std::next(it));
}

void updateM1ff(std::vector<uint64_t> &dictionary, size_t i) {
  auto it = std::begin(dictionary) + i;
  if (it <= std::next(std::begin(dictionary))) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }
  else {
    std::rotate(std::next(std::begin(dictionary)), it, std::next(it));
  }
}

void updateM1ff2(std::vector<uint64_t> &dictionary, size_t i, uint64_t prev[2]) {
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

void updateSticky(std::vector<uint64_t> &dictionary, size_t i, double v) {
  auto it = std::begin(dictionary) + i;

  size_t new_pos = std::round(i * v);
  std::rotate(std::begin(dictionary) + new_pos, it, std::next(it));
}

void update_k(std::vector<uint64_t> &dictionary, size_t i, size_t k) {
  auto it = std::begin(dictionary) + i;

  size_t new_pos = std::max<int64_t>(0, static_cast<int64_t>(i) - static_cast<int64_t>(k));
  std::rotate(std::begin(dictionary) + new_pos, it, std::next(it));
}

void updateC(std::vector<uint64_t> &dictionary, size_t i, size_t c, std::map<uint64_t, size_t> &counts) {
  auto it = std::begin(dictionary) + i;

  if (counts[*it] >= c) {
    std::rotate(std::begin(dictionary), it, std::next(it));
  }

  counts[*it]++;
}

#endif
