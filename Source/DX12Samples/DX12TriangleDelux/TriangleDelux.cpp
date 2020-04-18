#include "TriangleDelux.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "../ThirdPart/d3dx12.h"
#include <Nix/IO/Archive.h>


bool DeviceDX12::initialize()
{
    uint32_t dxgiFactoryFlags = 0;
#ifdef _DEBUG
    {
        if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debugController)))) {
            _debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    HRESULT result = 0;

    result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_factory));
    if(FAILED(result)) return false;

    //get a hardware adapter;

    for(UINT adpaterIndex = 0; DXGI_ERROR_NOT_FOUND != _factory->EnumAdapters1(adpaterIndex, &_hardwareAdapter); ++adpaterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        _hardwareAdapter->GetDesc1(&desc);
        if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if(SUCCEEDED(D3D12CreateDevice(_hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) 
            break;
    }

    //create device with the adapter

    result = D3D12CreateDevice(_hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
    if(FAILED(result)) return false;


    //craete graphics command allocator;
    for(uint32_t i = 0; i < MaxFlightCount; ++i) {
        auto currCommandAllocator = _graphicsCommandAllocator[i];
        if(!currCommandAllocator) continue;
        _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&currCommandAllocator));
        currCommandAllocator->Reset();

        //create command list
        auto currCommandList = _graphicsCommandLists[i];
        if(!currCommandList) continue;
        _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, currCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&currCommandList));
        currCommandList->Close();

        //create fence and init fence values;
        _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_graphicsFences[i]));
        _graphicsFenceValues[i] = 0;
    }

    //create graphics fence event
    _graphicsFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr); 

    //create graphics command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    {
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    }

    result = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_graphicsCommandQueue));
    if(FAILED(result)) return false;

    //crate copy command queue, command list, commnad allocator, fence, fence event
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    result = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_uploadQueue));
    if(FAILED(result)) return false;

    result = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_uploadCommandAllocator));
    if(result) return false;

    result = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _uploadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_uploadCommandList));
    if(FAILED(result)) return false;
    _uploadCommandList->Close();
    _uploadFenceValue = 0;
    _uploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return true;
}

void DeviceDX12::executeCommand(ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    ID3D12CommandList *lists[] = { commandList.Get() };
    _graphicsCommandQueue->ExecuteCommandLists(1, lists);
    auto result = _graphicsCommandQueue->Signal(_graphicsFences[_flightIndex].Get(), _graphicsFenceValues[_flightIndex]);
    if(result) _running = false;
}

ComPtr<IDXGISwapChain3> DeviceDX12::createSwapChian(HWND hwnd, uint32_t width, uint32_t height)
{
    ComPtr<IDXGISwapChain1> swapChian;
    DXGI_SWAP_CHAIN_DESC1 swapChianDesc = {};
    {
        swapChianDesc.BufferCount = MaxFlightCount;
        swapChianDesc.Width = width;
        swapChianDesc.Height = height;
        swapChianDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChianDesc.BufferUsage = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChianDesc.SampleDesc.Count = 1;
    }

    // Swap chain needs the queue so that it can force a flush on it.

    auto lh = _graphicsCommandQueue.Get();
    auto result = _factory->CreateSwapChainForHwnd(lh, hwnd, &swapChianDesc, nullptr, nullptr, &swapChian);
    if(FAILED(result)) {
        if(result == DXGI_ERROR_DEVICE_REMOVED) result = _device->GetDeviceRemovedReason();
        return nullptr;
    }

    result = _factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if(FAILED(result)) return nullptr;

    ComPtr<IDXGISwapChain3> finalSwapChain;
    swapChian.As(&finalSwapChain);
    return finalSwapChain;

    return finalSwapChain;
}

void DeviceDX12::waitForFlight(uint32_t flight)
{
    if(_graphicsFences[flight]->GetCompletedValue() < _graphicsFenceValues[flight]) {
        auto result = _graphicsFences[flight]->SetEventOnCompletion(_graphicsFenceValues[flight], _graphicsFenceEvent);
        if(FAILED(result)) _running = false;
    }

    WaitForSingleObject(_graphicsFenceEvent, INFINITE);
}


ComPtr<ID3D12GraphicsCommandList> DeviceDX12::onTick(uint64_t dt, uint32_t flightIndex)
{
    _flightIndex = flightIndex;
    ++_graphicsFenceValues[flightIndex];
    HRESULT result;
    auto &cmdAllocator = _graphicsCommandAllocator[flightIndex];
    auto &cmdList = _graphicsCommandLists[flightIndex];
    cmdAllocator->Reset();
    result = cmdList->Reset(cmdAllocator.Get(), nullptr);
    if(FAILED(result)) return nullptr;

    return cmdList;
}

void DeviceDX12::flushGraphicsQueue()
{
    for(uint32_t index = 0; index < MaxFlightCount; ++index) {
        ++_graphicsFenceValues[index];
        auto result = _graphicsCommandQueue->Signal(_graphicsFences[index].Get(), _graphicsFenceValues[index]);
        if(FAILED(result)) _running = false;

        WaitForSingleObject(_graphicsFenceEvent, INFINITE);
    }
}

ComPtr<ID3D12Resource> DeviceDX12::createVertexBuffer(const void *data, size_t length)
{
    D3D12_HEAP_PROPERTIES heapProps;
    {
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CreationNodeMask = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
    }

    ComPtr<ID3D12Resource> buffer = nullptr;
    auto result = _device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(length),
                                                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer)
                                                );
    if(FAILED(result)) return nullptr;

    buffer->SetName(L"VertexBuffer");
    uploadBuffer(buffer, data, length);
    return buffer;
}


