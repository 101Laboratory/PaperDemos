#include "Win32InputHandler.h"

#include <windowsx.h>

bool IsKeyDown(int keyCode) {
  return GetKeyState(keyCode) & 0x8000;
}

void Win32InputHandler::HandleInput(UINT msg, WPARAM wParam, LPARAM lParam) {
  if (!IsValid())
    return;

  switch (msg) {
    case WM_MOUSEMOVE:
      OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return;

    case WM_LBUTTONDOWN:
      OnLMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return;

    case WM_LBUTTONUP:
      OnLMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return;

    case WM_RBUTTONDOWN:
      OnRMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return;

    case WM_RBUTTONUP:
      OnRMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return;

    case WM_KEYDOWN:
      OnKeyDown(static_cast<int>(wParam));
      return;

    case WM_KEYUP:
      OnKeyUp(static_cast<int>(wParam));
      return;

    case WM_MOUSEWHEEL:
      OnMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam));
      return;

    default:
      return;
  }
}