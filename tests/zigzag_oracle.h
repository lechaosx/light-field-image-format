#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace oracle {

template <typename F>
void zigzagScan2D(const size_t size[2], F &&callback) {
  std::array<size_t, 2> rot {0, 1};
  std::array<size_t, 2> pos {};

  while (true) {
    while (true) {
      callback(pos.data());

      if (pos[rot[0]] > 0 && pos[rot[1]] < size[rot[1]] - 1) {
        pos[rot[0]]--;
        pos[rot[1]]++;
      } else {
        break;
      }
    }

    std::rotate(rot.begin(), rot.begin() + 1, rot.end());

    if (pos[rot[0]] < size[rot[0]] - 1) {
      pos[rot[0]]++;
    } else if (pos[rot[1]] < size[rot[1]] - 1) {
      pos[rot[1]]++;
    } else {
      break;
    }
  }
}

template <typename F>
void zigzagScan3D(const size_t size[3], F &&callback) {
  std::array<size_t, 3> rot {0, 1, 2};
  std::array<size_t, 3> pos {};

  while (true) {
    while (true) {
      while (true) {
        callback(pos.data());

        if (pos[rot[0]] > 0 && pos[rot[1]] < size[rot[1]] - 1) {
          pos[rot[0]]--;
          pos[rot[1]]++;
        } else {
          break;
        }
      }

      std::rotate(rot.begin(), rot.begin() + 1, rot.begin() + 2);

      if (pos[rot[1]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
        pos[rot[1]]--;
        pos[rot[2]]++;
      } else if (pos[rot[0]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
        pos[rot[0]]--;
        pos[rot[2]]++;
      } else {
        break;
      }
    }

    std::rotate(rot.begin(), rot.begin() + 2, rot.end());

    if (pos[rot[0]] < size[rot[0]] - 1) {
      pos[rot[0]]++;
    } else if (pos[rot[1]] < size[rot[1]] - 1) {
      pos[rot[1]]++;
    } else if (pos[rot[2]] < size[rot[2]] - 1) {
      pos[rot[2]]++;
    } else {
      break;
    }
  }
}

template <typename F>
void zigzagScan4D(const size_t size[4], F &&callback) {
  std::array<size_t, 4> rot {0, 1, 2, 3};
  std::array<size_t, 4> pos {};

  while (true) {
    while (true) {
      while (true) {
        while (true) {
          callback(pos.data());

          if (pos[rot[0]] > 0 && pos[rot[1]] < size[rot[1]] - 1) {
            pos[rot[0]]--;
            pos[rot[1]]++;
          } else {
            break;
          }
        }

        std::rotate(rot.begin(), rot.begin() + 1, rot.begin() + 2);

        if (pos[rot[1]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
          pos[rot[1]]--;
          pos[rot[2]]++;
        } else if (pos[rot[0]] > 0 && pos[rot[2]] < size[rot[2]] - 1) {
          pos[rot[0]]--;
          pos[rot[2]]++;
        } else {
          break;
        }
      }

      std::rotate(rot.begin(), rot.begin() + 2, rot.begin() + 3);

      if (pos[rot[2]] > 0 && pos[rot[3]] < size[rot[3]] - 1) {
        pos[rot[2]]--;
        pos[rot[3]]++;
      } else if (pos[rot[1]] > 0 && pos[rot[3]] < size[rot[3]] - 1) {
        pos[rot[1]]--;
        pos[rot[3]]++;
      } else if (pos[rot[0]] > 0 && pos[rot[3]] < size[rot[3]] - 1) {
        pos[rot[0]]--;
        pos[rot[3]]++;
      } else {
        break;
      }
    }

    std::rotate(rot.begin(), rot.begin() + 3, rot.end());

    if (pos[rot[0]] < size[rot[0]] - 1) {
      pos[rot[0]]++;
    } else if (pos[rot[1]] < size[rot[1]] - 1) {
      pos[rot[1]]++;
    } else if (pos[rot[2]] < size[rot[2]] - 1) {
      pos[rot[2]]++;
    } else if (pos[rot[3]] < size[rot[3]] - 1) {
      pos[rot[3]]++;
    } else {
      break;
    }
  }
}

}
