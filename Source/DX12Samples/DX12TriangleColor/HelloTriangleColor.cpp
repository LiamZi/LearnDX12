/*
 * @Author: your name
 * @Date: 2020-04-14 11:20:26
 * @LastEditTime: 2020-04-15 12:19:53
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \LearnDX12\Source\DX12Samples\DX12TriangleColor\HelloTriangleColor.cpp
 */


#include "HelloTriangleColor.h"
#include <d3dcompiler.h>
#include "../ThirdPart/d3dx12.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <Nix/IO/Archive.h>



const char vertexShader[] = R"(
    float4 main(float3 pos : POSITION) : SV_POSITON
    {
        return float4(pos, 1.0f);
    }
)";

const char fragShader[] = R"(
    float4 main() : SV_TARGET
    {
        return float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
)";

HelloTriangle::HelloTriangle(/* args */)
:m_dxgiFactory(NULL)
,m_dxgiAdapter(NULL)
,m_dxgiDevice(NULL)
,m_commandQueue(NULL)
,m_swapchain(NULL)
,m_flightIndex(0)
,m_rtvDescriptorHeap(NULL)
,m_rtvDescriptorSize(0)
,m_commandList(NULL)
,m_fenceEvent(NULL)
,m_running(false)
,_archive(NULL)
,_pipelineStateObject(NULL)
,_pipelineRootSignature(NULL)
,_vertexBuffer(NULL)
{

}


HelloTriangle::~HelloTriangle()
{
}


