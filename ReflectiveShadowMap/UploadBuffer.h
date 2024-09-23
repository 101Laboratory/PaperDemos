#pragma once
#include <d3d12.h>
#include <d3dx12_core.h>
#include <wrl/client.h>

#include <cassert>
#include <functional>
#include <stdexcept>

#include "D3DUtils.h"

inline int ComputePaddedSize(int size, int alignment) {
  if (alignment <= 0) {
    throw std::runtime_error{"alignment must be positive"};
  }
  int padding = (alignment - size % alignment) % alignment;
  return size + padding;
}

template <typename T, int alignment = sizeof(T)>
class UploadBuffer {
public:
  explicit UploadBuffer(ID3D12Device* device, size_t count);

  ~UploadBuffer();

  size_t BufferByteSize() const { return elemPaddedSize_ * elemCount_; }

  size_t ElementPaddedSize() const { return elemPaddedSize_; }

  void ClearBuffer(int value) const { memset(bufBegin_, value, BufferByteSize()); }

  void LoadBuffer(size_t byteOffset, const void* dataBegin, size_t byteSize) const;

  void LoadElement(size_t index, const T& elem);

  D3D12_GPU_VIRTUAL_ADDRESS ElementGpuVirtualAddress(size_t index = 0) const;

  ID3D12Resource* Resource() const { return buffer_.Get(); }

  size_t ElementCount() const { return elemCount_; }

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
  void* bufBegin_ = nullptr;
  int elemPaddedSize_ = 0;
  size_t elemCount_ = 0;
};

template <typename T, int alignment>
UploadBuffer<T, alignment>::UploadBuffer(ID3D12Device* device, size_t count)
    : elemPaddedSize_{ComputePaddedSize(sizeof(T), alignment)},
      elemCount_{count} {

  auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(elemCount_ * elemPaddedSize_);

  DX::ThrowIfFailed(device->CreateCommittedResource(&heapProp,
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &resDesc,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                    nullptr,
                                                    IID_PPV_ARGS(buffer_.GetAddressOf())));

  D3D12_RANGE range = {0, 0};  // CPU won't read;
  DX::ThrowIfFailed(buffer_->Map(0, &range, &bufBegin_));
}

template <typename T, int alignment>
UploadBuffer<T, alignment>::~UploadBuffer() {
  D3D12_RANGE range = {0, 0};
  buffer_->Unmap(0, &range);
  bufBegin_ = nullptr;
}

template <typename T, int alignment>
void UploadBuffer<T, alignment>::LoadBuffer(size_t byteOffset,
                                            const void* dataBegin,
                                            size_t byteSize) const {
  auto* dst = static_cast<UINT8*>(bufBegin_) + byteOffset;
  memcpy(dst, dataBegin, byteSize);
}

template <typename T, int alignment>
void UploadBuffer<T, alignment>::LoadElement(size_t index, const T& elem) {
  auto* p = static_cast<UINT8*>(bufBegin_);
  p += index * elemPaddedSize_;
  memcpy(p, &elem, sizeof(elem));
}

template <typename T, int alignment>
D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<T, alignment>::ElementGpuVirtualAddress(size_t index) const {
  return buffer_->GetGPUVirtualAddress() + index * elemPaddedSize_;
}
