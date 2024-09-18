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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
  try {
    Win32Window window(hInstance, hPrevInstance, pCmdLine, nCmdShow);
    D3DApp app{L"RSM Demo", 1280, 720};

    auto cameraInputHandler = std::make_unique<OrbitCameraInputHandler>(app.GetCamera());
    window.RegisterInputHandler(std::move(cameraInputHandler));

    window.Show();
    window.RunD3DApp(&app);

    while (app.IsRunning()) {
      PollEvent();
      app.Update();
      app.Render();

      auto title = app.GetAppName();
      auto fps = std::to_wstring(static_cast<int>(app.GetFramesPerSecond()));
      title += L"\t" + fps + L" fps";
      SetWindowText(window.GetHandle(), title.c_str());
    }

    app.Destroy();

  } catch (const std::runtime_error& e) {
    std::string what = e.what();
    std::wstring wWhat = {what.begin(), what.end()};
    OutputDebugString(wWhat.c_str());
    std::cout << e.what() << "\n";
  }
}
