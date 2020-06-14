#include "Device.hpp"

Device::Device()
{
}

Device::~Device()
{
}

bool Device::initialize(HWND hwnd, uint32_t width, uint32_t height)
{
#if defined(DEBUG) || defined(_DEBUG)
    {
        //Enable the d3d12 debug layer;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&_debugController)));
        _debugController->EnableDebugLayer();
    }
#endif

    _hwnd = hwnd;
    _width = width;
    _height = height;

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&_factory)));

    auto result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));

    if (FAILED(result))
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));
    }

    _rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    _dsvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    _cbvSrvUavDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


    createCommnadObjectsAndFence();
    _swapChain = createSwapChian(1280, 720);
    
    createUploadCommandObjectsAndFence();
    createRtvAndDsvDescriptorHeaps();

    //check 4x MSAA quality support for back buffer format.
    check4xMsaaSupport();

#ifdef _DEBUG
    logAdapters();
#endif

    return true;
}

void Device::check4xMsaaSupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {};
    qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    qualityLevels.SampleCount = 4;
    qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    qualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(_device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &qualityLevels, sizeof(qualityLevels)));
    _4xMsaaQuality = qualityLevels.NumQualityLevels;
    assert(_4xMsaaQuality > 0 && "Unexpected MSAA quality level. ");
}

void Device::createCommnadObjectsAndFence()
{
    //initizaliza command object and graphics fence

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_graphicsCommandQueue)));

    for (uint32_t i = 0; i < MaxFlightCount; ++i)
    {
        ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_graphicsCommandAllocator[i].GetAddressOf())));

        ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _graphicsCommandAllocator[i].Get(), nullptr, IID_PPV_ARGS(_graphicsCommandLists[i].GetAddressOf())));
        _graphicsCommandLists[i]->Close();

        ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_graphicsFences[i])));
    }

    //TODO: 感觉初始化这个EVENT 并没有什么效果。 以后再看。
    _graphicsFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
}

void Device::createUploadCommandObjectsAndFence()
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(_device->CreateCommandQueue(&desc, IID_PPV_ARGS(_uploadQueue.GetAddressOf())));
    ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_uploadCommandAllocator.GetAddressOf())));
    ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _uploadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(_uploadCommandList.GetAddressOf())));
    _uploadCommandList->Close();
    ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_uploadFence.GetAddressOf())));
    _uploadFenceValue = 0;
    _uploadFenceEvent =  CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
}

ComPtr<IDXGISwapChain3> Device::createSwapChian(const uint32_t width, const uint32_t height)
{
    //create swap chain
    ComPtr<IDXGISwapChain> swapChian;

    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
    desc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = MaxFlightCount;
    desc.OutputWindow = _hwnd;
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    auto queue = _graphicsCommandQueue.Get();
    ThrowIfFailed(_factory->CreateSwapChain(queue, &desc, swapChian.GetAddressOf()));

    ComPtr<IDXGISwapChain3> finalSwapChain;
    swapChian.As(&finalSwapChain);
    return finalSwapChain;
}

void Device::flushGraphicsCommandQueue()
{
    for (uint32_t i = 0; i < MaxFlightCount; ++i)
    {
        //advance the fence value to mark commands up to this fence point.
        ++_graphicsFenceValues[i];
        auto currentFence = _graphicsFenceValues[i];

        ThrowIfFailed(_graphicsCommandQueue->Signal(_graphicsFences[i].Get(), currentFence));
        if (_graphicsFences[i]->GetCompletedValue() < currentFence)
        {
            auto eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(_graphicsFences[i]->SetEventOnCompletion(currentFence, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }
}

void Device::waitForFlight(uint32_t flight)
{
    if(_graphicsFences[flight]->GetCompletedValue() < _graphicsFenceValues[flight])
    {
        auto eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(_graphicsFences[flight]->SetEventOnCompletion(_graphicsFenceValues[flight], eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

ComPtr<ID3D12GraphicsCommandList> Device::draw(uint32_t flightIndex, ComPtr<ID3D12PipelineState> &pipelineState)
{
    ComPtr<ID3D12PipelineState> state = nullptr;
    if(pipelineState || pipelineState.Get()) state = pipelineState;

    _flightIndex = flightIndex;
    ++_graphicsFenceValues[flightIndex];
    auto &cmdAllocator = _graphicsCommandAllocator[flightIndex];
    auto &cmdList = _graphicsCommandLists[flightIndex];
    cmdAllocator->Reset();
    //TODO: cmdList reset need pipeline state.
    ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), state.Get()));
    return cmdList;
}

void Device::executeCommand(ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    ID3D12CommandList *lists[] = { commandList.Get() };
    _graphicsCommandQueue->ExecuteCommandLists(1, lists);
    ThrowIfFailed(_graphicsCommandQueue->Signal(_graphicsFences[_flightIndex].Get(), _graphicsFenceValues[_flightIndex]));
    // flushGraphicsCommandQueue();
}

void Device::createRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = MaxFlightCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(_rtvHeap.GetAddressOf())));

    // D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    // dsvHeapDesc.NumDescriptors = 1;
    // dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    // dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    // dsvHeapDesc.NodeMask = 0;
    // ThrowIfFailed(_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(_device->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.GetAddressOf())));
}

Device::operator ComPtr<ID3D12Device> () const {
    return _device;
}


void Device::logAdapters()
{
    uint32_t i = 0;
    IDXGIAdapter *adapter = nullptr;
    std::vector<IDXGIAdapter *> adapterList;
    while (_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());
        adapterList.emplace_back(adapter);

        ++i;
    }

    for (auto i = 0; i < adapterList.size(); ++i)
    {
        logAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void Device::logAdapterOutputs(IDXGIAdapter *adapter)
{
    uint32_t i = 0;
    IDXGIOutput *output = nullptr;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());
        logOutputDisplayModes(output, DXGI_FORMAT_R8G8B8A8_UNORM);
        ReleaseCom(output);

        ++i;
    }
}

void Device::logOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format)
{
    uint32_t count = 0;
    uint32_t flags = 0;

    output->GetDisplayModeList(format, flags, &count, nullptr);
    std::vector<DXGI_MODE_DESC> modeList(count);

    for (auto &iter : modeList)
    {
        auto n = iter.RefreshRate.Numerator;
        auto d = iter.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(iter.Width) + L" " +
            L"Height = " + std::to_wstring(iter.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}

void Device::set4xMassState(bool value)
{
    if(_4xMsaaState != value) 
    {
        _4xMsaaState = value;

        // Recreate the swapchain and buffers with new multisample settings.
        createSwapChian(_width, _height);
    }
}