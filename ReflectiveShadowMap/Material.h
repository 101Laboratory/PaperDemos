#pragma once
#include <DirectXMath.h>


struct Diffuse {
  DirectX::XMFLOAT3 albedo;
  float reflectance = 0.8;
};