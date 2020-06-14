#include "TriangleDelux.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>

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

    //create graphics command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    {
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    }


    result = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_graphicsCommandQueue));
    if(FAILED(result)) return false;

    //craete graphics command allocator;
    for(uint32_t i = 0; i < MaxFlightCount; ++i) {
        _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_graphicsCommandAllocator[i]));
        _graphicsCommandAllocator[i]->Reset();

        //create command list
        _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _graphicsCommandAllocator[i].Get(), nullptr, IID_PPV_ARGS(&_graphicsCommandLists[i]));
        _graphicsCommandLists[i]->Close();

        //create fence and init fence values;
        _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_graphicsFences[i]));
        _graphicsFenceValues[i] = 0;
    }

    //create graphics fence event
    _graphicsFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr); 

    //crate copy command queue, command list, commnad allocator, fence, fence event
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    result = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_uploadQueue));
    if(FAILED(result)) return false;


    result = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_uploadCommandAllocator));
    if(result) return false;

    result = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _uploadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_uploadCommandList));
    if(FAILED(result)) return false;
    _uploadCommandList->Close();
    _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_uploadFence));
    _uploadFenceValue = 0;
    _uploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return true;
}

void DeviceDX12::executeCommand(ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    ID3D12CommandList *lists[] = { commandList.Get() };
    _graphicsCommandQueue->ExecuteCommandLists(1, lists);
    auto result = _graphicsCommandQueue->Signal(_graphicsFences[_flightIndex].Get(), _graphicsFenceValues[_flightIndex]);
    if(FAILED(result)) _running = false;
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
        // swapChianDesc.BufferUsage = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChianDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChianDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
    // if(_graphicsFences[flight]->GetCompletedValue() < _graphicsFenceValues[flight]) {
    //     auto result = _graphicsFences[flight]->SetEventOnCompletion(_graphicsFenceValues[flight], _graphicsFenceEvent);
    //     if(FAILED(result)) _running = false;
    // }

    // WaitForSingleObject(_graphicsFenceEvent, INFINITE);

    if (_graphicsFences[flight]->GetCompletedValue() < _graphicsFenceValues[flight]) {
		HRESULT rst;
		rst = _graphicsFences[flight]->SetEventOnCompletion(_graphicsFenceValues[flight], _graphicsFenceEvent);
		if (FAILED(rst)) {
			_running = false;
		}
		WaitForSingleObject(_graphicsFenceEvent, INFINITE);
	}
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
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
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
	uint32_t pixelData[8][64] = {
        { 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff},
        { 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff},
        { 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff},
        { 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff},
        { 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff},
        { 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff},
        { 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff},
        { 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff, 0x000000ff, 0xffffffff}		
	};

    _device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
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
    if(!wnd || !arch) return false;

    _archive = arch;

    if(!this->_device.initialize()) return false;

    this->_hwnd = wnd;

    HRESULT result = 0;

    ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

    this->_simpleTexture = _device.createTexture();

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    {
        srvHeapDesc.NumDescriptors = 2;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        result = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_pipelineDescriptorHeap));
		if( FAILED(result)) return false;
	}

    auto cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    result = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                        D3D12_HEAP_FLAG_NONE,
                                        &CD3DX12_RESOURCE_DESC::Buffer(256 * 2),
                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                        nullptr,
                                        IID_PPV_ARGS(&_constantBuffer));
    if(FAILED(result)) return false;

    void *mappedPtr = nullptr;
    _constantBuffer->Map(0, nullptr, &mappedPtr);
    float offset[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    memcpy(mappedPtr, &offset, sizeof(offset));
    _constantBuffer->Unmap(0, nullptr);

    _constantBuffer->SetName(L"Contant Buffer");

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbufferViewDesc = {};
    {
        cbufferViewDesc.BufferLocation = _constantBuffer->GetGPUVirtualAddress();
        cbufferViewDesc.SizeInBytes = 256 * 2;
    }

    // 在 heap 上创建需要使用 CPU handle，但是在渲染绑定的时候绑定的是 GPU handle
    // CD3DX12_CPU_DESCRIPTOR_HANDLE pipelineCPUDescriptorHandlerStart(_pipelineDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    // CD3DX12_GPU_DESCRIPTOR_HANDLE pipelineGPUDescriptorHandlerStart(_pipelineDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE pipelineCPUDescriptorHandlerStart( _pipelineDescriptorHeap->GetCPUDescriptorHandleForHeapStart() );
	CD3DX12_GPU_DESCRIPTOR_HANDLE pipelineGPUDescriptorHandlerStart( _pipelineDescriptorHeap->GetGPUDescriptorHandleForHeapStart() );
    //save gpu handle

    _simpleTextureViewGPUDescriptorHandle = pipelineGPUDescriptorHandlerStart;
    _constantBufferGPUDescriptorHandle= pipelineGPUDescriptorHandlerStart.Offset(cbvSrvDescriptorSize);


    D3D12_SHADER_RESOURCE_VIEW_DESC textureResourceViewDesc = {};
    {
        textureResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        textureResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        textureResourceViewDesc.Texture2D.MipLevels = 1;
    }

    //creator texture view
    device->CreateShaderResourceView(_simpleTexture.Get(), &textureResourceViewDesc, pipelineCPUDescriptorHandlerStart);

    //creator constant buffer view
    device->CreateConstantBufferView(&cbufferViewDesc, pipelineCPUDescriptorHandlerStart.Offset(cbvSrvDescriptorSize));

	CD3DX12_DESCRIPTOR_RANGE1 vertexDescriptorRanges[1];{
		// vertex constant buffer / uniform
		vertexDescriptorRanges[0].Init(
			D3D12_DESCRIPTOR_RANGE_TYPE_CBV,			// type
			1,											// descriptor count
			0,											// base register
			0,											// register space
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC 	// descriptor range flag
		);
	}

    CD3DX12_DESCRIPTOR_RANGE1 pixelDescriptorRanges[2];
    {
		// fragment texture
		pixelDescriptorRanges[0].Init(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,				// descriptor type
			1,												// descriptor count
			0,												// base shader register
			0,												// register space
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC			// descriptor data type
		);
		// fragment sampler
		pixelDescriptorRanges[1].Init(
			D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,			// descriptor type
			1,												// descriptor count
			1,												// base shader register
			0,												// register space
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE// descriptor data type
		);
	}

    constexpr int vertexRangeCount = sizeof(vertexDescriptorRanges) / sizeof(CD3DX12_DESCRIPTOR_RANGE1);
    // constexpr int vertexRangeCount = 1;
	constexpr int pixelRangeCount = sizeof(pixelDescriptorRanges) / sizeof(CD3DX12_DESCRIPTOR_RANGE1);

    CD3DX12_ROOT_PARAMETER1 rootParameters[3];{
		rootParameters[0].InitAsDescriptorTable( vertexRangeCount , &vertexDescriptorRanges[0], D3D12_SHADER_VISIBILITY_VERTEX );
		rootParameters[1].InitAsDescriptorTable( 1 , &pixelDescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL );
		rootParameters[2].InitAsDescriptorTable( 1 , &pixelDescriptorRanges[1], D3D12_SHADER_VISIBILITY_PIXEL );
	}

    D3D12_STATIC_SAMPLER_DESC sampler = {}; {
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}   


    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc {};
    {
		rootSignatureDesc.Init_1_1( 3, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );
	}


    // ComPtr<ID3DBlob> signature;
    // ComPtr<ID3DBlob> error;
    ID3DBlob *signature = nullptr;
    ID3DBlob *error = nullptr;

    //create root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if ( FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

    result = D3DX12SerializeVersionedRootSignature( &rootSignatureDesc, featureData.HighestVersion, &signature, &error);
    // result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

    if (FAILED(result)) {
		const char * msg = (const char *)error->GetBufferPointer();
		OutputDebugStringA(msg);
		return false;
	}

    // result = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_pipelineRootSignature));
    result = device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_pipelineRootSignature));
    if(FAILED(result))  { 
        return false;
    }

    ID3DBlob *vertexShader = nullptr;
    ID3DBlob *fragmentShader = nullptr;
    ID3DBlob *errorBuffer = nullptr;

    auto file = _archive->open("shaders/triangleDelux/VertexShader.hlsl", 1);
    if(!file) return false;

    result = D3DCompile(file->constData(), file->size(),
                    "VertexShader.hlsl",
                    nullptr, nullptr,
                    "VsMain", "vs_5_0", 
                    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                    0, &vertexShader, &errorBuffer);
    if(FAILED(result)) {
        OutputDebugStringA((char *)errorBuffer->GetBufferPointer());
        return false;
    }

    file->release();

    file = _archive->open("shaders/triangleDelux/FragmentShader.hlsl", 1);
    if(!file) return false;
    result = D3DCompile(file->constData(), file->size(),
                "FragmentShader.hlsl",
                nullptr, nullptr,
                "PsMain", "ps_5_0", 
                D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                0, &fragmentShader, &errorBuffer);

    if(FAILED(result)) {
        OutputDebugStringA((char *)errorBuffer->GetBufferPointer());
        return false;
    }

    D3D12_SHADER_BYTECODE vertexShaderByteCode= {};
    {
        vertexShaderByteCode.BytecodeLength = vertexShader->GetBufferSize();
        vertexShaderByteCode.pShaderBytecode = vertexShader->GetBufferPointer();
    }

    D3D12_SHADER_BYTECODE pixelShaderByteCode =  {};
    {
        pixelShaderByteCode.BytecodeLength = fragmentShader->GetBufferSize();
        pixelShaderByteCode.pShaderBytecode = fragmentShader->GetBufferPointer();
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        {
            "TEXCOORD",                                  // Semantic Name
            0,			                                // Semantic Index
            DXGI_FORMAT_R32G32_FLOAT,                   // Format
            1,                                          // Slot Index
            12,                                          // Offset
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // InputSlotClass
            0                                           // step rate
        }
    };

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {}; 
    {
        inputLayoutDesc.NumElements = 2;
        inputLayoutDesc.pInputElementDescs = &inputLayout[0];
    }

    DXGI_SAMPLE_DESC samplerDesc = {};
    {
        samplerDesc.Count = 1;
    }

    //create pipeline state
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
    {
        auto& desc = pipelineStateDesc;
        desc.InputLayout = inputLayoutDesc;
        desc.pRootSignature = _pipelineRootSignature;
        desc.VS = vertexShaderByteCode;
        desc.PS = pixelShaderByteCode;
        desc.SampleDesc = samplerDesc;
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = false;
        desc.DepthStencilState.StencilEnable = false;
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.SampleMask = ~0;
    }


    result = device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&_pipelineStateObject));
    if(FAILED(result)) return false;

    // a triangle
    struct  Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 uv;
    };

    Vertex vertexData[] = {
        { { 0.0f, 0.5f, 0.5f }, { 0.5f, 0.0f } },
        { { 0.5f, -0.5f, 0.5f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f } },
    };


    int vertexBufferSize = sizeof(vertexData);
       
    this->_vertexBuffer = _device.createVertexBuffer(vertexData, vertexBufferSize);
    { 
        // create vertex buffer view
		this->_vertexBufferView.BufferLocation = 0;
		this->_vertexBufferView.SizeInBytes = sizeof(vertexData);
		this->_vertexBufferView.StrideInBytes = sizeof(Vertex);
	}

   
    // create a vertex buffer view for the triangle. 
    // We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.StrideInBytes = sizeof(Vertex);
    _vertexBufferView.SizeInBytes = vertexBufferSize;

    return true;
}


