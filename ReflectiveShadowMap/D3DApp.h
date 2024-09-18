#pragma once

#include <d3d12.h>
#include <d3dx12_core.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <memory>

#include "camera.h"
#include "Timer.h"

struct CBufferObject {
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 proj;
  DirectX::XMFLOAT4X4 translate;
  DirectX::XMFLOAT4 red;
  float timeElapsed;
};

struct Vertex {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT4 color;
};

class D3DApp {
public:
  static constexpr int s_renderTargetCount = 2;

  D3DApp(std::wstring name, int viewportWidth, int viewportHeight);

  void Initialize(HWND window);

  void Update();

  void Render();

  void Destroy();

  bool IsRunning() const;

  void StartRunning();

  void PauseRunning();

  Camera* GetCamera() const;

  float GetViewportWidth() const;

  float GetViewportHeight() const;

  std::wstring GetAppName() const { return name_; }

  float GetFramesPerSecond() const { return framesPerSecond_; }

private:
  std::wstring name_;

  bool isRunning_ = false;
  
  Timer<Milliseconds> timer_;
  float framesPerSecond_ = 0.0f;
  float totalTimeElapsed_ = 0.0f;
  float accumulatedFrameTime_ = 0.0f;
  size_t accumulatedFrameCount_ = 0;
  Timer<Seconds>::TimePoint prevFrameTimePoint_ = {};

  CD3DX12_VIEWPORT viewport_;
  CD3DX12_RECT scissorRect_;

  std::unique_ptr<Camera> camera_ = {};

  Microsoft::WRL::ComPtr<ID3D12Device> device_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_;
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, s_renderTargetCount> renderTargets_;
  int frameIndex_ = -1;

  UINT rtvDescriptorSize_ = 0;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvHeap_;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

  Microsoft::WRL::ComPtr<ID3D12Resource> cBuffer_;
  UINT8* cBufferBegin_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  UINT64 nextFenceValue_ = 0;

  void PopulateCommandList();
  void WaitForPreviousFrame();
  void Transition(ID3D12Resource* resource,
                  D3D12_RESOURCE_STATES before,
                  D3D12_RESOURCE_STATES after);

  void FrameStatics();
  void UpdateScene();
};
