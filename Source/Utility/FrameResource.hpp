#ifndef __FRAME_RESOURCE_HPP__
#define __FRAME_RESOURCE_HPP__

#include <wrl.h>
#include <memory>

#include <PassConstants.hpp>
#include <ConstantObject.hpp>
#include <Vertex.hpp>

class FrameResource
{
public:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdListAllocation;

    std::unique_ptr<UploadBuffer<PassConstants>> _passConstantBuffer = nullptr;
    std::unique_ptr<UploadBuffer<ConstantObject>> _objectConstantBuffer = nullptr;
    std::unique_ptr<UploadBuffer<Vertex>> _waveVertexBuffer = nullptr;

    uint64_t _fences = 0;

public:
    FrameResource(ID3D12Device *device, uint32_t passCount, uint32_t objectCount, uint32_t waveVertexCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(_cmdListAllocation.GetAddressOf())));
    
        _passConstantBuffer = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
        _objectConstantBuffer = std::make_unique<UploadBuffer<ConstantObject>>(device, objectCount, true);
        _waveVertexBuffer = std::make_unique<UploadBuffer<Vertex>>(device, waveVertexCount, false);
    };
    FrameResource(const FrameResource &rhs) = delete;
    FrameResource &operator=(const FrameResource &rhs) = delete;

    ~FrameResource() {};

   
};


#endif