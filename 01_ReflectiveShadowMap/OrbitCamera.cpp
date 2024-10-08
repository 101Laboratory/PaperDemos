#include "OrbitCamera.h"

#include "MathUtils.h"

using namespace DirectX;

void CameraSetOrbitWithOffset(Camera* cam,
                              DirectX::XMFLOAT3 focus,
                              float deltaR,
                              float deltaPhiDeg,
                              float deltaThetaDeg,
                              float minR) {
  auto p = ToXMVector(cam->worldPosition);
  auto f = ToXMVector(focus);

  const auto& [dx0, dy0, dz0] = ToXMFloat3(p - f);
  auto [r, phi, theta] = ToSpherical(dx0, dy0, dz0);
  float deltaPhi = XMConvertToRadians(deltaPhiDeg);
  float deltaTheta = XMConvertToRadians(deltaThetaDeg);

  r = std::max(minR, r + deltaR);
  phi = Wrap(phi + deltaPhi, 0.0f, XM_2PI);
  theta = Clamp(theta + deltaTheta, 0.1f, XM_PI - 0.1f);

  const auto& [dx, dy, dz] = ToCartesian(r, phi, theta);
  auto pos = f + XMVectorSet(dx, dy, dz, 0.0f);
  cam->worldPosition = ToXMFloat3(pos);
  cam->FocusAtPoint(focus);
}