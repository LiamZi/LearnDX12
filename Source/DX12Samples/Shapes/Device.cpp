#include "Device.hpp"


bool Device::initialize()
{
#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory)));

    auto hardWareRes = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_d3dDevice));
    if(FAILED(hardWareRes))
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(
                    D3D12CreateDevice(warpAdapter.Get(), 
                                        D3D_FEATURE_LEVEL_11_0, 
                                        IID_PPV_ARGS(&_d3dDevice))
                    );
    }


    ThrowIfFailed(_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

    _rtvDescSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    _dsvDescSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    _cbvSrvUavDescSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    qualityLevels.Format = _renderTargetFormat;
    qualityLevels.SampleCount = 4;
    qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    qualityLevels.NumQualityLevels = 0;

    ThrowIfFailed(_d3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &qualityLevels,
        sizeof(qualityLevels)
    ));

    _4xMsaaQuality = qualityLevels.NumQualityLevels;
    assert(_4xMsaaQuality > 0 && "Unexpected MSAA qulity level.");

#ifdef _DEBUG
    logAdapters();
#endif

    createCommandObjects();
    createSwapChain();
    createRtvAndDsvDescriptorHeaps();

    return true;
}

void Device::createCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    {
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(_d3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(_graphicsCmdQueue.GetAddressOf())));

        ThrowIfFailed(_d3dDevice->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(_graphicsCmdListAlloc.GetAddressOf())));

        ThrowIfFailed(_d3dDevice->CreateCommandList(
                    0, 
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    _graphicsCmdListAlloc.Get(),
                    nullptr,
                    IID_PPV_ARGS(_graphicsCmdList.GetAddressOf())));

        _graphicsCmdList->Close();
    }
}

void Device::createSwapChain()
{
    _swapChain.Reset();

    DXGI_SWAP_CHAIN_DESC desc;
    desc.BufferDesc.Width = _width;
    desc.BufferDesc.Height = _height;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = _renderTargetFormat;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
    desc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount =  MaxFlightCount;
    desc.OutputWindow = (HWND)_hwnd;
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(_dxgiFactory->CreateSwapChain(
        _graphicsCmdQueue.Get(),
        &desc,
        _swapChain.GetAddressOf()
    ));
}

void Device::flushCommandQueue()
{
    _currentFence++;
    ThrowIfFailed(_graphicsCmdQueue->Signal(_fence.Get(), _currentFence));

    if(_fence->GetCompletedValue() < _currentFence)
    {
        auto eventHandle =  CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(_fence->SetEventOnCompletion(_currentFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void Device::onResize(const uint32_t width, const uint32_t height)
{
    assert(_d3dDevice);
    assert(_swapChain);
    assert(_graphicsCmdListAlloc);
    

    //flush before changing any resources.
    flushCommandQueue();

    //reset command list;
    ThrowIfFailed(_graphicsCmdList->Reset(_graphicsCmdListAlloc.Get(), nullptr));

    //release the previous resource we will be recreating.
    for (size_t i = 0; i < MaxFlightCount; i++)
    {
        _renderTargets[i].Reset();
    }

    _depthStencilTargets.Reset();

    //resize the swap chian
    swapChainResizeBuffer(width, height);
    _currFlightIndex = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint32_t i = 0; i < MaxFlightCount; i++)
    {
        ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i])));
        _renderTargets[i]->SetName(L"swapchain render target buffer");
        _d3dDevice->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, _rtvDescSize);
    }


    //create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC dsDesc;
    dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsDesc.Alignment = 0;
    dsDesc.Width = width;
    dsDesc.Height = height;
    dsDesc.DepthOrArraySize = 1;
    dsDesc.MipLevels = 1;

    dsDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    dsDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
    dsDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
    dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = _depthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(_d3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &dsDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(_depthStencilTargets.GetAddressOf())
    ));

    //crate descriptor to mip level 0 of entire resource using the format of the resource.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = _depthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    _d3dDevice->CreateDepthStencilView(_depthStencilTargets.Get(), &dsvDesc, depthStencilView());

    //transition the resource from its inital state to be used as a depth buffer.
    _graphicsCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_depthStencilTargets.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    

    //execute the resize commands.
    ThrowIfFailed(_graphicsCmdList->Close());
    ID3D12CommandList *cmds[] = { _graphicsCmdList.Get() };
    _graphicsCmdQueue->ExecuteCommandLists(_countof(cmds), cmds);

    //wait until resize is complete.
    flushCommandQueue();


    _viewPort.TopLeftX = 0;
    _viewPort.TopLeftY = 0;
    _viewPort.Width = static_cast<float>(width);
    _viewPort.Height = static_cast<float>(height);
    _viewPort.MinDepth = 0.0f;
    _viewPort.MaxDepth = 1.0f;

    _width = width;
    _height = height;
    _viewScissor = {0, 0, _width, _height };
}

