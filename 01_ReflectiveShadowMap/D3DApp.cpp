#include "D3DApp.h"

#include "D3DUtils.h"
#include "Rect.h"

using DX::ThrowIfFailed;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

D3DApp::D3DApp(std::wstring name, int viewportWidth, int viewportHeight)
    : name_{std::move(name)},
      viewport_{
          MakeViewport(static_cast<float>(viewportWidth), static_cast<float>(viewportHeight))},
      scissorRect_{MakeScissorRect(viewportWidth, viewportHeight)} {

  camera_ = std::make_unique<Camera>(
      XMFLOAT3{0.0f, 2.0f, -10.0f}, XMFLOAT3{0.0f, 0.0f, 1.0f},
      static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight), 60.0f, 1.f, 100.0f);
}

void D3DApp::Initialize(HWND window) {
  UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
  if (ComPtr<ID3D12Debug> debugController;
      SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
    debugController->EnableDebugLayer();
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  ComPtr<IDXGIFactory4> factory;
  ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(factory.GetAddressOf())));

  {
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    DX::GetHardwareAdapter(factory.Get(), hardwareAdapter.GetAddressOf(), false);

    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(device_.ReleaseAndGetAddressOf())));
  }
  {
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device_->CreateCommandQueue(
        &queueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = s_renderTargetCount;
    swapChainDesc.Width = viewport_.Width;
    swapChainDesc.Height = viewport_.Height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue_.Get(), window, &swapChainDesc,
                                                  nullptr, nullptr, swapChain.GetAddressOf()));
    ThrowIfFailed(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&swapChain_));
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
  }

  rtvHeap_ = MakeRtvHeap(device_.Get(), 6);
  dsvHeap_ = MakeDsvHeap(device_.Get(), 2);
  cbvSrvHeap_ = MakeCbvSrvUavHeap(device_.Get(), 9);


  for (int i = 0; i < s_renderTargetCount; ++i) {
    auto handle = rtvHeap_->CpuHandle(i);
    ThrowIfFailed(
        swapChain_->GetBuffer(i, IID_PPV_ARGS(renderTargets_[i].ReleaseAndGetAddressOf())));
    device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, handle);
  }

  {
    // depth buffer
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthBufferFormat_, static_cast<UINT64>(viewport_.Width),
        static_cast<UINT>(viewport_.Height), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    auto clearVal = CD3DX12_CLEAR_VALUE(depthBufferFormat_, 1.f, 0);
    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearVal,
        IID_PPV_ARGS(depthStencilBuffer_.ReleaseAndGetAddressOf())));


    // dsv
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthBufferFormat_;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device_->CreateDepthStencilView(depthStencilBuffer_.Get(), &dsvDesc, dsvHeap_->CpuHandle(0));
  }


  ThrowIfFailed(device_->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator_.ReleaseAndGetAddressOf())));

  // Static sampler
  D3D12_STATIC_SAMPLER_DESC staticSamplerDesc{};
  staticSamplerDesc.ShaderRegister = 0;
  staticSamplerDesc.RegisterSpace = 0;
  staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
  staticSamplerDesc.MipLODBias = 0.f;
  staticSamplerDesc.MinLOD = 0;
  staticSamplerDesc.MaxLOD = INT_MAX;

  // Root signature
  {
    CD3DX12_DESCRIPTOR_RANGE range[3];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);  // b0
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);  // b1
    range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);  // t0-t3

    CD3DX12_ROOT_PARAMETER rootParameter[3];
    // register b0: pass constant
    rootParameter[0].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_ALL);

    // register b1: model constant
    rootParameter[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

    // register t0-t2: textures
    rootParameter[2].InitAsDescriptorTable(1, &range[2]);


    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init(_countof(rootParameter), rootParameter, 1, &staticSamplerDesc,
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              signature.GetAddressOf(), error.GetAddressOf()));
    ThrowIfFailed(device_->CreateRootSignature(0, signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(rootSignature_.GetAddressOf())));
  }

  // Pipeline state object and shaders
  {
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> lightVertexShader;
    ComPtr<ID3DBlob> lightPixelShader;

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSLight", "vs_5_0",
                                     compileFlags, 0, lightVertexShader.GetAddressOf(), nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSLight", "ps_5_0",
                                     compileFlags, 0, lightPixelShader.GetAddressOf(), nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VS", "vs_5_0",
                                     compileFlags, 0, vertexShader.GetAddressOf(), nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PS", "ps_5_0",
                                     compileFlags, 0, pixelShader.GetAddressOf(), nullptr));


    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };


    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso1Desc{};
    pso1Desc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
    pso1Desc.pRootSignature = rootSignature_.Get();
    pso1Desc.VS = CD3DX12_SHADER_BYTECODE(lightVertexShader.Get());
    pso1Desc.PS = CD3DX12_SHADER_BYTECODE(lightPixelShader.Get());
    pso1Desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso1Desc.RasterizerState.DepthBias = 10000;
    pso1Desc.RasterizerState.DepthBiasClamp = 0.f;
    pso1Desc.RasterizerState.SlopeScaledDepthBias = 1.f;
    pso1Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso1Desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso1Desc.DSVFormat = depthBufferFormat_;
    pso1Desc.SampleMask = UINT_MAX;
    pso1Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso1Desc.NumRenderTargets = 4;
    pso1Desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    pso1Desc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    pso1Desc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    pso1Desc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    pso1Desc.SampleDesc.Count = 1;

    ThrowIfFailed(device_->CreateGraphicsPipelineState(
        &pso1Desc, IID_PPV_ARGS(pipelineStatePass1_.ReleaseAndGetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso2Desc{};
    pso2Desc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
    pso2Desc.pRootSignature = rootSignature_.Get();
    pso2Desc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    pso2Desc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    pso2Desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso2Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso2Desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso2Desc.DSVFormat = depthBufferFormat_;
    pso2Desc.SampleMask = UINT_MAX;
    pso2Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso2Desc.NumRenderTargets = 1;
    pso2Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso2Desc.SampleDesc.Count = 1;

    ThrowIfFailed(device_->CreateGraphicsPipelineState(
        &pso2Desc, IID_PPV_ARGS(pipelineStatePass2_.ReleaseAndGetAddressOf())));

    ThrowIfFailed(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                             commandAllocator_.Get(), nullptr,
                                             IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf())));

    ThrowIfFailed(commandList_->Close());
  }

  // Constant buffer and view
  {
    passCBuffer_ = std::make_unique<ConstantBuffer<PassConstant>>(device_.Get(), 1);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = passCBuffer_->ElementGpuVirtualAddress();
    cbvDesc.SizeInBytes = passCBuffer_->BufferByteSize();
    device_->CreateConstantBufferView(&cbvDesc, cbvSrvHeap_->CpuHandle(0));
  }

  // Fence
  {
    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())));

    WaitForGpuCompletion();
  }


  // Prepare RSM
  {
    // RSM rtv and srv
    rsmDepth_ = std::make_unique<Rsm>(device_.Get(), rtvHeap_->CpuHandle(s_rsmRtvStartIndex),
                                      cbvSrvHeap_->CpuHandle(s_rsmSrvStartIndex),
                                      XMFLOAT4{1.f, 1.f, 1.f, 1.f});
    rsmNormal_ = std::make_unique<Rsm>(device_.Get(), rtvHeap_->CpuHandle(s_rsmRtvStartIndex + 1),
                                       cbvSrvHeap_->CpuHandle(s_rsmSrvStartIndex + 1));
    rsmFlux_ = std::make_unique<Rsm>(device_.Get(), rtvHeap_->CpuHandle(s_rsmRtvStartIndex + 2),
                                     cbvSrvHeap_->CpuHandle(s_rsmSrvStartIndex + 2));
    rsmWorldPos_ = std::make_unique<Rsm>(device_.Get(), rtvHeap_->CpuHandle(s_rsmRtvStartIndex + 3),
                                         cbvSrvHeap_->CpuHandle(s_rsmSrvStartIndex + 3));
  }

  // RSM depth stencil buffer
  {
    auto depthBufferHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto depthBufferResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthBufferFormat_, static_cast<UINT64>(s_rsmSize), static_cast<UINT>(s_rsmSize), 1, 0, 1,
        0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    auto clearVal = CD3DX12_CLEAR_VALUE(depthBufferFormat_, 1.f, 0);

    ThrowIfFailed(device_->CreateCommittedResource(
        &depthBufferHeapProp, D3D12_HEAP_FLAG_NONE, &depthBufferResDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearVal,
        IID_PPV_ARGS(shadowDepthBuffer_.ReleaseAndGetAddressOf())));
  }
  // RSM DSV
  {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthBufferFormat_;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device_->CreateDepthStencilView(shadowDepthBuffer_.Get(), &dsvDesc,
                                    dsvHeap_->CpuHandle(s_rsmDsvStartIndex));
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

  ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineStatePass1_.Get()));

  // Concatenate vertex buffers and send to default heap from temporary upload heap
  UINT vBufferSize = bunnyVBufferSize + rectVBufferSize;
  vBuffer_ = std::make_unique<DefaultBuffer>(device_.Get(), vBufferSize);
  commandList_->CopyBufferRegion(vBuffer_->Resource(), 0, bunnyVBuffer.Resource(), 0,
                                 bunnyVBuffer.BufferByteSize());
  commandList_->CopyBufferRegion(vBuffer_->Resource(), bunnyVBufferSize, rectVBuffer.Resource(), 0,
                                 rectVBufferSize);

  // Concatenate index buffers and send to default heap from temporary upload heap
  UINT iBufferSize = bunnyIBufferSize + rectIBufferSize;
  iBuffer_ = std::make_unique<DefaultBuffer>(device_.Get(), iBufferSize);
  commandList_->CopyBufferRegion(iBuffer_->Resource(), 0, bunnyIBuffer.Resource(), 0,
                                 bunnyIBuffer.BufferByteSize());
  commandList_->CopyBufferRegion(iBuffer_->Resource(), bunnyIBufferSize, rectIBuffer.Resource(), 0,
                                 rectIBufferSize);

  ThrowIfFailed(commandList_->Close());
  ExecuteCommandList();
  WaitForGpuCompletion();

  // VBV and IBV about the final concatenated vertex and index buffer
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
    auto transform = XMMatrixTransformation(origin, XMQuaternionIdentity(), scale, origin, rotation,
                                            translation);
    XMStoreFloat4x4(&bunnyItem.model, transform);
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
    auto transform = XMMatrixTransformation(origin, XMQuaternionIdentity(), scale, origin, rotation,
                                            translation);
    XMStoreFloat4x4(&rectXZ.model, transform);
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
    auto transform = XMMatrixTransformation(origin, XMQuaternionIdentity(), scale, origin, rotation,
                                            translation);
    XMStoreFloat4x4(&rectXY.model, transform);
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
    auto transform = XMMatrixTransformation(origin, XMQuaternionIdentity(), scale, origin, rotation,
                                            translation);
    XMStoreFloat4x4(&rectYZ.model, transform);
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
  for (auto& ri : renderItems_) {
    auto cbv = cbvSrvHeap_->CpuHandle(index);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = modelCBuffer_->ElementGpuVirtualAddress(ri.modelCBufferIndex);
    cbvDesc.SizeInBytes = modelCBuffer_->ElementPaddedSize();
    device_->CreateConstantBufferView(&cbvDesc, cbv);

    ri.modelCbv = cbvSrvHeap_->GpuHandle(index);
    ++index;
  }
}

