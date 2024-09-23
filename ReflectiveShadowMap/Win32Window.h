#pragma once

#include <Windows.h>
#include <memory>
#include <vector>

#include "Win32InputHandler.h"

class D3DApp;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Win32Window {
public:
  Win32Window(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

  void RunD3DApp(D3DApp* app);

  HWND GetHandle() const;

  void Show() const;

  void RegisterInputHandler(std::unique_ptr<Win32InputHandler> handler);

  void ForwardInput(UINT msg, WPARAM wParam, LPARAM lParam);

  void StopD3DApp();

  bool IsRunning() const { return isRunning_; }

private:
  // ReSharper disable once CppInconsistentNaming
  // ReSharper disable once IdentifierTypo
  HWND hwnd_ = nullptr;
  HINSTANCE hInstance_;
  HINSTANCE hPrevInstance_;
  PWSTR pCmdLine_;
  int nCmdShow_;

  D3DApp* app_ = nullptr;
  bool isRunning_ = false;
  std::vector<std::unique_ptr<Win32InputHandler>> inputHandlers_;
};