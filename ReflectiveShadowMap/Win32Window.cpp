#ifndef UNICODE
#define UNICODE
#endif

#include "Win32Window.h"

#include <stdexcept>

#include "D3DApp.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

  switch (msg) {
    case WM_CREATE: {
      LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
      SetWindowLongPtr(hWnd,
                       GWLP_USERDATA,
                       reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
      return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
      window->ForwardInput(msg, wParam, lParam);
      return 0;

    case WM_DESTROY:
      PostQuitMessage(0);
      window->StopD3DApp();
      return 0;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

Win32Window::Win32Window(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
    : hInstance_(hInstance),
      hPrevInstance_(hPrevInstance),
      pCmdLine_(pCmdLine),
      nCmdShow_(nCmdShow) {
  WNDCLASSEX windowClass{};
  windowClass.cbSize = sizeof(decltype(windowClass));
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = hInstance;
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.lpszClassName = L"Win32Window";
  RegisterClassEx(&windowClass);

  hwnd_ = CreateWindow(windowClass.lpszClassName,
                       L"Test demo",
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       nullptr,
                       nullptr,
                       hInstance,
                       this);
  if (!hwnd_)
    throw std::runtime_error{"failed to create window"};
}

void Win32Window::RunD3DApp(D3DApp* app) {
  app_ = app;

  auto w = static_cast<LONG>(app->GetViewportWidth());
  auto h = static_cast<LONG>(app->GetViewportHeight());
  RECT clientRect = {0, 0, w, h};
  AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);

  SetWindowPos(hwnd_,
               HWND_TOP,
               0,
               0,
               clientRect.right - clientRect.left,
               clientRect.bottom - clientRect.top,
               SWP_NOMOVE);

  app->Initialize(hwnd_);
  app->StartRunning();
  isRunning_ = true;
}

HWND Win32Window::GetHandle() const {
  return hwnd_;
}

void Win32Window::Show() const {
  ShowWindow(hwnd_, nCmdShow_);
}

void Win32Window::RegisterInputHandler(std::unique_ptr<Win32InputHandler> handler) {
  inputHandlers_.push_back(std::move(handler));
}

void Win32Window::ForwardInput(UINT msg, WPARAM wParam, LPARAM lParam) {
  auto it = std::begin(inputHandlers_);
  while (it != std::end(inputHandlers_)) {
    if (!(*it)->IsValid()) {
      it = inputHandlers_.erase(it);
    } else {
      (*it++)->HandleInput(msg, wParam, lParam);
    }
  }
}

void Win32Window::StopD3DApp() {
  if (!app_)
    return;

  app_->Destroy();
  isRunning_ = false;
}