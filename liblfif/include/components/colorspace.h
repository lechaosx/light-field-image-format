#pragma once

#include <cmath>
#include <cstdint>

namespace YCbCr {

  inline float RGBToY(uint16_t R, uint16_t G, uint16_t B) {
    return (0.299 * R) + (0.587 * G) + (0.114 * B);
  }

  inline float RGBToCb(uint16_t R, uint16_t G, uint16_t B) {
    return - (0.168736 * R) - (0.331264 * G) + (0.5 * B);
  }

  inline float RGBToCr(uint16_t R, uint16_t G, uint16_t B) {
    return (0.5 * R) - (0.418688 * G) - (0.081312 * B);
  }

  inline float YCbCrToR(float Y, float, float Cr) {
    return Y + 1.402 * Cr;
  }

  inline float YCbCrToG(float Y, float Cb, float Cr) {
    return Y - 0.344136 * Cb - 0.714136 * Cr;
  }

  inline float YCbCrToB(float Y, float Cb, float) {
    return Y + 1.772 * Cb;
  }
}

namespace YCoCg {

  inline float RGBToY(uint16_t R, uint16_t G, uint16_t B) {
    return (0.25 * R) + (0.5 * G) + (0.25 * B);
  }

  inline float RGBToCo(uint16_t R, uint16_t, uint16_t B) {
    return (0.5 * R) - (0.5 * B);
  }

  inline float RGBToCg(uint16_t R, uint16_t G, uint16_t B) {
    return - (0.25 * R) + (0.5 * G) - (0.25 * B);
  }

  inline float YCoCgToR(float Y, float Co, float Cg) {
    return Y + Co - Cg;
  }

  inline float YCoCgToG(float Y, float, float Cg) {
    return Y + Cg;
  }

  inline float YCoCgToB(float Y, float Co, float Cg) {
    return Y - Co - Cg;
  }
}
