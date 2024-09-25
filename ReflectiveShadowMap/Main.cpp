#include <iostream>
#include <stdexcept>
#include <string>

#include "CameraInput.h"
#include "D3DApp.h"
#include "Win32Window.h"

void PollEvent() {
  MSG msg{};
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void DisplayFramesPerSecond(const Win32Window& window, const D3DApp& app) {
  auto title = app.GetAppName();
  auto fps = std::to_wstring(app.GetFramesPerSecond());
  title += L"\t" + fps + L" fps";
  SetWindowText(window.GetHandle(), title.c_str());
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
  try {
    Win32Window window(hInstance, hPrevInstance, pCmdLine, nCmdShow);
    D3DApp app{L"RSM Demo", 1280, 720};

    auto orbitCameraInput = std::make_unique<OrbitCameraInputHandler>(
        app.GetCamera(),
        DirectX::XMFLOAT3{0.f, 2.f, 0.f},
        0.1f);
    auto fpsCameraInput = std::make_unique<FpsCameraInputHandler>(app.GetCamera(), 0.5f, 0.25f);

    window.RegisterInputHandler(std::move(orbitCameraInput));
    // window.RegisterInputHandler(std::move(fpsCameraInput));

    window.Show();
    window.RunD3DApp(&app);

    while (window.IsRunning()) {
      PollEvent();
      app.Update();
      app.Render();

      DisplayFramesPerSecond(window, app);
    }

  } catch (const std::runtime_error& e) {
    std::string what = e.what();
    std::wstring wWhat = {what.begin(), what.end()};
    OutputDebugString(wWhat.c_str());
    std::cout << e.what() << "\n";
  }
}
