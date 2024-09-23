#pragma once
#include <DirectXMath.h>


#include "Win32InputHandler.h"

class Camera;

class OrbitCameraInputHandler final : public Win32InputHandler {
public:
  explicit OrbitCameraInputHandler(Camera* camera) : camera_{camera} {}

  OrbitCameraInputHandler(Camera* camera, DirectX::XMFLOAT3 focus, float minR)
      : camera_{camera},
        focus_{focus},
        minR_{minR} {}

  bool IsValid() override;
  void OnMouseMove(int x, int y) override;
  void OnLMouseDown(int x, int y) override;
  void OnLMouseUp(int x, int y) override;
  void OnRMouseDown(int x, int y) override;
  void OnRMouseUp(int x, int y) override;
  void OnKeyDown(int key) override;
  void OnKeyUp(int key) override;
  void OnMouseScroll(short delta) override;
  void ProcessKey() override;

private:
  Camera* camera_ = nullptr;

  int prevX_ = 0;
  int prevY_ = 0;

  bool lMouseDown_ = false;
  bool rMouseDown_ = false;

  DirectX::XMFLOAT3 focus_ = {0.f, 0.f, 0.f};
  float minR_ = 0.1f;
};

class FpsCameraInputHandler final : public Win32InputHandler {
public:
  explicit FpsCameraInputHandler(Camera* camera, float moveSpeed = 1.f, float rotateSpeed = 0.5f)
      : camera_{camera},
        deltaDist_{moveSpeed},
        deltaAngle_{rotateSpeed} {}

  bool IsValid() override;
  void OnMouseMove(int x, int y) override;
  void OnLMouseDown(int x, int y) override;
  void OnLMouseUp(int x, int y) override;
  void OnRMouseDown(int x, int y) override;
  void OnRMouseUp(int x, int y) override;
  void OnKeyDown(int key) override;
  void OnKeyUp(int key) override;
  void OnMouseScroll(short delta) override;
  void ProcessKey() override;

private:
  Camera* camera_ = nullptr;

  int prevX_ = 0;
  int prevY_ = 0;

  bool lMouseDown_ = false;
  bool rMouseDown_ = false;

  float deltaDist_ = 1.f;
  float deltaAngle_ = 0.5f;
};