#pragma once

// Hardcoded per-dimension zigzag scans, preserved verbatim from v1.0.0
// (liblfif/include/components/zigzag.h). They are superseded in the library by
// the generalized N-dimensional zigzagScan, and kept here ONLY as independent
// cross-check oracles: the explicit 2D/3D/4D loop structure below was written
// separately from the generalized algorithm, so agreement between them
// validates the generalization.

#include <array>
#include <algorithm>
#include <cstddef>

namespace oracle {

template <typename F>
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

} // namespace oracle
