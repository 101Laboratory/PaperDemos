#include "DescriptorHeap.h"

#include "D3DUtils.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc)
    : type_{desc.Type},
      count_{desc.NumDescriptors} {
  descriptorSize_ = device->GetDescriptorHandleIncrementSize(type_);
  DX::ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap_.GetAddressOf())));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CpuHandle(INT index) {
  return {heap_->GetCPUDescriptorHandleForHeapStart(), index, descriptorSize_};
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GpuHandle(INT index) {
  return {heap_->GetGPUDescriptorHandleForHeapStart(), index, descriptorSize_};
}

std::unique_ptr<DescriptorHeap> MakeRtvHeap(ID3D12Device* device, UINT count) {
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  desc.NumDescriptors = count;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  return std::make_unique<DescriptorHeap>(device, desc);
}

std::unique_ptr<DescriptorHeap> MakeDsvHeap(ID3D12Device* device, UINT count) {
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  desc.NumDescriptors = count;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 0;

  return std::make_unique<DescriptorHeap>(device, desc);
}

std::unique_ptr<DescriptorHeap> MakeCbvSrvUavHeap(ID3D12Device* device, UINT count) {
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = count;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  // desc.NodeMask = 0;

  return std::make_unique<DescriptorHeap>(device, desc);
}

std::unique_ptr<DescriptorHeap> MakeSamplerHeap(ID3D12Device* device, UINT count) {
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  desc.NumDescriptors = count;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  // desc.NodeMask = 0;

  return std::make_unique<DescriptorHeap>(device, desc);
}