#include "CameraInput.h"

#include "Camera.h"
#include "OrbitCamera.h"

bool OrbitCameraInputHandler::IsValid() {
  return camera_;
}

void OrbitCameraInputHandler::OnMouseMove(int x, int y) {
  auto dx = static_cast<float>(x - prevX_);
  auto dy = static_cast<float>(y - prevY_);

  if (lMouseDown_) {
    CameraSetOrbitWithOffset(camera_, {0.0f, 0.0f, 0.0f}, 0.0f, -dx, -dy);
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
                           {0.0f, 0.0f, 0.0f},
                           static_cast<float>(delta) / (-120.0f),
                           0.0f,
                           0.0f);
}