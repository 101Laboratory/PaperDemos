#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <stdexcept>

#include "D3DUtils.h"

template<size_t width, size_t height, DXGI_FORMAT format>
class RenderTarget {
public:
  static constexpr size_t Width() { return width; }

  static constexpr size_t Height() { return height; }

  static constexpr DXGI_FORMAT Format() { return format; }

  explicit RenderTarget(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE rtv,
                        CD3DX12_CPU_DESCRIPTOR_HANDLE srv,
                        DirectX::XMFLOAT4 clearColor = {0.f, 0.f, 0.f, 1.f});

  CD3DX12_CPU_DESCRIPTOR_HANDLE Srv() const { return srv_; }

  CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv() const { return rtv_; }

  void TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES to);

  void Clear(ID3D12GraphicsCommandList* commandList);


private:
  Microsoft::WRL::ComPtr<ID3D12Resource> texture_;

  FLOAT clearColor_[4];

  D3D12_RESOURCE_STATES state_;

  CD3DX12_CPU_DESCRIPTOR_HANDLE srv_;
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_;
};

template<size_t width, size_t height, DXGI_FORMAT format>
RenderTarget<width, height, format>::RenderTarget(ID3D12Device* device,
                                                  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv,
                                                  CD3DX12_CPU_DESCRIPTOR_HANDLE srv,
                                                  DirectX::XMFLOAT4 clearColor)
    : clearColor_{clearColor.x, clearColor.y, clearColor.z, clearColor.w},
      state_{D3D12_RESOURCE_STATE_GENERIC_READ} {

  auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0,
                                              D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

  auto clearVal = CD3DX12_CLEAR_VALUE(format, clearColor_);
  DX::ThrowIfFailed(
      device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                                      &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &clearVal,
                                      IID_PPV_ARGS(texture_.ReleaseAndGetAddressOf())));

  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format = format;
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  rtvDesc.Texture2D.PlaneSlice = 0;
  rtvDesc.Texture2D.MipSlice = 0;
  device->CreateRenderTargetView(texture_.Get(), &rtvDesc, rtv);
  rtv_ = rtv;

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Texture2D.MipLevels = 1;
  srvDesc.Texture2D.PlaneSlice = 0;
  srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
  device->CreateShaderResourceView(texture_.Get(), &srvDesc, srv);
  srv_ = srv;
}

template<size_t width, size_t height, DXGI_FORMAT format>
void RenderTarget<width, height, format>::TransitionTo(ID3D12GraphicsCommandList* commandList,
                                                       D3D12_RESOURCE_STATES to) {
  auto transition = CD3DX12_RESOURCE_BARRIER::Transition(texture_.Get(), state_, to);
  commandList->ResourceBarrier(1, &transition);
  state_ = to;
}

template<size_t width, size_t height, DXGI_FORMAT format>
void RenderTarget<width, height, format>::Clear(ID3D12GraphicsCommandList* commandList) {
  commandList->ClearRenderTargetView(rtv_, clearColor_, 0, nullptr);
}
