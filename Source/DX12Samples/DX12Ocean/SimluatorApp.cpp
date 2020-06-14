#include "SimluatorApp.hpp"
#include "MeshGeometry.hpp"
#include "../ThirdPart/Nix/Utils/Utils.hpp"




struct Vertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};


SimluatorApp::SimluatorApp()
:_vsShader(nullptr)
,_fragShader(nullptr)
,_boxGeometry(nullptr)
,_pipelineStateObject(nullptr)
,_swapChain(nullptr)
{

}


bool SimluatorApp::initialize(void *hwnd, Nix::IArchive *archive)
{
    if(!hwnd) return false;
    
    _hwnd = hwnd;
    _archive = archive;

    if(!this->_device.initialize((HWND)hwnd)) return false;

    ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

    createDescriptorHeap(device);
    createConstantBuffers(device);
    createRootSignature(device);
    createShaderAndLayout(device);
    // createFrameResouce(device);
    
    createCubeGeometry(device);
    // createLandGeometry();
    // createWavesGeometryBuffers();
    createPipelineStateObjects(device);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart();
    auto rtvDescriptorSize = _device.getRtvDescSize();
    _swapChain = _device.getSwapChain();

    // for (int i = 0; i < MaxFlightCount; i++) {
    //     // first we get the n'th buffer in the swap chain and store it in the n'th
    //     // position of our ID3D12Resource array
    //     HRESULT rst = _swapChain->GetBuffer( i, IID_PPV_ARGS(&_renderTargets[i]));
    //     _renderTargets[i]->SetName(L"swapchain render target buffer");
    //     if (FAILED(rst)) return false;
    //     // the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
    //     device->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, { rtvHandle.ptr + rtvDescriptorSize * i });
    // }


    return true;
}

void SimluatorApp::createConstantBuffers(ComPtr<ID3D12Device> device)
{
    _cbvObj = std::make_unique<UploadBuffer<ConstantObject>>(device.Get(), 1, true);
    uint32_t objByteSize = Nix::Utils::calcConstantBufferSize(sizeof(ConstantObject));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = _cbvObj->Resource()->GetGPUVirtualAddress();
    auto boxConstantBufferIndex = 0;
    cbAddress += boxConstantBufferIndex * objByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
    {
        desc.BufferLocation = cbAddress;
        desc.SizeInBytes = Nix::Utils::calcConstantBufferSize(sizeof(ConstantObject));
    }

    device->CreateConstantBufferView(&desc, _cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void SimluatorApp::createDescriptorHeap(ComPtr<ID3D12Device> device)
{
    D3D12_DESCRIPTOR_HEAP_DESC  cbvHeapDesc;
    {
        cbvHeapDesc.NumDescriptors = 1;
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cbvHeapDesc.NodeMask = 0;
    }
    ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(_cbvHeap.GetAddressOf())));
}

void SimluatorApp::createRootSignature(ComPtr<ID3D12Device> device)
{
    CD3DX12_ROOT_PARAMETER rootParameter[1];

    //create root cbv.
    // rootParameter[0].InitAsConstantBufferView(0);
    // rootParameter[1].InitAsConstantBufferView(1);
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    rootParameter[0].InitAsDescriptorTable(1, &cbvTable);
    //a root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC desc(1, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    //create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    auto result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
    if(errorBlob != nullptr) ::OutputDebugStringA((char *)errorBlob->GetBufferPointer());

    ThrowIfFailed(result);

    ThrowIfFailed(device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), 
                                        serializedRootSignature->GetBufferSize(), 
                                        IID_PPV_ARGS(&_pipelineRootSignature)));
}

void SimluatorApp::createShaderAndLayout(ComPtr<ID3D12Device> device)
{
    HRESULT result = S_OK;

    _vsShader = Nix::Utils::compileShader(L"bin\\shaders\\color.hlsl", nullptr, "vsMain", "vs_5_0");
    _fragShader = Nix::Utils::compileShader(L"bin\\shaders\\color.hlsl", nullptr, "psMain", "ps_5_0");
 

    _inputLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void SimluatorApp::createPipelineStateObjects(ComPtr<ID3D12Device> device)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
    ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    desc.InputLayout = { _inputLayout.data(), (uint32_t)_inputLayout.size() };
    desc.pRootSignature = _pipelineRootSignature.Get();
    desc.VS = 
    {
        reinterpret_cast<BYTE *> (_vsShader->GetBufferPointer()),
        _vsShader->GetBufferSize(),
    };

    desc.PS = 
    {
        reinterpret_cast<BYTE *>(_fragShader->GetBufferPointer()),
        _fragShader->GetBufferSize()
    };

    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.SampleMask = UINT_MAX;
    // desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = _device.get4xMassState() ? 4 : 1;
    desc.SampleDesc.Quality = _device.get4xMassState() ? (_device.get4xMassQuality() - 1) : 0;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&_pipelineStateObject)));
}

void SimluatorApp::createFrameResouce(ComPtr<ID3D12Device> device)
{

}

