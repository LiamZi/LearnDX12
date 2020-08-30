#ifndef __COMMON_HPP__
#define __COMMON_HPP__


#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>

#include "../ThirdPart/d3dx12.h"
#include "../ThirdPart/Nix/Utils/Utils.hpp"
#include "../ThirdPart/Nix/Utils/UploadBuffer.hpp"
#include "../ThirdPart/Nix/Math/MathHelper.h"

#define MULTIFLIHGT 0

constexpr uint32_t MaxFlightCount = 3;
#endif