#pragma once
#include <DirectXMath.h>

#include "MathUtils.h"

/**
 * Light up a volume of rectangular cuboid in 3D space.
 * The directional light starts from "pos" and going in direction of "dir".
 * The cuboid is aligned with "dir". The two sides perpendicular to "dir" have size "width x height"
 * and their distance is "affectedDepth".
 */
struct DirectionalLight {
  DirectionalLight(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, DirectX::XMFLOAT3 color,
                   float width, float height, float affectedDepth)
      : pos(pos),
        dir(dir),
        color(color),
        width(width),
        height(height),
        affectedDepth(affectedDepth) {}

  DirectX::XMFLOAT3 pos = {0.f, 0.f, 0.f};
  DirectX::XMFLOAT3 dir = {0.0f, -1.0f, 0.0f};
  DirectX::XMFLOAT3 color = {1.f, 1.f, 1.f};
  float width = 10.f;
  float height = 10.f;
  float affectedDepth = 100.f;
  float epsilon = 0.01f;
};

DirectionalLight MakeSceneDefaultDirectionalLight();

// World to view matrix of light.
DirectX::XMMATRIX MatLightView(const DirectionalLight* light);

// Orthogonal projection matrix of light.
// near = epsilon
// far = epsilon + affectedDepth
DirectX::XMMATRIX MatLightOrtho(const DirectionalLight* light);