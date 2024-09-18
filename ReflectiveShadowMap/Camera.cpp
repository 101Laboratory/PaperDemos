#include "Camera.h"

#include "MathUtils.h"

using namespace DirectX;

XMFLOAT3 Camera::GetWorldPosition() const {
  return worldPosition_;
}

void Camera::SetWorldPosition(const XMFLOAT3& worldPosition) {
  worldPosition_ = worldPosition;
}

XMFLOAT3 Camera::GetLookTo() const {
  return lookTo_;
}

void Camera::SetLookTo(const XMFLOAT3& lookTo) {
  lookTo_ = lookTo;
}

void Camera::LookAt(DirectX::XMFLOAT3 focusPoint) {
  auto f = ToXMVector(focusPoint);
  auto p = ToXMVector(worldPosition_);

  if (XMVector3NearEqual(f, p, XMVectorReplicate(1e-6)))
    return;

  auto dir = XMVector3Normalize(f - p);
  lookTo_ = ToXMFloat3(dir);
}

float Camera::GetAspectRatio() const {
  return aspectRatio_;
}

void Camera::SetAspectRatio(const float aspectRatio) {
  aspectRatio_ = aspectRatio;
}

float Camera::GetVFovDeg() const {
  return vFovDeg_;
}

void Camera::SetVFov(const float vFov) {
  vFovDeg_ = vFov;
}

float Camera::GetZNear() const {
  return zNear_;
}

void Camera::SetZNear(const float zNear) {
  zNear_ = zNear;
}

float Camera::GetZFar() const {
  return zFar_;
}

void Camera::SetZFar(const float zFar) {
  zFar_ = zFar;
}

XMVECTOR CameraAxisX(const Camera* cam) {
  auto z = CameraAxisZ(cam);
  auto up = ToXMVector(cam->GetUpVector());
  return XMVector3Normalize(XMVector3Cross(up, z));
}

XMVECTOR CameraAxisY(const Camera* cam) {
  auto z = CameraAxisZ(cam);
  auto x = CameraAxisX(cam);
  return XMVector3Cross(z, x);
}

XMVECTOR CameraAxisZ(const Camera* cam) {
  return XMVector3Normalize(ToXMVector(cam->GetLookTo()));
}

XMMATRIX MatView(const Camera* cam) {
  auto pos = ToXMVector(cam->GetWorldPosition());
  auto dir = XMVector3Normalize(ToXMVector(cam->GetLookTo()));
  auto up = XMVector3Normalize(ToXMVector(cam->GetUpVector()));
  return XMMatrixLookToLH(pos, dir, up);
}

XMMATRIX MatProj(const Camera* cam) {
  auto vfov = XMConvertToRadians(cam->GetVFovDeg());
  auto r = cam->GetAspectRatio();
  auto zNear = cam->GetZNear();
  auto zFar = cam->GetZFar();
  return XMMatrixPerspectiveFovLH(vfov, r, zNear, zFar);
}