void D3DApp::FrameStatistics() {
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
  }
}

void D3DApp::UpdateScene() {
  // Update model constants for every render items
  for (const auto& ri : renderItems_) {
    ModelConstant c{};
    c.model = ri.model;
    c.invModel = Float4x4Inverse(ri.model);
    c.color = ri.material->albedo;
    modelCBuffer_->LoadElement(ri.modelCBufferIndex, c);
  }
}

void D3DApp::Update() {
  if (!isRunning_)
    return;

  FrameStatistics();
  UpdateScene();

  // Update pass constant buffer
  passCBuffer_->ClearBuffer(0);

  XMMATRIX view = MatView(camera_.get());
  auto detView = XMMatrixDeterminant(view);

  XMMATRIX proj = MatProj(camera_.get());
  auto detProj = XMMatrixDeterminant(proj);

  XMMATRIX lightView = MatLightView(&directionalLight_);
  auto detLightView = XMMatrixDeterminant(lightView);

  XMMATRIX lightOrtho = MatLightOrtho(&directionalLight_);
  auto detLightOrtho = XMMatrixDeterminant(lightOrtho);

  PassConstant cbo{};
  XMStoreFloat4x4(&cbo.view, view);
  XMStoreFloat4x4(&cbo.invView, XMMatrixInverse(&detView, view));
  XMStoreFloat4x4(&cbo.proj, proj);
  XMStoreFloat4x4(&cbo.invProj, XMMatrixInverse(&detProj, proj));
  XMStoreFloat4x4(&cbo.lightView, lightView);
  XMStoreFloat4x4(&cbo.invLightView, XMMatrixInverse(&detLightView, lightView));
  XMStoreFloat4x4(&cbo.lightOrtho, lightOrtho);
  XMStoreFloat4x4(&cbo.invLightOrtho, XMMatrixInverse(&detLightOrtho, lightOrtho));
  cbo.lightFlux = directionalLight_.color;
  cbo.lightZNear = directionalLight_.epsilon;
  cbo.lightDirection = directionalLight_.dir;
  cbo.lightZFar = directionalLight_.epsilon + directionalLight_.affectedDepth;
  cbo.lightPos = directionalLight_.pos;
  cbo.width = viewport_.Width;
  cbo.height = viewport_.Height;
  cbo.rsmSize = static_cast<float>(s_rsmSize);
  cbo.timeElapsed = totalTimeElapsed_;
  passCBuffer_->LoadElement(0, cbo);
}

