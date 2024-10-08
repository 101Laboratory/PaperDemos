#include "Color.h"

#include "MathUtils.h"

RgbaColorF::operator RgbaColorI() const {
  unsigned char r = Clamp(static_cast<unsigned char>(this->r * 255.0), 0, 255);
  unsigned char g = Clamp(static_cast<unsigned char>(this->g * 255.0), 0, 255);
  unsigned char b = Clamp(static_cast<unsigned char>(this->b * 255.0), 0, 255);
  unsigned char a = Clamp(static_cast<unsigned char>(this->a * 255.0), 0, 255);
  return RgbaColorI{r, g, b, a};
}

RgbaColorI::operator RgbaColorF() const {
  float r = Clamp(this->r / 255.0f, 0.0, 1.0);
  float g = Clamp(this->g / 255.0f, 0.0, 1.0);
  float b = Clamp(this->b / 255.0f, 0.0, 1.0);
  float a = Clamp(this->a / 255.0f, 0.0, 1.0);
  return RgbaColorF{r, g, b, a};
}
