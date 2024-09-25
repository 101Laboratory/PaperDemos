#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class ReflectiveShadowMap {
public:

  
private:
  Microsoft::WRL::ComPtr<ID3D12Resource> depth_;
  Microsoft::WRL::ComPtr<ID3D12Resource> normal_;
  Microsoft::WRL::ComPtr<ID3D12Resource> flux_;
};
