#ifndef __GEOMETRY_GENERATOR_HPP__
#define __GEOMETRY_GENERATOR_HPP__

#include <cstdint>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

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
    private:
        std::vector<uint16_t> _indices16;

    public:
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
};

class GeometryGenerator
{
public:
    /* data */
   


    
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