bool HelloTriangle::initialize(void *_wnd, Nix::IArchive* archive)
{
    if(!_wnd || !archive) return false;

    _archive = archive;

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif

    // 1. create factory
	HRESULT factoryHandle = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
	if (FAILED(factoryHandle)) {
		return false;
	}

    // 2.  enum all GPU devices and select a GPU that support DX12
	int adapterIndex = 0;
	bool adapterFound = false;
    while (m_dxgiFactory->EnumAdapters1(adapterIndex, &m_dxgiAdapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC1 desc;
        m_dxgiAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { // if it's software
            ++adapterIndex;
            continue;
        }
        // test it can create a device that support dx12 or not
        HRESULT rst = D3D12CreateDevice(m_dxgiAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr );
        if (SUCCEEDED(rst)) {
            adapterFound = true;
            break;
        }
        ++adapterIndex;
    }

    if (!adapterFound) return false;

    // 3. create the device
    HRESULT rst = D3D12CreateDevice(
        m_dxgiAdapter,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_dxgiDevice)
    );
    
    if (FAILED(rst)) return false;

    // 4. create command queue
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc; {
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cmdQueueDesc.NodeMask = 0;
    }
    rst = m_dxgiDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(rst)) return false;

    // 5. swapchain / when windows is resize
    this->_hwnd = _wnd;// handle of the window

    // 6. create descriptor heap
    // describe an rtv descriptor heap and create
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; {
        rtvHeapDesc.NumDescriptors = MaxFlightCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    }
       
    rst = m_dxgiDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));
    if (FAILED(rst)) return false;

    m_rtvDescriptorSize = m_dxgiDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 8. create command allocators
    for (int i = 0; i < MaxFlightCount; i++) {
        rst = m_dxgiDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));
        if (FAILED(rst)) return false;
    }

    // create the fences
    for (int i = 0; i < MaxFlightCount; i++) {
        rst = m_dxgiDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i]));
        if (FAILED(rst)) return false;
        m_fenceValue[i] = 0; // set the initial fence value to 0
    }

    // create a handle to a fence event
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) return false;


    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    ID3DBlob *signature = nullptr;
    ID3DBlob *error = nullptr;

    rst = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if(FAILED(rst)) return false;

    rst = m_dxgiDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_pipelineRootSignature));
    if(FAILED(rst)) return false;

    ID3DBlob *vertexShader = nullptr;
    ID3DBlob *fragmentShader = nullptr;
    ID3DBlob *shaderErrorBuffer = nullptr;

    auto file = _archive->open("shaders/triangle/vertexShaderColor.hlsl", 1);
    if(!file) return false;

    rst = D3DCompile(file->constData(), file->size(), 
                    "vertexShaderColor.hlsl", nullptr, nullptr, 
                    "main", "vs_5_0",
                    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                    0, &vertexShader, &shaderErrorBuffer);
    
    if(FAILED(rst)) return false;
    file->release();

    file = _archive->open("shaders/triangle/fragmentShaderColor.hlsl", 1);
    if(!file) return false;

    rst = D3DCompile(file->constData(), file->size(),
                    "fragmentShaderColor.hlsl", nullptr, nullptr,
                    "main", "ps_5_0",
                    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                    0, &fragmentShader, &shaderErrorBuffer);
    
    if(FAILED(rst)) { 
        OutputDebugStringA((char *)shaderErrorBuffer->GetBufferPointer()); 
        return false; 
    }

    D3D12_SHADER_BYTECODE vertexShaderByteCode = {};
    vertexShaderByteCode.BytecodeLength = vertexShader->GetBufferSize();
    vertexShaderByteCode.pShaderBytecode = vertexShader->GetBufferPointer();

    D3D12_SHADER_BYTECODE fragmentShaderByteCode = {};
    fragmentShaderByteCode.BytecodeLength = fragmentShader->GetBufferSize();
    fragmentShaderByteCode.pShaderBytecode = fragmentShader->GetBufferPointer();


    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    {
        inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout;
    }


    DXGI_SAMPLE_DESC sampler = {};
    {
        sampler.Count = 1;
    }
    

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
    {
        auto &desc = pipelineStateDesc;
        desc.InputLayout = inputLayoutDesc;
        desc.pRootSignature = _pipelineRootSignature;
        desc.VS = vertexShaderByteCode;
        desc.PS = fragmentShaderByteCode;
        desc.SampleDesc = sampler;
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

    rst = m_dxgiDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&_pipelineStateObject));
    if(FAILED(rst)) return false;

    // 9. create command list
	// create the command list with the first allocator
    rst = m_dxgiDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0], _pipelineStateObject, IID_PPV_ARGS(&m_commandList));
    if (FAILED(rst)) return false;

    m_commandList->Close();


    //trianle
    // struct Vertex {
    //     DirectX::XMFLOAT3 pos;
    // };

    struct Vertex {
        Vertex(float x, float y, float z, float r, float g, float b, float a) : pos(x, y, z), color(r, g, b, z) {}
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT4 color;
    };

    Vertex vlist[] = {
        { 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
    };

    int vBufferSize = sizeof(vlist);
    
    m_dxgiDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                        D3D12_HEAP_FLAG_NONE,
                                        &CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                        nullptr,
                                        IID_PPV_ARGS(&_vertexBuffer)
    );

    _vertexBuffer->SetName(L"Vertex Buffer Resouce Heap");

    ID3D12Resource *vBufferUploadHeap;
    m_dxgiDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                        D3D12_HEAP_FLAG_NONE,
                                        &CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                        nullptr,
                                        IID_PPV_ARGS(&vBufferUploadHeap)
    );
    vBufferUploadHeap->SetName(L"Vertex Buff Upload Resouce Heap");
    m_commandAllocators[m_flightIndex]->Reset();
    m_commandList->Reset(m_commandAllocators[m_flightIndex], _pipelineStateObject);


    D3D12_SUBRESOURCE_DATA vertexData = {};
    {
        vertexData.pData = reinterpret_cast<BYTE *>(vlist);
        vertexData.RowPitch = vBufferSize;
        vertexData.SlicePitch = vBufferSize;
    }

    UpdateSubresources(m_commandList, _vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_vertexBuffer, 
                                D3D12_RESOURCE_STATE_COPY_DEST, 
                                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
    
    m_commandList->Close();
    ID3D12CommandList *commandLists[] ={ m_commandList };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ++m_fenceValue[m_flightIndex];
    
    rst = m_commandQueue->Signal(m_fence[m_flightIndex], m_fenceValue[m_flightIndex]);
    if(FAILED(rst)) return false;

    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.StrideInBytes = sizeof(Vertex);
    _vertexBufferView.SizeInBytes = vBufferSize;

    return true;
}


 void HelloTriangle::resize(uint32_t _width, uint32_t _height)
 {
     _viewPort.TopLeftX = 0;
     _viewPort.TopLeftY = 0;
     _viewPort.Width = (float)(_width);
     _viewPort.Height = (float)(_height);
     _viewPort.MinDepth = 0.0f;
     _viewPort.MaxDepth = 1.0f;

     _scissorRect.left = 0;
     _scissorRect.top = 0;
     _scissorRect.right = _width;
     _scissorRect.bottom = _height;

     if(!this->m_swapchain) {
        DXGI_MODE_DESC displayModeDesc = {}; 
        {
            displayModeDesc.Width = _width;
            displayModeDesc.Height = _height;
            displayModeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 不需要提前获取支持什么格式？ 对于vk来说是必走流程
        }

        DXGI_SAMPLE_DESC samplerDesc = {};
        {
            samplerDesc.Count = 1;
        }

        DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
        {
            swapchainDesc.BufferCount = MaxFlightCount;
            swapchainDesc.BufferDesc = displayModeDesc;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapchainDesc.OutputWindow = (HWND)_hwnd;
            swapchainDesc.SampleDesc = samplerDesc;
            swapchainDesc.Windowed = true;
        }

        IDXGISwapChain* swapchain = nullptr;
        HRESULT rst = this->m_dxgiFactory->CreateSwapChain(this->m_commandQueue, &swapchainDesc, &swapchain);
        if(FAILED(rst)) return;
        
        this->m_swapchain = static_cast<IDXGISwapChain3 *>(swapchain);
        this->m_flightIndex = m_swapchain->GetCurrentBackBufferIndex();

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (unsigned int i = 0; i < MaxFlightCount; i++)
        {
            auto rst = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
            if(FAILED(rst)) return;
            m_dxgiDevice->CreateRenderTargetView(m_renderTargets[i], nullptr, {rtvHandle.ptr + m_rtvDescriptorSize * i});
        }
    }
    else {

        for (size_t flightIndex = 0; flightIndex < MaxFlightCount; flightIndex++)
        {
           if(m_fence[flightIndex]->GetCompletedValue() < m_fenceValue[flightIndex]) {
               HRESULT rst;
               rst = m_fence[flightIndex]->SetEventOnCompletion(m_fenceValue[flightIndex], m_fenceEvent);
               if(FAILED(rst)) m_running = false;

               WaitForSingleObject(m_fenceEvent, INFINITE);
           }
        }

        m_swapchain->ResizeBuffers(MaxFlightCount, _width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
        
        for (size_t i = 0; i < MaxFlightCount; i++)
        {
            auto lh = m_renderTargets[i];
            if(!lh) continue;
            lh->Release();
            lh = nullptr;
        }

        this->m_flightIndex = m_swapchain->GetCurrentBackBufferIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (unsigned int i = 0; i < MaxFlightCount; i++)
        {
            auto rst = m_swapchain->GetBuffer(i , IID_PPV_ARGS(&m_renderTargets[i]));
            if(FAILED(rst)) return;
            m_dxgiDevice->CreateRenderTargetView(m_renderTargets[i], nullptr, { rtvHandle.ptr + m_rtvDescriptorSize * i });
        }
        
    }

    printf("resized !");
 }


void HelloTriangle::release()
{
    for (size_t flightIndex = 0; flightIndex < MaxFlightCount; flightIndex++)
    {
        if(m_fence[flightIndex]->GetCompletedValue() < m_fenceValue[flightIndex]) {
            HRESULT rst;
            rst = m_fence[flightIndex]->SetEventOnCompletion(m_fenceValue[flightIndex], m_fenceEvent);
            if(FAILED(rst)) m_running = false;

            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    m_swapchain->Release();
    m_swapchain = nullptr;
    m_rtvDescriptorHeap->Release();
    m_rtvDescriptorHeap = nullptr;
    m_commandList->Release();
    m_commandList = nullptr;

    for (size_t flightIndex = 0; flightIndex < MaxFlightCount; flightIndex++)
    {
        m_commandAllocators[flightIndex]->Release();
        m_commandAllocators[flightIndex] = nullptr;

        m_fence[flightIndex]->Release();
        m_fence[flightIndex] = nullptr;
    }

    m_dxgiDevice->Release();
    m_dxgiDevice = nullptr;

    printf("destroyed.");
}

void HelloTriangle::draw()
{
    if(!m_swapchain) return;

    {
        HRESULT rst;
        m_flightIndex = m_swapchain->GetCurrentBackBufferIndex();
        if(m_fence[m_flightIndex]->GetCompletedValue()< m_fenceValue[m_flightIndex]) {
            rst = m_fence[m_flightIndex]->SetEventOnCompletion(m_fenceValue[m_flightIndex], m_fenceEvent);
            if(FAILED(rst)) m_running = false;

            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    {
        ID3D12CommandAllocator *commandAllocator = m_commandAllocators[m_flightIndex];
        auto rst = commandAllocator->Reset();
        if(FAILED(rst)) m_running = false;

        m_commandList->Reset(commandAllocator, _pipelineStateObject);
        if(FAILED(rst)) m_running = false;


        D3D12_RESOURCE_BARRIER barrier;
        {
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.pResource = m_renderTargets[m_flightIndex];
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        }

        m_commandList->ResourceBarrier(1, &barrier);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                                            m_flightIndex, m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        m_commandList->SetGraphicsRootSignature(_pipelineRootSignature);
        m_commandList->SetPipelineState(_pipelineStateObject);
        m_commandList->RSSetViewports(1, &_viewPort);
        m_commandList->RSSetScissorRects(1, &_scissorRect);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);

        std::swap(barrier.Transition.StateAfter, barrier.Transition.StateBefore);
        m_commandList->ResourceBarrier(1, &barrier);

        rst = m_commandList->Close();
        if(FAILED(rst)) m_running = false;
    }

    {
        ID3D12CommandList *commandLists[] = { m_commandList };
        m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
        ++m_fenceValue[m_flightIndex];

        auto rst = m_commandQueue->Signal(m_fence[m_flightIndex], m_fenceValue[m_flightIndex]);
        if(FAILED(rst)) m_running = false;

        rst = m_swapchain->Present(1, 0);
        if(FAILED(rst)) m_running = false;
    }
}

void HelloTriangle::tick(double dt)
{
    printf("test tick : %lf \n", dt);
}

char *HelloTriangle::title()
{
    return "Hello DX12 Triangle Color.";
}

uint32_t HelloTriangle::rendererType()
{
    return 0;
}

HelloTriangle theapp;

NixApplication* getApplication() {
    return &theapp;
}

