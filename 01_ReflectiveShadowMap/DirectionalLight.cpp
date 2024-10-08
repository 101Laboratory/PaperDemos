#include "DirectionalLight.h"

using namespace DirectX;

DirectionalLight MakeSceneDefaultDirectionalLight() {
  auto pos = XMFLOAT3{6.f, 6.f, -6.f};
  auto dir = XMFLOAT3{-1.f, -1.f, 1.f};
  auto color = XMFLOAT3{1.f, 1.f, 1.f};
  float w = 15.f;
  float h = 15.f;
  float d = 50.f;
  return {pos, dir, color, w, h, d};
}

XMMATRIX MatLightView(const DirectionalLight* light) {
  auto pos = ToXMVector(light->pos);
  auto dir = XMVector3Normalize(ToXMVector(light->dir));
  auto up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
  return XMMatrixLookToLH(pos, dir, up);
}

// pos       near                         far
//  |         |                            |
//  | epsilon |        affectedDepth       |
//  |---------|----------------------------|
//  |         |                            |
XMMATRIX MatLightOrtho(const DirectionalLight* light) {
  auto width = light->width;
  auto height = light->height;
  auto near = light->epsilon;
  auto far = light->epsilon + light->affectedDepth;
  return XMMatrixOrthographicLH(width, height, near, far);
}