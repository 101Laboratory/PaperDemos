// ReSharper disable CppInconsistentNaming
#pragma once
#include <DirectXMath.h>

#include <tuple>

DirectX::XMVECTOR ToXMVector(DirectX::XMFLOAT3 v);

DirectX::XMVECTOR ToXMVector(DirectX::XMFLOAT4 v);

DirectX::XMFLOAT3 ToXMFloat3(DirectX::FXMVECTOR v);

DirectX::XMFLOAT4 ToXMFloat4(DirectX::FXMVECTOR v);

DirectX::XMMATRIX ToXMMatrix(DirectX::XMFLOAT4X4 m);

DirectX::XMFLOAT4X4 ToXMFloat4x4(DirectX::XMMATRIX m);


double Clamp(double x, double lo, double hi);

int Clamp(int x, int lo, int hi);

double Wrap(double x, double lo, double hi);

float MapRange(float val, float l0, float r0, float l1, float r1);

std::tuple<float, float, float> ToCartesian(float r, float phi, float theta);

std::tuple<float, float, float> ToSpherical(float x, float y, float z);

DirectX::XMFLOAT4X4 Float4x4Identity();

DirectX::XMFLOAT4X4 Float4x4Inverse(DirectX::XMFLOAT4X4 m);