void Device::swapChainResizeBuffer(const uint32_t width, const uint32_t height)
{
    if(!_swapChain) return;

    ThrowIfFailed(_swapChain->ResizeBuffers(
                MaxFlightCount, 
                width,
                height,
                _renderTargetFormat,
                DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
}

void Device::createRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
    rtvDesc.NumDescriptors = MaxFlightCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDesc.NodeMask = 0;
    ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(
                &rtvDesc, 
                IID_PPV_ARGS(_rtvHeap.GetAddressOf())
    ));

    D3D12_DESCRIPTOR_HEAP_DESC  dsvDesc;
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDesc.NodeMask = 0;

    ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(
                &dsvDesc, 
                IID_PPV_ARGS(_dsvHeap.GetAddressOf())
    ));
}

void Device::createRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE cbvTableFrist;
    cbvTableFrist.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE cbvTableSecond;
    cbvTableSecond.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    //root parameter can be a table, root descrpitor or root constans.
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    //create root cbvs.
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTableFrist);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTableSecond);

    //a root signature is an array of root parameter.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 
                                            0, nullptr,
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    //craete a root signature withe a single slot which points to descriptor range consisting of a single constant buffer.
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    auto hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                        serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
    
    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char *)errorBlob->GetBufferPointer());
    }

    ThrowIfFailed(hr);

    ThrowIfFailed(_d3dDevice->CreateRootSignature(
        0,
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(_rootSignature.GetAddressOf())
    ));
}

void Device::createPipelineStateObjects(ComPtr<ID3D12PipelineState> &opaque, 
                                    ComPtr<ID3D12PipelineState> &wire, 
                                    std::unordered_map<std::string, ComPtr<ID3DBlob>> &shaders, 
                                    std::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;

    //pso for opaque objects.
    ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaqueDesc.InputLayout = { inputLayout.data(), (uint32_t) inputLayout.size() };
    opaqueDesc.VS = 
    {
        reinterpret_cast<BYTE *>(shaders["standardVS"]->GetBufferPointer()),
        shaders["standardVS"]->GetBufferSize()
    };

    opaqueDesc.PS = 
    {
        reinterpret_cast<BYTE *>(shaders["opaquePS"]->GetBufferPointer()),
        shaders["opaquePS"]->GetBufferSize()
    };

    opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaqueDesc.SampleMask = UINT_MAX;
    opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaqueDesc.NumRenderTargets = 1;
    opaqueDesc.RTVFormats[0] = _renderTargetFormat;
    opaqueDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
    opaqueDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
    opaqueDesc.DSVFormat = _depthStencilFormat;

    ThrowIfFailed(_d3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(opaque.GetAddressOf())));


    //pso for opaque wireframe objects.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframeDesc = opaqueDesc;
    wireframeDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(_d3dDevice->CreateGraphicsPipelineState(&wireframeDesc, IID_PPV_ARGS(wire.GetAddressOf())));
}

void Device::createShadersAndLayout(std::unordered_map<std::string, ComPtr<ID3DBlob>> &shaders, std::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout)
{
    shaders["standardVS"] = Utils::compileShader(L"bin\\shaders\\Shapes\\shapes.hlsl", nullptr, "VS", "vs_5_1");
    shaders["opaquePS"] = Utils::compileShader(L"bin\\shaders\\Shapes\\shapes.hlsl", nullptr, "PS", "ps_5_1");


    inputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

ID3D12Resource *Device::currRenderTarget() const
{
    return _renderTargets[_currFlightIndex].Get();
}


D3D12_CPU_DESCRIPTOR_HANDLE Device::currRenderTargetView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        _rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        _currFlightIndex,
        _rtvDescSize
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE Device::depthStencilView() const
{
    return _dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void Device::logAdapters()
{
        UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while(_dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        logAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void Device::logAdapterOutputs(IDXGIAdapter* adapter)
{
        UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        logOutputDisplayModes(output, _renderTargetFormat);

        ReleaseCom(output);

        ++i;
    }
}

void Device::logOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}

void Device::executeCommand()
{
    ID3D12CommandList *cmdLists[] = { _graphicsCmdList.Get() };
    _graphicsCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void Device::resetGraphicsCmdList()
{
    ThrowIfFailed(_graphicsCmdList->Reset(_graphicsCmdListAlloc.Get(), nullptr));
}

void Device::closeGraphicsCmdList()
{
    ThrowIfFailed(_graphicsCmdList->Close());
}

Device::operator Microsoft::WRL::ComPtr<ID3D12Device>() const 
{
    return _d3dDevice;
}

void Device::release()
{

#if defined(DEBUG) || defined(_DEBUG)
    ID3D12DebugDevice1 *d3dDebug;
    auto hr = _d3dDevice->QueryInterface(&d3dDebug);
    if(SUCCEEDED(hr))
    {
        hr = d3dDebug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
    }
    
    if (d3dDebug != nullptr)			d3dDebug->Release();
#endif
	if (_d3dDevice != nullptr)			_d3dDevice->Release();
}