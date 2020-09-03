#ifndef __RENDER_ITEM_HPP__
#define __RENDER_ITEM_HPP__

#include <d3d12.h>
#include <DirectXMath.h>
#include "../Math/MathHelper.h"
#include "MeshGeometry.hpp"


using namespace DirectX;
using namespace Nix;

// class MeshGeometry;

const int MAXFLIGHTRESOURCE = 3;

enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class RenderItem
{
public:
    /* data */
    XMFLOAT4X4 _world = MathHelper::Identity4x4();

    MeshGeometry *_geo = nullptr;

   D3D12_PRIMITIVE_TOPOLOGY _primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    uint32_t _indexCount = 0;
    uint32_t _startIndexLocation = 0;
    uint32_t _objContantBufferIndex = -1;
    int _baseVertexLocation = 0;
    int _numFramesDirty = MAXFLIGHTRESOURCE;

public:
    RenderItem(/* args */) {}
    ~RenderItem() {}
};


#endif