void TriangleDelux::resize(uint32_t width, uint32_t height)
{
     _viewPort.TopLeftX = 0;
     _viewPort.TopLeftY = 0;
     _viewPort.Width = (float)(width);
     _viewPort.Height = (float)(height);
     _viewPort.MinDepth = 0.0f;
     _viewPort.MaxDepth = 1.0f;

     _viewScissor.left = 0;
     _viewScissor.top = 0;
     _viewScissor.right = width;
     _viewScissor.bottom = height;

    if(!this->_swapChain) {
        HRESULT result = 0;

        DXGI_MODE_DESC displayModeDesc = {}; 
        {
            displayModeDesc.Width = width;
            displayModeDesc.Height = height;
            displayModeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 不需要提前获取支持什么格式？ 对于vk来说是必走流程
        }

        DXGI_SAMPLE_DESC samplerDesc = {};
        {
            samplerDesc.Count = 1;
        }

        _swapChain = _device.createSwapChian((HWND)_hwnd, width, height);

        ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; {
            rtvHeapDesc.NumDescriptors = MaxFlightCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        }

        result = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvDescrpitorHeap));
        if(FAILED(result)) return;

        _rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // 7. create render targets & render target view
        // Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _rtvDescrpitorHeap->GetCPUDescriptorHandleForHeapStart();
        for (int i = 0; i < MaxFlightCount; i++) {
            // first we get the n'th buffer in the swap chain and store it in the n'th
            // position of our ID3D12Resource array
            HRESULT rst = _swapChain->GetBuffer( i, IID_PPV_ARGS(&_renderTargets[i]));
            _renderTargets[i]->SetName(L"swapchain render target buffer");
            if (FAILED(rst)) return;
            // the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
            device->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, { rtvHandle.ptr + _rtvDescriptorSize * i });
        }

        //create sampler heap
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        {
            samplerHeapDesc.NumDescriptors = 1;
            samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }

        result = device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&_samplerDescriptorHeap));
        if(FAILED(result)) return;

        _samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        _samplerGPUDescriptorHandle = _samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

        D3D12_SAMPLER_DESC sampler = {};
        {
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.MipLODBias = 0;
            sampler.MaxAnisotropy = 0;
            sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            sampler.BorderColor[0] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            sampler.BorderColor[1] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            sampler.BorderColor[2] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            sampler.BorderColor[3] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            sampler.MinLOD = 0.0f;
            sampler.MaxLOD = D3D12_FLOAT32_MAX;
        } 

        D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = _samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        device->CreateSampler(&sampler, samplerHandle);

    }
    else {
         ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;
         D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _rtvDescrpitorHeap->GetCPUDescriptorHandleForHeapStart();
         //wait for graphics queue
         _device.flushGraphicsQueue();

        for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
            _renderTargets[flightIndex].Reset();
        }

        HRESULT rst = _swapChain->ResizeBuffers( MaxFlightCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0 );
        if( FAILED(rst)) OutputDebugString(L"Error!");

        // get render target buffer & re-create render target view
        for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
            _swapChain->GetBuffer(flightIndex, IID_PPV_ARGS(&_renderTargets[flightIndex]));
            _renderTargets[flightIndex]->SetName(L"swapchain render target buffer");
            device->CreateRenderTargetView(_renderTargets[flightIndex].Get(), nullptr, { rtvHandle.ptr + _rtvDescriptorSize * flightIndex });
        }
    }
}

