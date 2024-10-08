#include "CameraInput.h"

#include "Camera.h"
#include "FpsCamera.h"
#include "OrbitCamera.h"

bool OrbitCameraInputHandler::IsValid() {
  return camera_;
}

void OrbitCameraInputHandler::OnMouseMove(int x, int y) {
  auto dx = static_cast<float>(x - prevX_);
  auto dy = static_cast<float>(y - prevY_);

  if (lMouseDown_) {
    CameraSetOrbitWithOffset(camera_, focus_, 0.0f, -dx, -dy, minR_);
  }

  prevX_ = x;
  prevY_ = y;
}

void OrbitCameraInputHandler::OnLMouseDown(int x, int y) {
  lMouseDown_ = true;
}

void OrbitCameraInputHandler::OnLMouseUp(int x, int y) {
  lMouseDown_ = false;
}

void OrbitCameraInputHandler::OnRMouseDown(int x, int y) {
  rMouseDown_ = true;
}

void OrbitCameraInputHandler::OnRMouseUp(int x, int y) {
  rMouseDown_ = false;
}

void OrbitCameraInputHandler::OnKeyDown(int key) {}

void OrbitCameraInputHandler::OnKeyUp(int key) {}

void OrbitCameraInputHandler::OnMouseScroll(short delta) {
  CameraSetOrbitWithOffset(camera_,
                           focus_,
                           0.25f * static_cast<float>(delta) / (-120.0f),
                           0.0f,
                           0.0f,
                           minR_);
}

void OrbitCameraInputHandler::ProcessKey() {}

bool FpsCameraInputHandler::IsValid() {
  return camera_;
}

void FpsCameraInputHandler::OnMouseMove(int x, int y) {
  auto dx = static_cast<float>(x - prevX_);
  auto dy = static_cast<float>(y - prevY_);

  CameraLookUp(camera_, -dy);
  CameraLookRight(camera_, dx);

  prevX_ = x;
  prevY_ = y;
}

void FpsCameraInputHandler::OnLMouseDown(int x, int y) {}

void FpsCameraInputHandler::OnLMouseUp(int x, int y) {}

void FpsCameraInputHandler::OnRMouseDown(int x, int y) {}

void FpsCameraInputHandler::OnRMouseUp(int x, int y) {}

void FpsCameraInputHandler::OnKeyDown(int key) {}

void FpsCameraInputHandler::OnKeyUp(int key) {}

void FpsCameraInputHandler::OnMouseScroll(short delta) {}

void FpsCameraInputHandler::ProcessKey() {
  float forwardDist = IsKeyDown('W') ? 1.f : (IsKeyDown('S') ? -1.f : 0.f);
  CameraMoveForward(camera_, forwardDist * deltaDist_);

  float rightDist = IsKeyDown('D') ? 1.f : (IsKeyDown('A') ? -1.f : 0.f);
  CameraMoveRight(camera_, rightDist * deltaDist_);
}