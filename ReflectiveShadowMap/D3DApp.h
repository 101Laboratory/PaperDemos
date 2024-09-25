#pragma once

#include <d3d12.h>
#include <d3dx12_core.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <memory>

#include "camera.h"
#include "ConstantBuffer.h"
#include "DefaultBuffer.h"
#include "DescriptorHeap.h"
#include "Material.h"
#include "Model.h"
#include "Timer.h"

struct PassConstant {
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 proj;
  float timeElapsed;
};

struct ModelConstant {
  DirectX::XMFLOAT4X4 world;  // Model-to-world transform
  DirectX::XMFLOAT3 color;
};

struct RenderItem {
  DirectX::XMFLOAT4X4 world = Float4x4Identity();
  size_t startIndexLocation = 0;
  size_t indexCount = 0;
  size_t baseVertexLocation = 0;
  size_t modelCBufferIndex = 0;
  CD3DX12_GPU_DESCRIPTOR_HANDLE modelCbv;
  std::shared_ptr<Diffuse> material;
};

class D3DApp {
public:
  static constexpr int s_renderTargetCount = 2;

  D3DApp(std::wstring name, int viewportWidth, int viewportHeight);

  void Initialize(HWND window);

  void InitializeScene();

  void Update();
  
  void ExecuteCommandList() const;

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

  std::unique_ptr<DescriptorHeap> rtvHeap_;
  
  std::unique_ptr<DescriptorHeap> dsvHeap_;

  std::unique_ptr<DescriptorHeap> cbvHeap_;

  Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;
  DXGI_FORMAT depthBufferFormat_ = DXGI_FORMAT_D16_UNORM;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;


  std::unique_ptr<DefaultBuffer> vBuffer_;
  D3D12_VERTEX_BUFFER_VIEW vbv_{};

  std::unique_ptr<DefaultBuffer> iBuffer_;
  D3D12_INDEX_BUFFER_VIEW ibv_;

  std::vector<RenderItem> renderItems_;

  std::unique_ptr<ConstantBuffer<PassConstant>> passCBuffer_;
  std::unique_ptr<ConstantBuffer<ModelConstant>> modelCBuffer_;

  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  UINT64 nextFenceValue_ = 0;

  void PopulateCommandList();
  void WaitForPreviousFrame();
  void Transition(ID3D12Resource* resource,
                  D3D12_RESOURCE_STATES before,
                  D3D12_RESOURCE_STATES after);

  void FrameStatics();
  void UpdateScene();
  void DrawAllRenderItems();
};