void TriangleDelux::release()
{
    for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
		_device.waitForFlight( flightIndex );
	}
	_swapChain->Release();
	_swapChain = nullptr;
	_rtvDescrpitorHeap->Release();
	_rtvDescrpitorHeap = nullptr;
	//	
    printf("destroyed");
}

void TriangleDelux::draw()
{
    if(!_swapChain) return;

    HRESULT result;

    _flightIndex = _swapChain->GetCurrentBackBufferIndex();
    _device.waitForFlight(_flightIndex);

    ComPtr<ID3D12GraphicsCommandList> commandList = _device.onTick(0, _flightIndex);
    
    //transform the render target's layout
    D3D12_RESOURCE_BARRIER barrier;
    {
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.pResource = _renderTargets[_flightIndex].Get();
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    }

    commandList->ResourceBarrier(1, &barrier);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvDescrpitorHeap->GetCPUDescriptorHandleForHeapStart(), _flightIndex, _rtvDescriptorSize);
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    //transfrom the render target's layout

    commandList->SetGraphicsRootSignature(_pipelineRootSignature);

    ID3D12DescriptorHeap* heaps[] = {
        _pipelineDescriptorHeap.Get(),
        _samplerDescriptorHeap.Get()
    };

    commandList->SetDescriptorHeaps( 2, heaps ); 

    // 第[0]个是 const buffer view descriptor
    commandList->SetGraphicsRootDescriptorTable(0, _constantBufferGPUDescriptorHandle );
    // 第[1] 个是 texture view descriptor
    commandList->SetGraphicsRootDescriptorTable(1, _simpleTextureViewGPUDescriptorHandle );
    // 第[2] 个是 sampler descriptor
    commandList->SetGraphicsRootDescriptorTable(2, _samplerGPUDescriptorHandle);

    commandList->SetPipelineState(_pipelineStateObject);
    commandList->RSSetViewports(1, &_viewPort);
    commandList->RSSetScissorRects(1, &_viewScissor);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0); //fianlly draw 3 vertices (draw the triangle)

    std::swap(barrier.Transition.StateAfter, barrier.Transition.StateBefore);
    commandList->ResourceBarrier(1, &barrier);

    result = commandList->Close();
    _device.executeCommand(commandList);

    result = _swapChain->Present(1, 0);

}

void TriangleDelux::tick(float dt)
{
    
}

char *TriangleDelux::title()
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

