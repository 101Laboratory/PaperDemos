#include "FpsCamera.h"

#include "MathUtils.h"

using namespace DirectX;

void CameraWorldOffset(Camera* cam, XMFLOAT3 offset) {
  auto pos = ToXMVector(cam->GetWorldPosition());
  auto offsetV = ToXMVector(offset);
  auto newPos = ToXMFloat3(pos + offsetV);
  cam->SetWorldPosition(newPos);
}

void CameraMoveForward(Camera* cam, float d) {
  XMVECTOR dir = CameraAxisZ(cam);
  CameraWorldOffset(cam, ToXMFloat3(d * dir));
  // TODO: avoid pos and lookat equal
}

void CameraMoveRight(Camera* cam, float d) {
  XMVECTOR dir = CameraAxisX(cam);
  CameraWorldOffset(cam, ToXMFloat3(d * dir));
}

void CameraMoveUp(Camera* cam, float d) {
  XMVECTOR dir = CameraAxisY(cam);
  CameraWorldOffset(cam, ToXMFloat3(d * dir));
}

void CameraLookRotate(Camera* cam, XMVECTOR q) {
  XMVECTOR z = CameraAxisZ(cam);
  auto z1 = XMVector3Rotate(z, q);
  auto lookAt = ToXMVector(cam->GetWorldPosition()) + z1;
  cam->SetLookTo(ToXMFloat3(lookAt));
}

void CameraLookRight(Camera* cam, float deg) {
  XMVECTOR axis = CameraAxisY(cam);
  auto q = XMQuaternionRotationAxis(axis, XMConvertToRadians(deg));
  CameraLookRotate(cam, q);
}

void CameraLookUp(Camera* cam, float deg) {
  XMVECTOR axis = CameraAxisX(cam);
  auto q = XMQuaternionRotationAxis(axis, XMConvertToRadians(-deg));
  CameraLookRotate(cam, q);
}