#include "D3DApp.h"

#include "D3DUtils.h"
#include "Rect.h"

using DX::ThrowIfFailed;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

D3DApp::D3DApp(std::wstring name, int viewportWidth, int viewportHeight)
    : name_{std::move(name)},
      viewport_{0.0f, 0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)},
      scissorRect_{0, 0, static_cast<LONG>(viewportWidth), static_cast<LONG>(viewportHeight)} {
  
  camera_ = std::make_unique<Camera>(
      XMFLOAT3{0.0f, 2.0f, -10.0f},
      XMFLOAT3{0.0f, 0.0f, 1.0f},
      static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
      60.0f,
      0.01f,
      100.0f);
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
  cbvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


  for (int i = 0; i < s_renderTargetCount; ++i) {
    auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE{rtvHeap_->GetCPUDescriptorHandleForHeapStart(),
                                                i,
                                                rtvDescriptorSize_};
    ThrowIfFailed(
        swapChain_->GetBuffer(i, IID_PPV_ARGS(renderTargets_[i].ReleaseAndGetAddressOf())));
    device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, handle);
  }

  {
    // depth buffer
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat_,
                                                static_cast<UINT64>(viewport_.Width),
                                                static_cast<UINT>(viewport_.Height),
                                                1,
                                                0,
                                                1,
                                                0,
                                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    auto clearVal = CD3DX12_CLEAR_VALUE(depthBufferFormat_, 1.f, 0);
    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearVal,
        IID_PPV_ARGS(depthStencilBuffer_.ReleaseAndGetAddressOf())));

    // dsv heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDesc,
                                                IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf())));

    // dsv
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthBufferFormat_;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device_->CreateDepthStencilView(depthStencilBuffer_.Get(),
                                    &dsvDesc,
                                    dsvHeap_->GetCPUDescriptorHandleForHeapStart());
  }

  {
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 5;  // 1 pass + 4 model; (4 model = 1 bunny + 3 walls)
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
    CD3DX12_DESCRIPTOR_RANGE range[2];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);  // b0
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);  // b1

    CD3DX12_ROOT_PARAMETER rootParameter[2];
    // register b0: pass constant
    rootParameter[0].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_ALL);

    // register b1: model constant
    rootParameter[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);


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
                                                   {"NORMAL",
                                                    0,
                                                    DXGI_FORMAT_R32G32B32_FLOAT,
                                                    0,
                                                    16,
                                                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                    0},
                                                   {"COLOR",
                                                    0,
                                                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                    0,
                                                    32,
                                                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                    0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = depthBufferFormat_;
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


  // Constant buffer and view
  {
    passCBuffer_ = std::make_unique<ConstantBuffer<PassConstant>>(device_.Get(), 1);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = passCBuffer_->ElementGpuVirtualAddress();
    cbvDesc.SizeInBytes = passCBuffer_->BufferByteSize();
    device_->CreateConstantBufferView(&cbvDesc, cbvHeap_->GetCPUDescriptorHandleForHeapStart());
  }

  // Fence
  {
    ThrowIfFailed(device_->CreateFence(0,
                                       D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())));

    WaitForPreviousFrame();
  }

  InitializeScene();
}