void SimluatorApp::createCubeGeometry(ComPtr<ID3D12Device> device)
{
     std::array<Vertex, 8> vertices = 
    {
        Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
        Vertex({XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
        Vertex({XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
        Vertex({XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
        Vertex({XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(Colors::Blue)}),
        Vertex({XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(Colors::Yellow)}),
        Vertex({XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(Colors::Cyan)}),
        Vertex({XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(Colors::Magenta)}),
    };

    std::array<uint16_t, 36> indices = 
    {
        //font face
        0, 1, 2,
        0, 2, 3,

        //back face
        4, 5, 6,
        4, 7, 6, 

        //left face
        3, 2, 6, 
        3, 6, 7, 

        //top face
        1, 5, 6, 
        1, 6, 2,
        
        //bottom face
        4, 0, 3, 
        4, 3, 7,
    };

    const uint32_t vbSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
    const uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

    _boxGeometry = std::make_unique<MeshGeometry>();

    // auto lh = D3DCreateBlob(vbSize, &_boxGeometry->getVertexBufferCPU());
    ThrowIfFailed(D3DCreateBlob(vbSize, &_boxGeometry->getVertexBufferCPU()));
    CopyMemory(_boxGeometry->getVertexBufferCPU()->GetBufferPointer(), vertices.data(), vbSize);

    ThrowIfFailed(D3DCreateBlob(ibSize, &_boxGeometry->getIndexBufferCPU()));
    CopyMemory(_boxGeometry->getIndexBufferCPU()->GetBufferPointer(), indices.data(), ibSize);
    
    //TODO:  uploadcommandList, uploadComandAllocation and uploadQueue 
    auto deviceCmdList = _device.getUploadCommandList();
    auto deviceCmdAllocation = _device.getUploadCommandAllocation();
    auto deviceCmdQueue = _device.getUploadCommandQueue();
    auto deviceFence = _device.getUploadFence();
    auto deviceFenceValue = _device.getUploadFenceValue();

    auto vertexbuffer = Nix::Utils::createUploadBuffer(device, deviceCmdList, 
                                            deviceCmdAllocation, deviceCmdQueue, 
                                            deviceFence, deviceFenceValue, 
                                            vertices.data(), vbSize, _boxGeometry->getVertexBufferUploader());

    // auto vertexBuffer = Nix::Utils::createBuffer(device, deviceCmdList, vertices.data(), vbSize);

                            
    _boxGeometry->setVertexBufferGPU(vertexbuffer);

    auto indexBuffer = Nix::Utils::createUploadBuffer(device, deviceCmdList, 
                                            deviceCmdAllocation, deviceCmdQueue, 
                                            deviceFence, deviceFenceValue, 
                                            indices.data(), ibSize, _boxGeometry->getIndexBufferUploader());

    _boxGeometry->setIndexBufferGPU(indexBuffer);

    _boxGeometry->setVertexStride(sizeof(Vertex));
    _boxGeometry->setVertexBufferSize(vbSize);
    _boxGeometry->setIndexBufferSize(ibSize);
    _boxGeometry->setIndexFormat(DXGI_FORMAT_R16_UINT);

    SubMeshGeometry subMesh;
    subMesh._indexCount = (uint32_t)indices.size();
    subMesh._startIndexLocation = 0;
    subMesh._baseVertexLocation = 0;

    // auto drawArgs = _boxGeometry->getDrawArgs();
    // drawArgs["box"] = subMesh;
    _boxGeometry->setDrawArgsElement("box", subMesh);
}

void SimluatorApp::createWavesGeometryBuffers()
{
   
    
}

void SimluatorApp::createLandGeometry()
{

}

void SimluatorApp::updateWaves(const float dt)
{

}


void SimluatorApp::resize(uint32_t width, uint32_t height)
{
    if(width <= 0 && height <= 0) return;

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



    if(!_swapChain) {
        // _swapChain = _device.createSwapChian(width, height);
        // _device.createRtvAndDsvDescriptorHeaps();
    }
    else {
        
        ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart();

        _device.flushGraphicsCommandQueue();

        for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
            _renderTargets[flightIndex].Reset();
        }

        HRESULT rst = _swapChain->ResizeBuffers( MaxFlightCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0 );
        if( FAILED(rst)) OutputDebugString(L"Error!");

        auto rtvDescriptorSize = _device.getRtvDescSize();
        // get render target buffer & re-create render target view
        for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
            _swapChain->GetBuffer(flightIndex, IID_PPV_ARGS(&_renderTargets[flightIndex]));
            _renderTargets[flightIndex]->SetName(L"swapchain render target buffer");
            device->CreateRenderTargetView(_renderTargets[flightIndex].Get(), nullptr, { rtvHandle.ptr + rtvDescriptorSize * flightIndex });
        }

        D3D12_RESOURCE_DESC depthStencilDesc;
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;

        depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

        depthStencilDesc.SampleDesc.Count = _device.get4xMassState() ? 4 : 1;
        depthStencilDesc.SampleDesc.Quality = _device.get4xMassState() ? (_device.get4xMassQuality() - 1) : 0;

        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        
        D3D12_CLEAR_VALUE optClear;
        optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        optClear.DepthStencil.Depth = 1.0f;
        optClear.DepthStencil.Stencil = 0;
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &optClear,
            IID_PPV_ARGS(_depthStencilBuffer.GetAddressOf())
        ));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(_depthStencilBuffer.Get(), &dsvDesc, _device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart());

        ComPtr<ID3D12GraphicsCommandList> commandList = _device.draw(_flightIndex, _pipelineStateObject);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

        ThrowIfFailed(commandList->Close());

        ID3D12CommandList *cmdList[] = { commandList.Get() };

        auto cmdQueue = _device.getGraphicsCmdQueue();
        cmdQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

        _device.flushGraphicsCommandQueue();
    }

    XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&_proj, p);

}

void SimluatorApp::release()
{

}

void SimluatorApp::tick(float dt)
{
    auto x = _radius * sinf(_phi) * cosf(_theta);
    auto y = _radius * cosf(_phi);
    auto z = _radius * sinf(_phi) * sinf(_theta);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&_view, view);

    XMMATRIX world = XMLoadFloat4x4(&_world);
    XMMATRIX proj = XMLoadFloat4x4(&_proj);
    XMMATRIX worldViewProj = world * view * proj;

    // update the constant buffer view the lastst worldviewproj.
    ConstantObject obj;
    XMStoreFloat4x4(&obj.worldViewProj, XMMatrixTranspose(worldViewProj));
    _cbvObj->copyData(0, obj);

}

