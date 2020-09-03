#ifndef __GEOMETRY_GENERATOR_HPP__
#define __GEOMETRY_GENERATOR_HPP__

#include <cstdint>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;


class GeometryGenerator
{
public:
    /* data */
   struct  Vertex
{
    /* data */
    XMFLOAT3 _position;
    XMFLOAT3 _normal;
    XMFLOAT3 _tangentU;
    XMFLOAT2 _texCoord;


    Vertex() {}
    Vertex(const XMFLOAT3 &p, const XMFLOAT3 &n,
        const XMFLOAT3 &t, const XMFLOAT2 &uv)
    :_position(p)
    ,_normal(n)
    ,_tangentU(t)
    ,_texCoord(uv)
    {

    }

    Vertex(float px, float py, float pz,
        float nx, float ny, float nz,
        float tx, float ty, float tz,
        float u, float v)
    :_position(px, py, pz)
    ,_normal(nx, ny, nz)
    ,_tangentU(tx, ty, tz)
    ,_texCoord(u, v)
    {

    }
};

struct MeshData
{
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices32;

    std::vector<uint16_t> &getIndices16()
    {
        if(_indices16.empty())
        {
            _indices16.resize(_indices32.size());
            for(auto i = 0; i < _indices32.size(); ++i)
                _indices16[i] = static_cast<uint16_t>(_indices32[i]); 
        }

        return _indices16;
    }
    private:
        std::vector<uint16_t> _indices16;
};


public:
    GeometryGenerator(/* args */) {}
    ~GeometryGenerator() {}

    MeshData createBox(float width, float height, float depth, std::uint32_t numSubdivisions);
    MeshData createSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);
    MeshData createGeosphere(float radius, std::uint32_t numSubdivisions);
    MeshData createCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);
    MeshData createGrid(float width, float depth, uint32_t m, uint32_t n);
    MeshData createQuad(float x, float y, float w, float h, float depth);

private:
    void subdivide(MeshData &data);
    Vertex midPoint(const Vertex &v0, const Vertex& v1);
    void createCylinderTopCap(float bottomRadius, float topRadius, 
                        float height, uint32_t sliceCount, 
                        uint32_t stackCount, MeshData &data);
    void createCylinderBottomCap(float bottomRadius, float topRadius, 
                        float height, uint32_t sliceCount, 
                        uint32_t stackCount, MeshData &data);
};


#endif

// //***************************************************************************************
// // GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
// //   
// // Defines a static class for procedurally generating the geometry of 
// // common mathematical objects.
// //
// // All triangles are generated "outward" facing.  If you want "inward" 
// // facing triangles (for example, if you want to place the camera inside
// // a sphere to simulate a sky), you will need to:
// //   1. Change the Direct3D cull mode or manually reverse the winding order.
// //   2. Invert the normal.
// //   3. Update the texture coordinates and tangent vectors.
// //***************************************************************************************

// #pragma once

// #include <cstdint>
// #include <DirectXMath.h>
// #include <vector>

// class GeometryGenerator
// {
// public:

//     using uint16 = std::uint16_t;
//     using uint32 = std::uint32_t;

// 	struct Vertex
// 	{
// 		Vertex(){}
//         Vertex(
//             const DirectX::XMFLOAT3& p, 
//             const DirectX::XMFLOAT3& n, 
//             const DirectX::XMFLOAT3& t, 
//             const DirectX::XMFLOAT2& uv) :
//             Position(p), 
//             Normal(n), 
//             TangentU(t), 
//             TexC(uv){}
// 		Vertex(
// 			float px, float py, float pz, 
// 			float nx, float ny, float nz,
// 			float tx, float ty, float tz,
// 			float u, float v) : 
//             Position(px,py,pz), 
//             Normal(nx,ny,nz),
// 			TangentU(tx, ty, tz), 
//             TexC(u,v){}

//         DirectX::XMFLOAT3 _position;
//         DirectX::XMFLOAT3 Normal;
//         DirectX::XMFLOAT3 TangentU;
//         DirectX::XMFLOAT2 TexC;
// 	};

// 	struct MeshData
// 	{
// 		std::vector<Vertex> _vertices;
//         std::vector<uint32> _indices32;

//         std::vector<uint16>& GetIndices16()
//         {
// 			if(mIndices16.empty())
// 			{
// 				mIndices16.resize(_indices32.size());
// 				for(size_t i = 0; i < _indices32.size(); ++i)
// 					mIndices16[i] = static_cast<uint16>(_indices32[i]);
// 			}

// 			return mIndices16;
//         }

// 	private:
// 		std::vector<uint16> mIndices16;
// 	};

// 	///<summary>
// 	/// Creates a box centered at the origin with the given dimensions, where each
//     /// face has m rows and n columns of vertices.
// 	///</summary>
//     MeshData createBox(float width, float height, float depth, uint32 numSubdivisions);

// 	///<summary>
// 	/// Creates a sphere centered at the origin with the given radius.  The
// 	/// slices and stacks parameters control the degree of tessellation.
// 	///</summary>
//     MeshData createSphere(float radius, uint32 sliceCount, uint32 stackCount);

// 	///<summary>
// 	/// Creates a geosphere centered at the origin with the given radius.  The
// 	/// depth controls the level of tessellation.
// 	///</summary>
//     MeshData createGeosphere(float radius, uint32 numSubdivisions);

// 	///<summary>
// 	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
// 	/// The bottom and top radius can vary to form various cone shapes rather than true
// 	// cylinders.  The slices and stacks parameters control the degree of tessellation.
// 	///</summary>
//     MeshData createCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

// 	///<summary>
// 	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
// 	/// at the origin with the specified width and depth.
// 	///</summary>
//     MeshData createGrid(float width, float depth, uint32 m, uint32 n);

// 	///<summary>
// 	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
// 	///</summary>
//     MeshData createQuad(float x, float y, float w, float h, float depth);

// private:
// 	void subdivide(MeshData& meshData);
//     Vertex midPoint(const Vertex& v0, const Vertex& v1);
//     void buildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
//     void buildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
// };

