#pragma once
#include <DirectXMath.h>

#include "Color.h"

struct DirectionalLight {
  DirectX::XMFLOAT3 direction = {0.0f, -1.0f, 0.0f};
  RgbaColorF color = color::White();
};
