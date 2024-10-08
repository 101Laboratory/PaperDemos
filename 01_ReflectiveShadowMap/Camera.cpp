#include "Camera.h"

#include "MathUtils.h"

using namespace DirectX;

void Camera::FocusAtPoint(DirectX::XMFLOAT3 p) {
  auto f = ToXMVector(p);
  auto pos = ToXMVector(worldPosition);

  if (XMVector3NearEqual(f, pos, XMVectorReplicate(1e-6)))
    return;

  auto dir = XMVector3Normalize(f - pos);
  lookTo = ToXMFloat3(dir);
}

XMVECTOR CameraAxisX(const Camera* cam) {
  auto z = CameraAxisZ(cam);
  auto up = ToXMVector(cam->up);
  return XMVector3Normalize(XMVector3Cross(up, z));
}

XMVECTOR CameraAxisY(const Camera* cam) {
  auto z = CameraAxisZ(cam);
  auto x = CameraAxisX(cam);
  return XMVector3Cross(z, x);
}

XMVECTOR CameraAxisZ(const Camera* cam) {
  return XMVector3Normalize(ToXMVector(cam->lookTo));
}

XMMATRIX MatView(const Camera* cam) {
  auto pos = ToXMVector(cam->worldPosition);
  auto dir = XMVector3Normalize(ToXMVector(cam->lookTo));
  auto up = XMVector3Normalize(ToXMVector(cam->up));
  return XMMatrixLookToLH(pos, dir, up);
}

XMMATRIX MatProj(const Camera* cam) {
  auto vfov = XMConvertToRadians(cam->vFovDeg);
  auto r = cam->aspectRatio;
  auto zNear = cam->zNear;
  auto zFar = cam->zFar;
  return XMMatrixPerspectiveFovLH(vfov, r, zNear, zFar);
}
