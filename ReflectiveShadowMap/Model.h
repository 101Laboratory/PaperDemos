#pragma once
#include <string>
#include <vector>

#include "MathUtils.h"

struct Vertex {
  DirectX::XMFLOAT3 pos;
  float padding0;
  DirectX::XMFLOAT3 normal;
};

class ObjModel {
public:
  explicit ObjModel(std::string file);

  const Vertex* VerticesBegin() const { return vertices_.data(); }

  size_t VerticesByteSize() const { return vertices_.size() * sizeof(Vertex); }

  size_t VertexCount() const { return vertices_.size(); }

  const std::uint16_t* IndicesBegin() const { return indices_.data(); }

  size_t IndicesByteSize() const { return indices_.size() * sizeof(std::uint16_t); }

  size_t IndexCount() const { return indices_.size(); }

private:
  std::vector<Vertex> vertices_;
  std::vector<std::uint16_t> indices_;
};
