#ifndef __OBJECT_CONSTATNTS_HPP__
#define __OBJECT_CONSTATNTS_HPP__

#include <DirectXMath.h>
#include "Nix/Math/MathHelper.h"

struct ConstantObject
{
   DirectX::XMFLOAT4X4 world = Nix::MathHelper::Identity4x4();
};

#endif

