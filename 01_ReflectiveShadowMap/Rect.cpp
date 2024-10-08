#include "Rect.h"

using namespace DirectX;

std::vector<Vertex> RectXZVertices(const RectXZ& rect) {
  float halfX = 0.5f * rect.spanX;
  float halfZ = 0.5f * rect.spanZ;
  XMFLOAT3 y = {0.f, 1.f, 0.f};

  Vertex v0;
  v0.pos = {-halfX, 0.f, halfZ};
  v0.normal = y;

  Vertex v1;
  v1.pos = {halfX, 0.f, halfZ};
  v1.normal = y;

  Vertex v2;
  v2.pos = {-halfX, 0.f, -halfZ};
  v2.normal = y;

  Vertex v3;
  v3.pos = {halfX, 0.f, -halfZ};
  v3.normal = y;

  std::vector<Vertex> vertices;
  vertices.push_back(v0);
  vertices.push_back(v1);
  vertices.push_back(v2);
  vertices.push_back(v3);
  return vertices;
}

std::vector<std::int16_t> RectXZIndices(const RectXZ& rect) {
  return {0, 1, 2, 2, 1, 3};
}