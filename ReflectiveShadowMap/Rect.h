#pragma once

#include "Model.h"

struct RectXZ {
  float spanX;
  float spanZ;
};


std::vector<Vertex> RectXZVertices(const RectXZ& rect);

std::vector<std::int16_t> RectXZIndices(const RectXZ& rect);