void SimluatorApp::draw()
{
    if(!_swapChain) return;
    HRESULT result = S_OK;

    _flightIndex = _swapChain->GetCurrentBackBufferIndex();
    _device.waitForFlight(_flightIndex);

    ComPtr<ID3D12GraphicsCommandList> commandList = _device.draw(_flightIndex, _pipelineStateObject);
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

    auto rtvDescriptorSize = _device.getRtvDescSize();
    auto dsvDescriptorSize = _device.getDsvDescSize();
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart(), _flightIndex, rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart(), _flightIndex, dsvDescriptorSize);
    commandList->ClearRenderTargetView(rtvHandle, Colors::LightSteelBlue, 0, nullptr);
    commandList->ClearDepthStencilView(_device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvHandle, true, &_device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart());
    // commandList->OMSetRenderTargets(1, &rtvHandle, false);
    //transfrom the render target's layout

    commandList->SetGraphicsRootSignature(_pipelineRootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = {
        _cbvHeap.Get(),
        // _samplerDescriptorHeap.Get()
    };

    commandList->SetDescriptorHeaps( _countof(heaps), heaps ); 

    CD3DX12_GPU_DESCRIPTOR_HANDLE pipelineGPUDescriptorHandlerStart( _cbvHeap->GetGPUDescriptorHandleForHeapStart());


    // 第[0]个是 const buffer view descriptor
    // commandList->SetGraphicsRootDescriptorTable(0, pipelineGPUDescriptorHandlerStart.Offset(_flightIndex, _device.getCbvDescSize()));
    commandList->SetGraphicsRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart()); 
    // commandList->SetGraphicsRootDescriptorTable(0, pipelineGPUDescriptorHandlerStart.Offset(_device.getCbvDescSize()));
    // 第[1] 个是 texture view descriptor
    // commandList->SetGraphicsRootDescriptorTable(1, _simpleTextureViewGPUDescriptorHandle );
    // 第[2] 个是 sampler descriptor
    // commandList->SetGraphicsRootDescriptorTable(2, _samplerGPUDescriptorHandle);

    commandList->SetPipelineState(_pipelineStateObject.Get());
    commandList->RSSetViewports(1, &_viewPort);
    commandList->RSSetScissorRects(1, &_viewScissor);
    // commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
    commandList->IASetVertexBuffers(0, 1, &_boxGeometry->vertexBufferView());
    // commandList->DrawInstanced(3, 1, 0, 0); //fianlly draw 3 vertices (draw the triangle)
    commandList->IASetIndexBuffer(&_boxGeometry->indexBufferView());
    commandList->DrawIndexedInstanced(_boxGeometry->getDrawArgs()["box"]._indexCount, 1, 0, 0, 0);

    std::swap(barrier.Transition.StateAfter, barrier.Transition.StateBefore);
    commandList->ResourceBarrier(1, &barrier);

    result = commandList->Close();
    _device.executeCommand(commandList);

    result = _swapChain->Present(1, 0);
}

void SimluatorApp::onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y)
{

}

char * SimluatorApp::title()
{
    return "Ocean Simluator";
}

uint32_t SimluatorApp::rendererType()
{
    return 0;
}

SimluatorApp theapp;

NixApplication* getApplication() {
    return &theapp;
}