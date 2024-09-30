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
#include "DirectionalLight.h"
#include "Material.h"
#include "Model.h"
#include "Timer.h"

struct PassConstant {
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 invView;
  DirectX::XMFLOAT4X4 proj;
  DirectX::XMFLOAT4X4 invProj;
  DirectX::XMFLOAT4X4 lightView;  // View matrix of light source
  DirectX::XMFLOAT4X4 invLightView;
  DirectX::XMFLOAT4X4 lightOrtho;  // Orthogonal projection matrix of light source
  DirectX::XMFLOAT4X4 invLightOrtho;
  DirectX::XMFLOAT3 lightFlux;  // Flux of light source
  float lightZNear;
  DirectX::XMFLOAT3 lightDirection;
  float lightZFar;
  DirectX::XMFLOAT3 lightPos;
  float width;    // Viewport width
  float height;   // Viewport height
  float rsmSize;  // Reflective shadow map size
  float timeElapsed;
};

struct ModelConstant {
  DirectX::XMFLOAT4X4 model;  // Model-to-model transform
  DirectX::XMFLOAT4X4 invModel;
  DirectX::XMFLOAT3 color;
};

struct RenderItem {
  DirectX::XMFLOAT4X4 model = Float4x4Identity();  // Model-to-model transform
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

  // Timer and game frame statistics
  Timer<Milliseconds> timer_;
  float framesPerSecond_ = 0.0f;
  float totalTimeElapsed_ = 0.0f;
  float accumulatedFrameTime_ = 0.0f;
  size_t accumulatedFrameCount_ = 0;
  Timer<Seconds>::TimePoint prevFrameTimePoint_ = {};


  std::unique_ptr<Camera> camera_ = {};

  // D3D objects
  Microsoft::WRL::ComPtr<ID3D12Device> device_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

  // Render target related
  CD3DX12_VIEWPORT viewport_;
  CD3DX12_RECT scissorRect_;

  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_;
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, s_renderTargetCount> renderTargets_;
  int frameIndex_ = -1;

  // Render target views
  // [0]: one of swap chain buffers
  // [1]: one of swap chain buffers
  // [2]: RSM depth texture (write)
  // [3]: RSM normal texture (write)
  // [4]: RSM flux texture (write)
  // [5]: RSM world pos texture (write)
  std::unique_ptr<DescriptorHeap> rtvHeap_;
  int rsmRtvStartIndex_ = 2;


  // Depth stencil views
  // [0]: depth buffer for final image
  // [1]: depth buffer for RSM
  std::unique_ptr<DescriptorHeap> dsvHeap_;
  int rsmDsvStartIndex_ = 1;

  Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;
  DXGI_FORMAT depthBufferFormat_ = DXGI_FORMAT_D16_UNORM;

  // Pipeline related
  //   Root signature:
  //   param[0]: descriptor table (1x cbv), register(b0)
  //   param[1]: descriptor table (1x cbv), register(b1)
  //   param[2]: descriptor table (4x srv), register(t0-t3)
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStatePass1_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStatePass2_;

  // Scene and constants
  std::unique_ptr<DefaultBuffer> vBuffer_;
  D3D12_VERTEX_BUFFER_VIEW vbv_{};

  std::unique_ptr<DefaultBuffer> iBuffer_;
  D3D12_INDEX_BUFFER_VIEW ibv_;

  // Constant buffer views and shader resource views
  // [0] cbv: pass constant
  // [1-4] cbv: model constants (bunny, RectXZ, RectXY, RectYZ)
  // [5] srv: RSM depth texture(read)
  // [6] srv: RSM normal texture (read)
  // [7] srv: RSM flux texture (read)
  // [8] srv: RSM world pos texture (read)
  std::unique_ptr<DescriptorHeap> cbvSrvHeap_;
  int rsmSrvStartIndex_ = 5;

  std::unique_ptr<ConstantBuffer<PassConstant>> passCBuffer_;
  std::unique_ptr<ConstantBuffer<ModelConstant>> modelCBuffer_;

  std::vector<RenderItem> renderItems_;

  DirectionalLight directionalLight_ = MakeSceneDefaultDirectionalLight();

  // Reflective shadow map
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> rsmTex_;
  Microsoft::WRL::ComPtr<ID3D12Resource> shadowDepthBuffer_;
  int texCount_ = 4;
  int texSize_ = 512;
  CD3DX12_VIEWPORT rsmViewport_;
  CD3DX12_RECT rsmRect_;


  // Synchronization
  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  UINT64 nextFenceValue_ = 0;

  void PopulateCommandListFirstPass();
  void PopulateCommandListSecondPass();

  void WaitForGpuCompletion();
  void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                  D3D12_RESOURCE_STATES after);

  void FrameStatistics();
  void UpdateScene();
  void DrawAllRenderItems();
};
