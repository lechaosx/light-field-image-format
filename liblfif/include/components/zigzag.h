/**
* @file zigzag.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief The algorithm for generating zig-zag matrices.
*/

#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>

#include "block.h"

template <size_t D>
struct zigzagScanCore {
  template <typename F>
  zigzagScanCore(const size_t size[D], size_t pos[D], size_t rot[D], F &&callback) {
    auto move = [&]() {
      if (pos[rot[D - 1]] < size[rot[D - 1]] - 1) {
        for (size_t i = 2; i <= D; i++) {
          if (pos[rot[D - i]] > 0) {
            pos[rot[D - i]]--;
            pos[rot[D - 1]]++;
            return true;
          }
        }
      }

      return false;
    };

    do {
      zigzagScanCore<D - 1>(size, pos, rot, callback);
    } while (move());

    std::rotate(rot, rot + D - 1, rot + D);
  }
};

template <>
struct zigzagScanCore<1> {
  template <typename F>
  zigzagScanCore(const size_t[1], size_t pos[1], size_t[1], F &&callback) {
    callback(pos);
  }
};

template <size_t D, typename F>
void zigzagScanStart(const size_t size[D], size_t pos[D], F &&callback) {
  size_t rot[D] {};

  for (size_t i = 0; i < D; i++) {
    rot[i] = i;
  }

  auto move = [&]() {
    for (size_t i = 0 ; i < D; i++) {
      if (pos[rot[i]] < size[rot[i]] - 1) {
        pos[rot[i]]++;
        return true;
      }
    }
    return false;
  };

  do {
    zigzagScanCore<D>(size, pos, rot, callback);
  } while(move());
}

template <size_t D, typename F>
void zigzagScan(const size_t size[D], F &&callback) {
  size_t pos[D] {};
  zigzagScanStart<D>(size, pos, callback);
}

template <size_t D, typename F>
void zigzagScanSkipFirst(const size_t size[D], F &&callback) {
  size_t pos[D] {};
  pos[0] = 1;
  zigzagScanStart<D>(size, pos, callback);
}

template <size_t D>
DynamicBlock<size_t, D> zigzagTable(const std::array<size_t, D> &size) {
  DynamicBlock<size_t, D> block(size);
  size_t i = 0;
  zigzagScan<D>(size.data(), [&](size_t pos[D]) {
    std::array<size_t, D> pos_array {};
    std::copy(pos, pos + D, std::begin(pos_array));
    block[pos_array] = i++; }
  );
  return block;
}

template <typename F>
[[deprecated]]
void zigzagScan2D(const size_t size[2], F &&callback) {
  std::array<size_t, 2> rot { 0, 1 };
  std::array<size_t, 2> pos {};

  while (true) {
    while (true) {
      callback(pos.data());

      if (pos[rot[0]] > 0 && pos[rot[1]] < size[rot[1]] - 1) {
        pos[rot[0]]--;
        pos[rot[1]]++;
      }
      else {
        break;
      }
    }

    std::rotate(std::begin(rot), std::begin(rot) + 1, std::begin(rot) + 2);

    if (pos[rot[0]] < size[rot[0]] - 1) {
      pos[rot[0]]++;
    }
    else if (pos[rot[1]] < size[rot[1]] - 1){
      pos[rot[1]]++;
    }
    else {
      break;
    }
  }
}

template <typename F>
[[deprecated]]
void zigzagScan3D(const size_t size[3], F &&callback) {
  std::array<size_t, 3> rot { 0, 1, 2 };
  std::array<size_t, 3> pos {};

  while (true) {
    while (true) {
      while (true) {
        callback(pos.data());

        if (pos[rot[0]] > 0 && pos[rot[1]] < size[rot[1]] - 1) {
          pos[rot[0]]--;
          pos[rot[1]]++;
        }
        else {
          break;
        }
      }

      std::rotate(std::begin(rot), std::begin(rot) + 1, std::begin(rot) + 2);

      if (pos[rot[1]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
        pos[rot[1]]--;
        pos[rot[2]]++;
      }
      else if (pos[rot[0]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
        pos[rot[0]]--;
        pos[rot[2]]++;
      }
      else {
        break;
      }
    }

    std::rotate(std::begin(rot), std::begin(rot) + 2, std::begin(rot) + 3);

    if (pos[rot[0]] < size[rot[0]] - 1) {
      pos[rot[0]]++;
    }
    else if (pos[rot[1]] < size[rot[1]] - 1) {
      pos[rot[1]]++;
    }
    else if (pos[rot[2]] < size[rot[2]] - 1) {
      pos[rot[2]]++;
    }
    else {
      break;
    }
  }
}

template <typename F>
[[deprecated]]
void zigzagScan4D(const size_t size[4], F &&callback) {
  std::array<size_t, 4> rot { 0, 1, 2, 3 };
  std::array<size_t, 4> pos {};

  while (true) {
    while (true) {
      while (true) {
        while (true) {
          callback(pos.data());

          if (pos[rot[0]] > 0 && pos[rot[1]] < size[rot[1]] - 1) {
            pos[rot[0]]--;
            pos[rot[1]]++;
          }
          else {
            break;
          }
        }

        std::rotate(std::begin(rot), std::begin(rot) + 1, std::begin(rot) + 2);

        if (pos[rot[1]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
          pos[rot[1]]--;
          pos[rot[2]]++;
        }
        else if (pos[rot[0]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
          pos[rot[0]]--;
          pos[rot[2]]++;
        }
        else {
          break;
        }
      }

      std::rotate(std::begin(rot), std::begin(rot) + 2, std::begin(rot) + 3);

      if (pos[rot[2]] > 0 && pos[rot[3]] < size[rot[3]] - 1) {
        pos[rot[2]]--;
        pos[rot[3]]++;
      }
      else if (pos[rot[1]] > 0 && pos[rot[3]] < size[rot[3]] - 1) {
        pos[rot[1]]--;
        pos[rot[3]]++;
      }
      else if (pos[rot[0]] > 0 && pos[rot[3]] < size[rot[3]] - 1) {
        pos[rot[0]]--;
        pos[rot[3]]++;
      }
      else {
        break;
      }
    }

    std::rotate(std::begin(rot), std::begin(rot) + 3, std::begin(rot) + 4);

    if (pos[rot[0]] < size[rot[0]] - 1) {
      pos[rot[0]]++;
    }
    else if (pos[rot[1]] < size[rot[1]] - 1) {
      pos[rot[1]]++;
    }
    else if (pos[rot[2]] < size[rot[2]] - 1) {
      pos[rot[2]]++;
    }
    else if (pos[rot[3]] < size[rot[3]] - 1) {
      pos[rot[3]]++;
    }
    else {
      break;
    }
  }
}
