#include "D3DApp.h"

#include "D3DUtils.h"

using DX::ThrowIfFailed;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

D3DApp::D3DApp(std::wstring name, int viewportWidth, int viewportHeight)
    : name_{std::move(name)},
      viewport_{0.0f, 0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)},
      scissorRect_{0, 0, static_cast<LONG>(viewportWidth), static_cast<LONG>(viewportHeight)} {
  camera_ = std::make_unique<Camera>(
      XMFLOAT3{0.0f, 0.0f, -5.0f},
      XMFLOAT3{0.0f, 0.0f, 1.0f},
      static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
      60.0f,
      1.0f,
      100.0f);
  // camera_->SetLookTo({0.25f, -0.25f * camera_->GetAspectRatio(), 0.0f});
}

void D3DApp::Initialize(HWND window) {
  UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
  {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
      debugController->EnableDebugLayer();
      dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
  }
#endif

  ComPtr<IDXGIFactory4> factory;
  ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(factory.GetAddressOf())));

  {
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    DX::GetHardwareAdapter(factory.Get(), hardwareAdapter.GetAddressOf(), false);

    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(),
                                    D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(device_.ReleaseAndGetAddressOf())));
  }
  {
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(
        device_->CreateCommandQueue(&queueDesc,
                                    IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = s_renderTargetCount;
    swapChainDesc.Width = viewport_.Width;
    swapChainDesc.Height = viewport_.Height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue_.Get(),
                                                  window,
                                                  &swapChainDesc,
                                                  nullptr,
                                                  nullptr,
                                                  swapChain.GetAddressOf()));
    ThrowIfFailed(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&swapChain_));
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
  }
  {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = s_renderTargetCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc,
                                                IID_PPV_ARGS(rtvHeap_.ReleaseAndGetAddressOf())));
  }
  rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  for (int i = 0; i < s_renderTargetCount; ++i) {
    auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE{rtvHeap_->GetCPUDescriptorHandleForHeapStart(),
                                                i,
                                                rtvDescriptorSize_};
    ThrowIfFailed(
        swapChain_->GetBuffer(i, IID_PPV_ARGS(renderTargets_[i].ReleaseAndGetAddressOf())));
    device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, handle);
  }
  {
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device_->CreateDescriptorHeap(&cbvHeapDesc,
                                                IID_PPV_ARGS(cbvHeap_.ReleaseAndGetAddressOf())));
  }

  ThrowIfFailed(
      device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf())));

  // Root signature
  {
    CD3DX12_DESCRIPTOR_RANGE range[1];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParameter[1];
    rootParameter[0].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init(_countof(rootParameter),
                           rootParameter,
                           0,
                           nullptr,
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              signature.GetAddressOf(),
                                              error.GetAddressOf()));
    ThrowIfFailed(device_->CreateRootSignature(0,
                                               signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(rootSignature_.GetAddressOf())));
  }

  // Pipeline state object and shaders
  {
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl",
                                     nullptr,
                                     nullptr,
                                     "VS",
                                     "vs_5_0",
                                     compileFlags,
                                     0,
                                     vertexShader.GetAddressOf(),
                                     nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl",
                                     nullptr,
                                     nullptr,
                                     "PS",
                                     "ps_5_0",
                                     compileFlags,
                                     0,
                                     pixelShader.GetAddressOf(),
                                     nullptr));

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {{"POSITION",
                                                    0,
                                                    DXGI_FORMAT_R32G32B32_FLOAT,
                                                    0,
                                                    0,
                                                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                    0},
                                                   {"COLOR",
                                                    0,
                                                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                    0,
                                                    12,
                                                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                    0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(pipelineState_.ReleaseAndGetAddressOf())));
  }

  ThrowIfFailed(device_->CreateCommandList(0,
                                           D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           commandAllocator_.Get(),
                                           pipelineState_.Get(),
                                           IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf())));
  ThrowIfFailed(commandList_->Close());

  // Vertex buffer
  {
    float aspectRatio = viewport_.Width / viewport_.Height;

    Vertex vertices[] = {{{0.0f, 0.8660254f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                         {{1.0f, -0.8660254f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                         {{-1.0f, -0.8660254f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    UINT vertexBufferSize = sizeof(vertices);

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    ThrowIfFailed(
        device_->CreateCommittedResource(&heapProp,
                                         D3D12_HEAP_FLAG_NONE,
                                         &resDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ,
                                         nullptr,
                                         IID_PPV_ARGS(vertexBuffer_.ReleaseAndGetAddressOf())));
    // Copy vertex data to vertex buffer
    UINT8* vertexBegin;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(vertexBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&vertexBegin)));
    memcpy(vertexBegin, vertices, vertexBufferSize);
    vertexBuffer_->Unmap(0, nullptr);

    // Vertex buffer view
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = sizeof(Vertex);
    vertexBufferView_.SizeInBytes = vertexBufferSize;
  }

  // Constant buffer
  {
    constexpr UINT cBufferSize = (sizeof(CBufferObject) + 255) & 0xffffff00;
    static_assert(cBufferSize % 256 == 0, "Constant buffer must be 256-byte aligned");

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(cBufferSize);
    ThrowIfFailed(
        device_->CreateCommittedResource(&heapProp,
                                         D3D12_HEAP_FLAG_NONE,
                                         &resDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ,
                                         nullptr,
                                         IID_PPV_ARGS(cBuffer_.ReleaseAndGetAddressOf())));

    // CBV
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = cBuffer_->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = cBufferSize;
    device_->CreateConstantBufferView(&cbvDesc, cbvHeap_->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(cBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&cBufferBegin_)));
  }

  // Fence
  {
    ThrowIfFailed(device_->CreateFence(0,
                                       D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())));

    WaitForPreviousFrame();
  }
}

void D3DApp::FrameStatics() {
  ++accumulatedFrameCount_;

  auto t = timer_.Now();
  auto frameTime = ToSeconds(t - prevFrameTimePoint_);
  prevFrameTimePoint_ = t;
  totalTimeElapsed_ += frameTime;
  accumulatedFrameTime_ += frameTime;

  if (accumulatedFrameTime_ > 1.0f) {
    framesPerSecond_ = static_cast<float>(accumulatedFrameCount_) / accumulatedFrameTime_;
    accumulatedFrameCount_ = 0;
    accumulatedFrameTime_ = 0.0f;

    auto [x, y, z] = camera_->GetWorldPosition();
    auto posString = std::to_wstring(x) + L"," + std::to_wstring(y) + L"," + std::to_wstring(z);
    OutputDebugString((std::to_wstring(totalTimeElapsed_) + L"\t" + posString + L"\n").c_str());
  }
}

void D3DApp::UpdateScene() {
  // auto t = totalTimeElapsed_;
  // float r = 5.0f;
  // float phi = t * 1.0f;
  // float theta = XM_PIDIV2;
  //
  // float x = r * std::sin(theta) * std::cos(phi);
  // float y = r * std::cos(theta);
  // float z = r * std::sin(theta) * std::sin(phi);
  //
  // camera_->SetWorldPosition({x, y, z});
}

void D3DApp::Update() {
  if (!isRunning_)
    return;

  FrameStatics();
  UpdateScene();

  // Update constant buffer
  constexpr UINT cBufferSize = (sizeof(CBufferObject) + 255) & 0xffffff00;
  memset(cBufferBegin_, 0, cBufferSize);

  CBufferObject cbo{};
  XMStoreFloat4x4(&cbo.view, MatView(camera_.get()));
  XMStoreFloat4x4(&cbo.proj, MatProj(camera_.get()));
  XMStoreFloat4(&cbo.red, XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f));
  XMStoreFloat4x4(&cbo.translate, XMMatrixTranslation(1.0f, 0.0f, 0.0f));
  // cbo.timeElapsed = ToSeconds(timer_.TimeElapsed());
  cbo.timeElapsed = totalTimeElapsed_;
  memcpy(cBufferBegin_, &cbo, sizeof(cbo));
}

void D3DApp::Render() {
  PopulateCommandList();

  ID3D12CommandList* commandLists[] = {commandList_.Get()};
  commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

  // present and swap front and back buffers
  ThrowIfFailed(swapChain_->Present(1, 0));
  frameIndex_ = (frameIndex_ + 1) % s_renderTargetCount;
  WaitForPreviousFrame();
}

void D3DApp::PopulateCommandList() {
  ThrowIfFailed(commandAllocator_->Reset());
  ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineState_.Get()));

  commandList_->SetGraphicsRootSignature(rootSignature_.Get());

  ID3D12DescriptorHeap* heaps[] = {cbvHeap_.Get()};
  commandList_->SetDescriptorHeaps(_countof(heaps), heaps);

  commandList_->SetGraphicsRootDescriptorTable(0, cbvHeap_->GetGPUDescriptorHandleForHeapStart());

  commandList_->RSSetViewports(1, &viewport_);
  commandList_->RSSetScissorRects(1, &scissorRect_);

  Transition(renderTargets_[frameIndex_].Get(),
             D3D12_RESOURCE_STATE_PRESENT,
             D3D12_RESOURCE_STATE_RENDER_TARGET);

  auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE{rtvHeap_->GetCPUDescriptorHandleForHeapStart(),
                                           frameIndex_,
                                           rtvDescriptorSize_};
  commandList_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

  constexpr float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  commandList_->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
  commandList_->DrawInstanced(3, 1, 0, 0);

  Transition(renderTargets_[frameIndex_].Get(),
             D3D12_RESOURCE_STATE_RENDER_TARGET,
             D3D12_RESOURCE_STATE_PRESENT);

  ThrowIfFailed(commandList_->Close());
}

void D3DApp::Destroy() {
  WaitForPreviousFrame();
}

Camera* D3DApp::GetCamera() const {
  return camera_.get();
}

void D3DApp::WaitForPreviousFrame() {
  auto fenceValue = ++nextFenceValue_;
  ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValue));

  if (fence_->GetCompletedValue() < fenceValue) {
    auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ThrowIfFailed(fence_->SetEventOnCompletion(fenceValue, event));
    WaitForSingleObject(event, INFINITE);
  }
}

void D3DApp::Transition(ID3D12Resource* resource,
                        D3D12_RESOURCE_STATES before,
                        D3D12_RESOURCE_STATES after) {
  auto transition = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
  commandList_->ResourceBarrier(1, &transition);
}

bool D3DApp::IsRunning() const {
  return isRunning_;
}

void D3DApp::StartRunning() {
  isRunning_ = true;
  prevFrameTimePoint_ = timer_.Now();
  timer_.Start();
}

void D3DApp::PauseRunning() {
  isRunning_ = false;
  timer_.Pause();
}

float D3DApp::GetViewportWidth() const {
  return viewport_.Width;
}

float D3DApp::GetViewportHeight() const {
  return viewport_.Height;
}