#pragma once

#include <DirectXMath.h>

class Camera {
public:
  Camera() = default;

  Camera(DirectX::XMFLOAT3 worldPosition,
         DirectX::XMFLOAT3 lookTo,
         float aspectRatio,
         float vFovDeg,
         float zNear,
         float zFar)
      : worldPosition_(worldPosition),
        lookTo_(lookTo),
        aspectRatio_(aspectRatio),
        vFovDeg_(vFovDeg),
        zNear_(zNear),
        zFar_(zFar) {}

  DirectX::XMFLOAT3 GetWorldPosition() const;

  void SetWorldPosition(const DirectX::XMFLOAT3& worldPosition);

  DirectX::XMFLOAT3 GetLookTo() const;

  void SetLookTo(const DirectX::XMFLOAT3& lookTo);

  void LookAt(DirectX::XMFLOAT3 focusPoint);

  DirectX::XMFLOAT3 GetUpVector() const { return up_; }

  void SetUpVector(const DirectX::XMFLOAT3& up) { up_ = up; }

  float GetAspectRatio() const;

  void SetAspectRatio(const float aspectRatio);

  float GetVFovDeg() const;

  void SetVFov(const float vFov);

  float GetZNear() const;

  void SetZNear(const float zNear);

  float GetZFar() const;

  void SetZFar(const float zFar);

private:
  DirectX::XMFLOAT3 worldPosition_ = {0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT3 lookTo_ = {0.0f, 0.0f, 1.0f};
  DirectX::XMFLOAT3 up_ = {0.0f, 1.0f, 0.0f};

private:
  float aspectRatio_ = 1.0f;
  float vFovDeg_ = 45.0f;
  float zNear_ = 0.1f;
  float zFar_ = 1000.0f;
};

DirectX::XMVECTOR CameraAxisX(const Camera* cam);

DirectX::XMVECTOR CameraAxisY(const Camera* cam);

DirectX::XMVECTOR CameraAxisZ(const Camera* cam);

DirectX::XMMATRIX MatView(const Camera* cam);

DirectX::XMMATRIX MatProj(const Camera* cam);
