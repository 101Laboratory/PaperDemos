#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl/client.h>

class DescriptorHeap {
public:
  DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc);

  size_t Count() const { return count_; }

  CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle(INT index);

  CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle(INT index);

  ID3D12DescriptorHeap* Heap() const { return heap_.Get(); }

private:
  D3D12_DESCRIPTOR_HEAP_TYPE type_;
  size_t count_ = 0;
  UINT descriptorSize_ = 0;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
};

std::unique_ptr<DescriptorHeap> MakeRtvHeap(ID3D12Device* device, UINT count);

std::unique_ptr<DescriptorHeap> MakeDsvHeap(ID3D12Device* device, UINT count);

std::unique_ptr<DescriptorHeap> MakeCbvSrvUavHeap(ID3D12Device* device, UINT count);

std::unique_ptr<DescriptorHeap> MakeSamplerHeap(ID3D12Device* device, UINT count);
