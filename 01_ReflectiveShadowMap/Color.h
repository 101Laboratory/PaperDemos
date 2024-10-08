#pragma once

struct RgbaColorI;

struct RgbaColorF {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;

  explicit operator RgbaColorI() const;
};

struct RgbaColorI {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;
  unsigned char a = 0;

  explicit operator RgbaColorF() const;
};

namespace color {
inline constexpr RgbaColorF Black() {
  return {0.0f, 0.0f, 0.0f, 1.0f};
}

inline constexpr RgbaColorF White() {
  return {1.0f, 1.0f, 1.0f, 1.0f};
}

inline constexpr RgbaColorF Red() {
  return {1.0f, 0.0f, 0.0f, 1.0f};
}

inline constexpr RgbaColorF Green() {
  return {0.0f, 1.0f, 0.0f, 1.0f};
}

inline constexpr RgbaColorF Blue() {
  return {0.0f, 0.0f, 1.0f, 1.0f};
}
}  // namespace color