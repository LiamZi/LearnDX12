#ifndef __UPLOAD_BUFFER_HPP__
#define __UPLOAD_BUFFER_HPP__

#include <Common.hpp>

template<typename T>
class UploadBuffer
{
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> _uploadBuffer;
    BYTE *_mappedData = nullptr;
    uint32_t _elementByteSize = 0;

    bool _isConstantBuffer = false;
    
public:
    UploadBuffer(ID3D12Device *device, uint32_t elementCount, bool isConstantBuffer)
    :_isConstantBuffer(isConstantBuffer) 
    {
        _elementByteSize = sizeof(T);
        if(_isConstantBuffer) _elementByteSize = Nix::Utils::calcConstantBufferSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uint64_t(_elementByteSize * elementCount)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&_uploadBuffer)
        ));

        ThrowIfFailed(_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_mappedData)));
    }

    UploadBuffer(const UploadBuffer &rhs) = delete;
    UploadBuffer &operator = (const UploadBuffer &rhs) = delete;
    ~UploadBuffer() 
    {
        if(_uploadBuffer != nullptr) _uploadBuffer->Unmap(0, nullptr);
        _uploadBuffer = nullptr;
    }

    ID3D12Resource *Resource() const 
    {
        return _uploadBuffer.Get();
    }

    void copyData(int elementIndex, const T &data)
    {
        memcpy(&_mappedData[elementIndex * _elementByteSize], &data, sizeof(T));
    }
};

#endif