void DeviceDX12::uploadBuffer(ComPtr<ID3D12Resource> buffer, const void *data, size_t length)
{
    HRESULT result = 0;
    
    //create staging buffer
    ComPtr<ID3D12Resource> stagingBuffer;

    //upload heap
    _device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(length),
        D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr, 
        IID_PPV_ARGS(&stagingBuffer)
    );

    stagingBuffer->SetName(L"triangle vertex buffer staging buffer.");
    void *mappedPtr = nullptr;
    stagingBuffer->Map(0, nullptr, &mappedPtr);

    //store vertex buffer in staging buffer;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT resouceFootprint;
    {
        D3D12_SUBRESOURCE_DATA vertextSubResouceData = {};
        {
            vertextSubResouceData.pData = reinterpret_cast<const BYTE *>(data);
            vertextSubResouceData.RowPitch = length;
            vertextSubResouceData.SlicePitch = length;
        }

        uint32_t numRows[1];
        uint64_t rowSizesInBytes[1];
        uint64_t bytesTotal;
        _device->GetCopyableFootprints(&buffer->GetDesc(),0, 1, 
                                    0, &resouceFootprint, numRows, 
                                    rowSizesInBytes, &bytesTotal);
    }

    memcpy(mappedPtr, data, length);
    stagingBuffer->Unmap(0, nullptr);

    _uploadCommandAllocator->Reset();
    _uploadCommandList->Reset(_uploadCommandAllocator.Get(), nullptr);
    _uploadCommandList->CopyBufferRegion(buffer.Get(), 0, stagingBuffer.Get(), 0, length);
    auto barrierDesc = CD3DX12_RESOURCE_BARRIER::Transition( buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
    _uploadCommandList->ResourceBarrier(1, &barrierDesc);

    _uploadCommandList->Close();

    ID3D12CommandList *lists[] = { _uploadCommandList.Get() };

    _uploadQueue->ExecuteCommandLists(1, lists);
    _uploadQueue->Signal(_uploadFence.Get(), ++_uploadFenceValue);

    //wait for upload operation
    if(_uploadFence->GetCompletedValue() < _uploadFenceValue) {
        result = _uploadFence->SetEventOnCompletion(_uploadFenceValue, _uploadFenceEvent);
        if(FAILED(result)) assert(false);

        WaitForSingleObject(_uploadFenceEvent, INFINITE);
    }
}

ComPtr<ID3D12Resource> DeviceDX12::createTexture()
{
    ComPtr<ID3D12Resource> texture = nullptr;
    D3D12_RESOURCE_DESC textureDesc = {};
    {
        auto &desc = textureDesc;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Width = 8;
        desc.Height = 8;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        desc.DepthOrArraySize = 1;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
    }

    CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

    //create a texture object;
    auto result = _device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, 
                                                &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                nullptr, IID_PPV_ARGS(&texture));

    if(FAILED(result)) return nullptr;

    UINT64 requiredSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint;
    _device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footPrint, nullptr, nullptr, &requiredSize);
    //create staging buffer
    ComPtr<ID3D12Resource> stagingBuffer = nullptr;
    uint32_t pixelData[] = {
        0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff,
		0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff,
		0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff,
		0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff,
		0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff,
		0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff,
		0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff,
		0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff,
    };

    _device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                    D3D12_HEAP_FLAG_NONE,
                                    &CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    nullptr, IID_PPV_ARGS(&stagingBuffer));
    stagingBuffer->SetName(L"simple texture staging buffer. ");

    void *mappedPtr = nullptr;
    stagingBuffer->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, pixelData, sizeof(pixelData));
    stagingBuffer->Unmap(0, nullptr);

    //copy pixel data into the texture object;
    _uploadCommandAllocator->Reset();
    _uploadCommandList->Reset(_uploadCommandAllocator.Get(), nullptr);

    {
        CD3DX12_TEXTURE_COPY_LOCATION dst(texture.Get(), 0);
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint;
        _device->GetCopyableFootprints(&texture->GetDesc(), 0, 1, 0, 
                                    &footPrint, nullptr, nullptr, nullptr);
        CD3DX12_TEXTURE_COPY_LOCATION src(stagingBuffer.Get(), footPrint);
        _uploadCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        //resource barrier;
        auto barrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        _uploadCommandList->ResourceBarrier(1, &barrierDesc);
    }
    
    _uploadCommandList->Close();
    ID3D12CommandList *lists[] = { _uploadCommandList.Get() };
    _uploadQueue->ExecuteCommandLists(1, lists);
    _uploadQueue->Signal(_uploadFence.Get(), ++_uploadFenceValue);

    //wait for upload operation
    if(_uploadFence->GetCompletedValue() < _uploadFenceValue) {
        result = _uploadFence->SetEventOnCompletion(_uploadFenceValue, _uploadFenceEvent);
        if(FAILED(result))  { assert(false); return nullptr; }

        WaitForSingleObject(_uploadFenceEvent, INFINITE); 
    }

    return texture;
}

DeviceDX12::operator Microsoft::WRL::ComPtr<ID3D12Device> () const {
    return _device;
}


TriangleDelux::TriangleDelux()
{

}

TriangleDelux::~TriangleDelux()
{
    
}


bool TriangleDelux::initialize(void *wnd, Nix::IArchive *arch) 
{
    return true;
}


void TriangleDelux::resize(uint32_t width, uint32_t height)
{

}

void TriangleDelux::release()
{

}

void TriangleDelux::tick()
{

}

const char *TriangleDelux::title()
{
    return " Triangle Delux For DX12 ";
}

uint32_t TriangleDelux::rendererType() 
{
    return 0;
}

TriangleDelux theapp;

NixApplication* getApplication() {
    return &theapp;
}

