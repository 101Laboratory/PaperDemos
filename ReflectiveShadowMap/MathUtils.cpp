#include "MathUtils.h"
// ReSharper disable CppInconsistentNaming

#include <cmath>

using namespace DirectX;

XMVECTOR ToXMVector(XMFLOAT3 v) {
  XMFLOAT4 v4 = {v.x, v.y, v.z, 0.0f};
  return XMLoadFloat4(&v4);
}

XMVECTOR ToXMVector(XMFLOAT4 v) {
  return XMLoadFloat4(&v);
}

XMFLOAT3 ToXMFloat3(FXMVECTOR v) {
  XMFLOAT3 ret;
  XMStoreFloat3(&ret, v);
  return ret;
}

XMFLOAT4 ToXMFloat4(FXMVECTOR v) {
  XMFLOAT4 ret;
  XMStoreFloat4(&ret, v);
  return ret;
}

double Clamp(double x, double lo, double hi) {
  return x > lo ? (x < hi ? x : hi) : lo;
}

int Clamp(int x, int lo, int hi) {
  return x > lo ? (x < hi ? x : hi) : lo;
}

double Wrap(double x, double lo, double hi) {
  if (hi < lo) {
    return Wrap(x, hi, lo);
  }
  return (x > 0 ? lo : hi) + std::fmod(x, hi - lo);
}

std::tuple<float, float, float> ToCartesian(float r, float phi, float theta) {
  float x = r * std::sin(theta) * std::cos(phi);
  float y = r * std::cos(theta);
  float z = r * std::sin(theta) * std::sin(phi);
  return {x, y, z};
}

std::tuple<float, float, float> ToSpherical(float x, float y, float z) {
  float r = std::sqrt(x * x + y * y + z * z);
  float phi = std::atan2(z, x);
  float theta = std::acos(y / r);
  return {r, phi, theta};
}

XMFLOAT4X4 Float4x4Identity() {
  // clang-format off
  return {
      1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f
  };
  // clang-format on
}