void D3DApp::ExecuteCommandList() const {
  ID3D12CommandList* commandLists[] = {commandList_.Get()};
  commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void D3DApp::Render() {
  PopulateCommandListFirstPass();
  ExecuteCommandList();

  WaitForGpuCompletion();

  PopulateCommandListSecondPass();
  ExecuteCommandList();


  // Present and swap front and back buffers
  ThrowIfFailed(swapChain_->Present(1, 0));
  frameIndex_ = (frameIndex_ + 1) % s_renderTargetCount;
  WaitForGpuCompletion();
}

void D3DApp::PopulateCommandListFirstPass() {
  ThrowIfFailed(commandAllocator_->Reset());
  ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineStatePass1_.Get()));

  commandList_->SetPipelineState(pipelineStatePass1_.Get());

  rsmDepth_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
  rsmNormal_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
  rsmFlux_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
  rsmWorldPos_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

  commandList_->SetGraphicsRootSignature(rootSignature_.Get());

  ID3D12DescriptorHeap* heaps[] = {cbvSrvHeap_->Heap()};
  commandList_->SetDescriptorHeaps(_countof(heaps), heaps);

  commandList_->SetGraphicsRootDescriptorTable(0, cbvSrvHeap_->GpuHandle(0));

  auto viewport = MakeViewport(s_rsmSize, s_rsmSize);
  commandList_->RSSetViewports(1, &viewport);

  auto rect = MakeScissorRect(s_rsmSize, s_rsmSize);
  commandList_->RSSetScissorRects(1, &rect);

  auto dsv = dsvHeap_->CpuHandle(s_rsmDsvStartIndex);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {rsmDepth_->Rtv(), rsmNormal_->Rtv(), rsmFlux_->Rtv(),
                                        rsmWorldPos_->Rtv()};

  commandList_->OMSetRenderTargets(4, rtvs, false, &dsv);


  rsmDepth_->Clear(commandList_.Get());
  rsmNormal_->Clear(commandList_.Get());
  rsmFlux_->Clear(commandList_.Get());
  rsmWorldPos_->Clear(commandList_.Get());

  commandList_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);


  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList_->IASetVertexBuffers(0, 1, &vbv_);
  commandList_->IASetIndexBuffer(&ibv_);

  DrawAllRenderItems();


  rsmDepth_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
  rsmFlux_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
  rsmNormal_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
  rsmWorldPos_->TransitionTo(commandList_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

  ThrowIfFailed(commandList_->Close());
}

