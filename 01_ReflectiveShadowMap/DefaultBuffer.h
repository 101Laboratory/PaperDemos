#pragma once
#include <d3d12.h>
#include <d3dx12_core.h>
#include <wrl/client.h>

#include "D3DUtils.h"

class DefaultBuffer {
public:
  explicit DefaultBuffer(ID3D12Device* device, size_t byteSize);

  size_t BufferByteSize() const { return byteSize_; }

  ID3D12Resource* Resource() const { return buffer_.Get(); }

  D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress() const { return buffer_->GetGPUVirtualAddress(); }

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
  size_t byteSize_ = 0;
};

inline DefaultBuffer::DefaultBuffer(ID3D12Device* device, size_t byteSize) : byteSize_{byteSize} {
  auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  DX::ThrowIfFailed(device->CreateCommittedResource(&heapProp,
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &desc,
                                                    D3D12_RESOURCE_STATE_COMMON,
                                                    nullptr,
                                                    IID_PPV_ARGS(buffer_.GetAddressOf())));
}
