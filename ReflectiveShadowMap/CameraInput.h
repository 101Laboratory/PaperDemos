#pragma once
#include "Win32InputHandler.h"

class Camera;

class OrbitCameraInputHandler final : public Win32InputHandler {
public:
  OrbitCameraInputHandler(Camera* camera) : camera_{camera} {}

  bool IsValid() override;
  void OnMouseMove(int x, int y) override;
  void OnLMouseDown(int x, int y) override;
  void OnLMouseUp(int x, int y) override;
  void OnRMouseDown(int x, int y) override;
  void OnRMouseUp(int x, int y) override;
  void OnKeyDown(int key) override;
  void OnKeyUp(int key) override;
  void OnMouseScroll(short delta) override;

private:
  Camera* camera_ = nullptr;

  int prevX_ = 0;
  int prevY_ = 0;

  bool lMouseDown_ = false;
  bool rMouseDown_ = false;
};
