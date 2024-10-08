#pragma once

#include "Camera.h"

void CameraWorldOffset(Camera* cam, DirectX::XMFLOAT3 offset);

void CameraMoveForward(Camera* cam, float d);

void CameraMoveRight(Camera* cam, float d);

void CameraMoveUp(Camera* cam, float d);

void CameraLookRight(Camera* cam, float deg);

void CameraLookUp(Camera* cam, float deg);