void D3DApp::InitializeScene() {
  ObjModel bunny{"stanford-bunny.obj"};

  // Load vertices of bunny
  UINT bunnyVBufferSize = bunny.VerticesByteSize();

  UploadBuffer<Vertex> bunnyVBuffer{device_.Get(), bunny.VertexCount()};
  bunnyVBuffer.LoadBuffer(0, bunny.VerticesBegin(), bunnyVBufferSize);

  // Load indices of bunny
  UINT bunnyIBufferSize = bunny.IndicesByteSize();

  UploadBuffer<std::uint16_t> bunnyIBuffer{device_.Get(), bunny.IndexCount()};
  bunnyIBuffer.LoadBuffer(0, bunny.IndicesBegin(), bunnyIBufferSize);

  RectXZ rect{1.f, 1.f};
  // Load plane vertices
  auto&& rectVertices = RectXZVertices(rect);
  UINT rectVBufferSize = rectVertices.size() * sizeof(Vertex);

  UploadBuffer<Vertex> rectVBuffer{device_.Get(), rectVertices.size()};
  rectVBuffer.LoadBuffer(0, rectVertices.data(), rectVBufferSize);

  // Load plane indices
  auto&& rectIndices = RectXZIndices(rect);
  UINT rectIBufferSize = rectIndices.size() * sizeof(std::uint16_t);

  UploadBuffer<std::uint16_t> rectIBuffer{device_.Get(), rectIndices.size()};
  rectIBuffer.LoadBuffer(0, rectIndices.data(), rectIBufferSize);

  ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineState_.Get()));

  // Concatenate vertex buffers
  UINT vBufferSize = bunnyVBufferSize + rectVBufferSize;
  vBuffer_ = std::make_unique<DefaultBuffer>(device_.Get(), vBufferSize);
  commandList_->CopyBufferRegion(vBuffer_->Resource(),
                                 0,
                                 bunnyVBuffer.Resource(),
                                 0,
                                 bunnyVBuffer.BufferByteSize());
  commandList_->CopyBufferRegion(vBuffer_->Resource(),
                                 bunnyVBufferSize,
                                 rectVBuffer.Resource(),
                                 0,
                                 rectVBufferSize);

  // Concatenate index buffers
  UINT iBufferSize = bunnyIBufferSize + rectIBufferSize;
  iBuffer_ = std::make_unique<DefaultBuffer>(device_.Get(), iBufferSize);
  commandList_->CopyBufferRegion(iBuffer_->Resource(),
                                 0,
                                 bunnyIBuffer.Resource(),
                                 0,
                                 bunnyIBuffer.BufferByteSize());
  commandList_->CopyBufferRegion(iBuffer_->Resource(),
                                 bunnyIBufferSize,
                                 rectIBuffer.Resource(),
                                 0,
                                 rectIBufferSize);

  ThrowIfFailed(commandList_->Close());
  ExecuteCommandList();
  WaitForPreviousFrame();

  // VBV and IBV
  vbv_.BufferLocation = vBuffer_->GpuVirtualAddress();
  vbv_.SizeInBytes = vBufferSize;
  vbv_.StrideInBytes = sizeof(Vertex);

  ibv_.BufferLocation = iBuffer_->GpuVirtualAddress();
  ibv_.SizeInBytes = iBufferSize;
  ibv_.Format = DXGI_FORMAT_R16_UINT;

  // Render item of bunny
  auto origin = XMVectorSet(0.f, 0.f, 0.f, 0.f);
  {
    RenderItem bunnyItem;
    float s = 20.f;
    auto translation = XMVectorSet(0.f, 0.f, 0.f, 0.f);
    auto rotation = XMQuaternionIdentity();
    auto scale = XMVectorSet(s, s, s, 0.f);
    auto transform = XMMatrixTransformation(origin,
                                            XMQuaternionIdentity(),
                                            scale,
                                            origin,
                                            rotation,
                                            translation);
    XMStoreFloat4x4(&bunnyItem.world, transform);
    bunnyItem.startIndexLocation = 0;
    bunnyItem.indexCount = bunnyIBuffer.ElementCount();
    bunnyItem.baseVertexLocation = 0;
    bunnyItem.modelCBufferIndex = 0;  // Model constant buffer index [0]
    bunnyItem.material = std::make_shared<Diffuse>();
    renderItems_.push_back(bunnyItem);
  }

  // Render items of walls
  float cornerSize = 5.f;
  {
    RenderItem rectXZ;
    auto translation = XMVectorSet(0.f, 0.f, 0.f, 0.f);
    auto rotation = XMQuaternionIdentity();
    auto scale = XMVectorSet(cornerSize, cornerSize, cornerSize, 0.f);
    auto transform = XMMatrixTransformation(origin,
                                            XMQuaternionIdentity(),
                                            scale,
                                            origin,
                                            rotation,
                                            translation);
    XMStoreFloat4x4(&rectXZ.world, transform);
    rectXZ.startIndexLocation = bunnyIBuffer.ElementCount();
    rectXZ.indexCount = rectIBuffer.ElementCount();
    rectXZ.baseVertexLocation = bunnyVBuffer.ElementCount();
    rectXZ.modelCBufferIndex = 1;
    rectXZ.material = std::make_shared<Diffuse>(Diffuse{XMFLOAT3{0.f, 0.8f, 0.f}});
    renderItems_.push_back(rectXZ);
  }
  {
    RenderItem rectXY;
    auto translation = XMVectorSet(0.f, cornerSize / 2, cornerSize / 2, 0.f);
    auto rotation = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), -XM_PIDIV2);
    auto scale = XMVectorSet(cornerSize, cornerSize, cornerSize, 0.f);
    auto transform = XMMatrixTransformation(origin,
                                            XMQuaternionIdentity(),
                                            scale,
                                            origin,
                                            rotation,
                                            translation);
    XMStoreFloat4x4(&rectXY.world, transform);
    rectXY.startIndexLocation = bunnyIBuffer.ElementCount();
    rectXY.indexCount = rectIBuffer.ElementCount();
    rectXY.baseVertexLocation = bunnyVBuffer.ElementCount();
    rectXY.modelCBufferIndex = 2;
    rectXY.material = std::make_shared<Diffuse>(Diffuse{XMFLOAT3{0.f, 0.f, 0.8f}});
    renderItems_.push_back(rectXY);
  }
  {
    RenderItem rectYZ;
    auto translation = XMVectorSet(-cornerSize / 2, cornerSize / 2, 0.f, 0.f);
    auto rotation = XMQuaternionRotationAxis(XMVectorSet(0.f, 0.f, 1.f, 0.f), -XM_PIDIV2);
    auto scale = XMVectorSet(cornerSize, cornerSize, cornerSize, 0.f);
    auto transform = XMMatrixTransformation(origin,
                                            XMQuaternionIdentity(),
                                            scale,
                                            origin,
                                            rotation,
                                            translation);
    XMStoreFloat4x4(&rectYZ.world, transform);
    rectYZ.startIndexLocation = bunnyIBuffer.ElementCount();
    rectYZ.indexCount = rectIBuffer.ElementCount();
    rectYZ.baseVertexLocation = bunnyVBuffer.ElementCount();
    rectYZ.modelCBufferIndex = 3;
    rectYZ.material = std::make_shared<Diffuse>(Diffuse{XMFLOAT3{0.8f, 0.f, 0.f}});
    renderItems_.push_back(rectYZ);
  }

  // Model constant buffers for render items
  modelCBuffer_ = std::make_unique<ConstantBuffer<ModelConstant>>(device_.Get(),
                                                                  renderItems_.size());

  // CBVs for model constant buffer.
  // Index 0 of cbvHeap is modelCbv to pass constant buffer, so start from index 1.
  int index = 1;
  auto cbv = CD3DX12_CPU_DESCRIPTOR_HANDLE{cbvHeap_->GetCPUDescriptorHandleForHeapStart(),
                                           index,
                                           cbvDescriptorSize_};
  for (auto& ri : renderItems_) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = modelCBuffer_->ElementGpuVirtualAddress(ri.modelCBufferIndex);
    cbvDesc.SizeInBytes = modelCBuffer_->ElementPaddedSize();
    device_->CreateConstantBufferView(&cbvDesc, cbv);

    ri.modelCbv = CD3DX12_GPU_DESCRIPTOR_HANDLE{cbvHeap_->GetGPUDescriptorHandleForHeapStart(),
                                                index,
                                                cbvDescriptorSize_};
    ++index;
    cbv.Offset(1, cbvDescriptorSize_);
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

    auto [x, y, z] = camera_->worldPosition;
    auto posString = std::to_wstring(x) + L"," + std::to_wstring(y) + L"," + std::to_wstring(z);
    OutputDebugString((std::to_wstring(totalTimeElapsed_) + L"\t" + posString + L"\n").c_str());
  }
}

