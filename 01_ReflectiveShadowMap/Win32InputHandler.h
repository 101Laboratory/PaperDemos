#pragma once

#include <Windows.h>

bool IsKeyDown(int keyCode);

class Win32InputHandler {
public:
  Win32InputHandler() = default;
  
  Win32InputHandler(const Win32InputHandler& other) = delete;
  Win32InputHandler(Win32InputHandler&& other) noexcept = delete;
  Win32InputHandler& operator=(const Win32InputHandler& other) = delete;
  Win32InputHandler& operator=(Win32InputHandler&& other) noexcept = delete;

  virtual ~Win32InputHandler() = default;

  virtual bool IsValid() = 0;

  void HandleInput(UINT msg, WPARAM wParam, LPARAM lParam);

  virtual void OnMouseMove(int x, int y) = 0;

  virtual void OnLMouseDown(int x, int y) = 0;
  virtual void OnLMouseUp(int x, int y) = 0;

  virtual void OnRMouseDown(int x, int y) = 0;
  virtual void OnRMouseUp(int x, int y) = 0;

  virtual void OnKeyDown(int key) = 0;
  virtual void OnKeyUp(int key) = 0;
  virtual void ProcessKey() = 0;

  virtual void OnMouseScroll(short delta) = 0;

};
