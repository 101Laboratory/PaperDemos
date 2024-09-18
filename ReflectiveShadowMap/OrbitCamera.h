#pragma once
#include "Camera.h"

void CameraSetOrbitWithOffset(Camera* cam,
                              DirectX::XMFLOAT3 focus,
                              float deltaR,
                              float deltaPhiDeg,
                              float deltaThetaDeg,
                              float minR = 1.0f);
