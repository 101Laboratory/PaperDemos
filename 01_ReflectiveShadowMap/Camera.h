#pragma once

#include <DirectXMath.h>

struct Camera {
  Camera(DirectX::XMFLOAT3 worldPosition,
         DirectX::XMFLOAT3 lookTo,
         float aspectRatio,
         float vFovDeg,
         float zNear,
         float zFar)
      : worldPosition(worldPosition),
        lookTo(lookTo),
        aspectRatio(aspectRatio),
        vFovDeg(vFovDeg),
        zNear(zNear),
        zFar(zFar) {}

  void FocusAtPoint(DirectX::XMFLOAT3 p);

  DirectX::XMFLOAT3 worldPosition = {0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT3 lookTo = {0.0f, 0.0f, 1.0f};
  DirectX::XMFLOAT3 up = {0.0f, 1.0f, 0.0f};

  float aspectRatio = 1.0f;
  float vFovDeg = 45.0f;
  float zNear = 0.1f;
  float zFar = 1000.0f;
};

DirectX::XMVECTOR CameraAxisX(const Camera* cam);

DirectX::XMVECTOR CameraAxisY(const Camera* cam);

DirectX::XMVECTOR CameraAxisZ(const Camera* cam);

DirectX::XMMATRIX MatView(const Camera* cam);

DirectX::XMMATRIX MatProj(const Camera* cam);
