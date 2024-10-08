#pragma once

#include "UploadBuffer.h"

template<typename T>
using ConstantBuffer = UploadBuffer<T, 256>;