void D3DApp::PopulateCommandListSecondPass() {
  ThrowIfFailed(commandAllocator_->Reset());
  ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineStatePass2_.Get()));

  commandList_->SetPipelineState(pipelineStatePass2_.Get());

  commandList_->SetGraphicsRootSignature(rootSignature_.Get());

  ID3D12DescriptorHeap* heaps[] = {cbvSrvHeap_->Heap()};
  commandList_->SetDescriptorHeaps(_countof(heaps), heaps);

  commandList_->SetGraphicsRootDescriptorTable(0, cbvSrvHeap_->GpuHandle(0));

  commandList_->SetGraphicsRootDescriptorTable(2, cbvSrvHeap_->GpuHandle(s_rsmSrvStartIndex));

  commandList_->RSSetViewports(1, &viewport_);
  commandList_->RSSetScissorRects(1, &scissorRect_);

  Transition(renderTargets_[frameIndex_].Get(), D3D12_RESOURCE_STATE_PRESENT,
             D3D12_RESOURCE_STATE_RENDER_TARGET);

  auto rtv = rtvHeap_->CpuHandle(frameIndex_);
  auto dsv = dsvHeap_->CpuHandle(0);

  commandList_->OMSetRenderTargets(1, &rtv, true, &dsv);

  constexpr float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  commandList_->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

  commandList_->ClearDepthStencilView(dsvHeap_->CpuHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0,
                                      nullptr);


  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList_->IASetVertexBuffers(0, 1, &vbv_);
  commandList_->IASetIndexBuffer(&ibv_);

  DrawAllRenderItems();

  Transition(renderTargets_[frameIndex_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
             D3D12_RESOURCE_STATE_PRESENT);

  ThrowIfFailed(commandList_->Close());
}

void D3DApp::DrawAllRenderItems() {
  for (const auto& ri : renderItems_) {
    commandList_->SetGraphicsRootDescriptorTable(1, ri.modelCbv);
    commandList_->DrawIndexedInstanced(ri.indexCount, 1, ri.startIndexLocation,
                                       ri.baseVertexLocation, 0);
  }
}

void D3DApp::Destroy() {
  WaitForGpuCompletion();
}

Camera* D3DApp::GetCamera() const {
  return camera_.get();
}

void D3DApp::WaitForGpuCompletion() {
  auto fenceValue = ++nextFenceValue_;
  ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValue));

  if (fence_->GetCompletedValue() < fenceValue) {
    auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ThrowIfFailed(fence_->SetEventOnCompletion(fenceValue, event));
    WaitForSingleObject(event, INFINITE);
  }
}

void D3DApp::Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
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

CD3DX12_VIEWPORT MakeViewport(float w, float h) {
  return CD3DX12_VIEWPORT{0.f, 0.f, w, h};
}

CD3DX12_RECT MakeScissorRect(LONG w, LONG h) {
  return CD3DX12_RECT{0, 0, w, h};
}