#ifndef __PASS_CONSTANTS_HPP__
#define __PASS_CONSTANTS_HPP__

// #include <Common.hpp>
#include <stdint.h>
#include <DirectXMath.h>
#include "Nix/Math/MathHelper.h"

using namespace Nix;

class PassConstants
{
public:
    /* data */
    DirectX::XMFLOAT4X4 _view = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _invView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _invProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _viewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _invViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 _eyePosW = { 0.0f, 0.0f, 0.0f };
   
    DirectX::XMFLOAT2 _renderTargetSize = {0.0f, 0.0f };
    DirectX::XMFLOAT2 _invRenderTargetSize = {0.0f, 0.0f };

    float _constantBufferObjPad1 = 0.0f;
    float _nearZ = 0.0f;
    float _farZ = 0.0f;
    float _totalTime = 0.0f;
    float _deltaTime = 0.0f;
    
public:
    PassConstants(/* args */) {};
    ~PassConstants() {};
};

#endif