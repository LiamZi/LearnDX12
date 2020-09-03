#ifndef __MESH_GEOMETRY_HPP__
#define __MESH_GEOMETRY_HPP__

#include <Common.hpp>

using namespace Microsoft::WRL;

struct SubMeshGeometry
{
    uint32_t _indexCount = 0;
    uint32_t _startIndexLocation = 0;
    int32_t _baseVertexLocation = 0;

    DirectX::BoundingBox _bounds;
};


class MeshGeometry
{
private:
    std::string _name;
    ComPtr<ID3DBlob> _vertexBufferCPU = nullptr;
    ComPtr<ID3DBlob> _indexBufferCPU = nullptr;

    ComPtr<ID3D12Resource> _vertexBufferGPU = nullptr;
    ComPtr<ID3D12Resource> _indexBufferGPU = nullptr;

    ComPtr<ID3D12Resource> _vertexBufferUploader = nullptr;
    ComPtr<ID3D12Resource> _indexBufferUploader = nullptr;

    //data about the buffers.
    uint32_t _vertexStride = 0;
    uint32_t _vertexBufferSize = 0;
    uint32_t _indexBufferSize = 0;
    DXGI_FORMAT _indexFormat;

    std::unordered_map<std::string, SubMeshGeometry> _drawArgs;
public:
    MeshGeometry(/* args */):_name("")
    ,_indexFormat(DXGI_FORMAT_B8G8R8A8_UNORM)
    {
        
    };
    ~MeshGeometry() {}

public:

    void setDrawArgsElement(const std::string &name, SubMeshGeometry &geometry)
    {
        _drawArgs[name] = geometry;
    }

    SubMeshGeometry *getDrawArgsSubMeshByName(const std::string &name)
    {
        // return _drawArgs[name]; 
        auto iter = _drawArgs.find(name);
        if(iter != _drawArgs.end())  
            return &(iter->second);

        return nullptr;
    }

    std::unordered_map<std::string, SubMeshGeometry> &getDrawArgs()
    {
        return _drawArgs;
    }

    void setName(const std::string &name)
    {
        _name = name;
    }

    std::string &getName()
    {
        return _name;
    }

    void setIndexFormat(const DXGI_FORMAT format)
    {
        _indexFormat = format;
    }

    DXGI_FORMAT getIndexFormat()
    {
        return _indexFormat;
    }

    void setVertexStride(uint32_t stride)
    {
        _vertexStride = stride;
    }

    uint32_t getVertexStride()
    {
        return _vertexStride;
    }
    
    void setVertexBufferCPU(const ComPtr<ID3DBlob> buffer)
    {
        _vertexBufferCPU = buffer;
    }

    ComPtr<ID3DBlob> &getVertexBufferCPU()
    {
        return _vertexBufferCPU;
    }

    void setVertexBufferGPU(const ComPtr<ID3D12Resource> buffer)
    {
        _vertexBufferGPU = buffer;
    }

    ComPtr<ID3D12Resource> &getVertexBufferGPU()
    {
        return _vertexBufferGPU;
    }

    void setIndexBufferCPU(const ComPtr<ID3DBlob> buffer)
    {
        _indexBufferCPU = buffer;
    }

    ComPtr<ID3DBlob> &getIndexBufferCPU()
    {
        return _indexBufferCPU;
    }

    void setIndexBufferGPU(const ComPtr<ID3D12Resource> buffer)
    {
        _indexBufferGPU = buffer;
    }

    ComPtr<ID3D12Resource> &getIndexBufferGPU()
    {
        return _indexBufferGPU;
    }

    void setVertexBufferUploader(const ComPtr<ID3D12Resource> buffer)
    {
        _vertexBufferUploader = buffer;
    }

    ComPtr<ID3D12Resource> &getVertexBufferUploader()
    {
        return _vertexBufferUploader;
    }

    void setIndexBufferUploader(const ComPtr<ID3D12Resource> buffer)
    {
        _indexBufferUploader = buffer;
    }

    ComPtr<ID3D12Resource> &getIndexBufferUploader()
    {
        return _indexBufferUploader;
    }

    void setVertexBufferSize(const uint32_t size)
    {
        _vertexBufferSize = size;
    }

    uint32_t getVertexBufferSize()
    {
        return _vertexBufferSize;
    }

    void setIndexBufferSize(const uint32_t size)
    {
        _indexBufferSize = size;
    }

    uint32_t getIndexBufferSize()
    {
        return _indexBufferSize;
    }


    D3D12_VERTEX_BUFFER_VIEW vertexBufferView() const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = _vertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = _vertexStride;
        vbv.SizeInBytes = _vertexBufferSize;

        return vbv;
    }


    D3D12_INDEX_BUFFER_VIEW indexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = _indexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = _indexFormat;
        ibv.SizeInBytes = _indexBufferSize;

        return ibv;
    }


    void disposeUploaders()
    {
        _vertexBufferUploader = nullptr;
        _indexBufferUploader = nullptr;
    }

};

#endif