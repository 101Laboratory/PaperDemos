#pragma once

#include "Model.h"

struct RectXZ {
  float spanX = 1.f;
  float spanZ = 1.f;
};


std::vector<Vertex> RectXZVertices(const RectXZ& rect);

std::vector<std::int16_t> RectXZIndices(const RectXZ& rect);