void D3DApp::UpdateScene() {
  // Update model constants for every render items
  for (const auto& ri : renderItems_) {
    ModelConstant c{};
    c.world = ri.world;
    c.color = ri.material->albedo;
    modelCBuffer_->LoadElement(ri.modelCBufferIndex, c);
  }
}

void D3DApp::Update() {
  if (!isRunning_)
    return;

  FrameStatics();
  UpdateScene();

  // Update constant buffer
  passCBuffer_->ClearBuffer(0);

  PassConstant cbo{};
  XMStoreFloat4x4(&cbo.view, MatView(camera_.get()));
  XMStoreFloat4x4(&cbo.proj, MatProj(camera_.get()));
  cbo.timeElapsed = totalTimeElapsed_;
  passCBuffer_->LoadElement(0, cbo);
}

void D3DApp::ExecuteCommandList() const {
  ID3D12CommandList* commandLists[] = {commandList_.Get()};
  commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void D3DApp::Render() {
  PopulateCommandList();

  ExecuteCommandList();

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
  auto dsv = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

  commandList_->OMSetRenderTargets(1, &rtv, true, &dsv);

  constexpr float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  commandList_->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

  commandList_->ClearDepthStencilView(dsvHeap_->GetCPUDescriptorHandleForHeapStart(),
                                      D3D12_CLEAR_FLAG_DEPTH,
                                      1.f,
                                      0,
                                      0,
                                      nullptr);

  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList_->IASetVertexBuffers(0, 1, &vbv_);
  commandList_->IASetIndexBuffer(&ibv_);

  // commandList_->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
  DrawAllRenderItems();

  Transition(renderTargets_[frameIndex_].Get(),
             D3D12_RESOURCE_STATE_RENDER_TARGET,
             D3D12_RESOURCE_STATE_PRESENT);

  ThrowIfFailed(commandList_->Close());
}

void D3DApp::DrawAllRenderItems() {
  for (const auto& ri : renderItems_) {
    // Model constant modelCbv
    commandList_->SetGraphicsRootDescriptorTable(1, ri.modelCbv);
    commandList_->DrawIndexedInstanced(ri.indexCount,
                                       1,
                                       ri.startIndexLocation,
                                       ri.baseVertexLocation,
                                       0);
  }
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