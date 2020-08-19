#include "SimluatorApp.hpp"
#include "MeshGeometry.hpp"
#include "Vertex.hpp"
#include "Nix/Utils/Utils.hpp"
#include "Waves.hpp"


static const XMFLOAT4 OCEANCOLOR = XMFLOAT4(0.004f, 0.016f, 0.047f, 1.0f);


SimluatorApp::SimluatorApp()
:_swapChain(nullptr)
{

}

SimluatorApp::~SimluatorApp()
{
    _device.flushGraphicsCommandQueue();
}


bool SimluatorApp::initialize(void *hwnd, Nix::IArchive *archive)
{
    if(!hwnd) return false;
    
    _hwnd = hwnd;
    _archive = archive;

    if(!this->_device.initialize((HWND)hwnd)) return false;

    ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

    _waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

    // createDescriptorHeap(device);
    // createConstantBuffers(device);
    createRootSignature(device);
    createShaderAndLayout(device);
    // createFrameResouce(device);
    
    createCubeGeometry(device);
    // createLandGeometry();
    createWavesGeometryBuffers(device);
    createRenderItems(device);
    createFrameResource(device);
    createPipelineStateObjects(device);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart();
    auto rtvDescriptorSize = _device.getRtvDescSize();
    _swapChain = _device.getSwapChain();

    _waves = std::make_unique<Waves>(256, 256, 1.0f, 0.03f, 4.0f, 2.0f);

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
    CD3DX12_ROOT_PARAMETER rootParameter[2];
    rootParameter[0].InitAsConstantBufferView(0);
    rootParameter[1].InitAsConstantBufferView(1);
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, 
                                rootParameter, 0, 
                                nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT result = D3D12SerializeRootSignature(&rootSigDesc, 
                                    D3D_ROOT_SIGNATURE_VERSION_1, 
                                    serializedRootSignature.GetAddressOf(), 
                                    errorBlob.GetAddressOf());
    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }

    ThrowIfFailed(result);

    ThrowIfFailed(device->CreateRootSignature(0, 
                serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(_pipelineRootSignature.GetAddressOf())));

}

void SimluatorApp::createShaderAndLayout(ComPtr<ID3D12Device> device)
{
    HRESULT result = S_OK;

    // _vsShader = Nix::Utils::compileShader(L"bin\\shaders\\color.hlsl", nullptr, "vsMain", "vs_5_0");
    // _fragShader = Nix::Utils::compileShader(L"bin\\shaders\\color.hlsl", nullptr, "psMain", "ps_5_0");
    _shaders["standardVS"] = Nix::Utils::compileShader(L"bin\\shaders\\Ocean\\ocean.hlsl", nullptr, "vsMain", "vs_5_0");
    _shaders["opaquePS"] =  Nix::Utils::compileShader(L"bin\\shaders\\Ocean\\ocean.hlsl", nullptr, "psMain", "ps_5_0");

    _inputLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void SimluatorApp::createPipelineStateObjects(ComPtr<ID3D12Device> device)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;

    ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaqueDesc.InputLayout = { _inputLayout.data(), (uint32_t)_inputLayout.size()};
    opaqueDesc.pRootSignature = _pipelineRootSignature.Get();
    opaqueDesc.VS = 
    {
        reinterpret_cast<BYTE *>(_shaders["standardVS"]->GetBufferPointer()),
        _shaders["standardVS"]->GetBufferSize()
    };

    opaqueDesc.PS = 
    {
        reinterpret_cast<BYTE *>(_shaders["opaquePS"]->GetBufferPointer()),
        _shaders["opaquePS"]->GetBufferSize()
    };

    opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
    opaqueDesc.SampleMask = UINT_MAX;
    opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaqueDesc.NumRenderTargets = 1;
    opaqueDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    opaqueDesc.SampleDesc.Count = _device.get4xMassState() ? 4 : 1;
    opaqueDesc.SampleDesc.Quality = _device.get4xMassState() ? (_device.get4xMassQuality() - 1) : 0;
    opaqueDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&_pipelineStateObjects["opaque"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireFramePsoDesc = opaqueDesc;
    opaqueWireFramePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&opaqueWireFramePsoDesc, IID_PPV_ARGS(&_pipelineStateObjects["opaque_wireframe"])));
}

void SimluatorApp::createFrameResource(ComPtr<ID3D12Device> device)
{
    if(_allRenderItems.size() <= 0) return;

    for (size_t i = 0; i < MaxFlightCount; i++)
    {
        _framesResources.push_back(std::make_unique<FrameResource>(device.Get(), 
                                                    1, 
                                                    (uint32_t)_allRenderItems.size(), 
                                                    _waves->vertexCount()));
    }
}


void SimluatorApp::createRenderItems(ComPtr<ID3D12Device> device)
{
    auto wavesRenderItem = std::make_unique<RenderItem>();
    wavesRenderItem->_world = MathHelper::Identity4x4();
    wavesRenderItem->_objContantBufferIndex = 0;
    wavesRenderItem->_geo = _geometries["water"].get();
    wavesRenderItem->_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    auto subMesh =  wavesRenderItem->_geo->getDrawArgsSubMeshByName("grid");
    assert(subMesh);

    wavesRenderItem->_indexCount = subMesh->_indexCount;
    wavesRenderItem->_startIndexLocation = subMesh->_startIndexLocation;
    wavesRenderItem->_baseVertexLocation = subMesh->_baseVertexLocation;

    _wavesRenderItem = wavesRenderItem.get();

    _renderItemLayer[(int)RenderLayer::Opaque].push_back(wavesRenderItem.get());

    _allRenderItems.push_back(std::move(wavesRenderItem));
}

void SimluatorApp::drawRenderItem(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &renderItems)
{
    auto objconstantBufferByteSize = Utils::calcConstantBufferSize(sizeof(ConstantObject));

    auto objectCB = _currFrameResource->_objectConstantBuffer->Resource();

    for (size_t i = 0; i < renderItems.size(); i++)
    {
        /* code */
        auto item = renderItems[i];

        cmdList->IASetVertexBuffers(0, 1, &item->_geo->vertexBufferView());
        cmdList->IASetIndexBuffer(&item->_geo->indexBufferView());
        cmdList->IASetPrimitiveTopology(item->_primitiveType);

        auto objCBAddresss = objectCB->GetGPUVirtualAddress();

        objCBAddresss += item->_objContantBufferIndex * objconstantBufferByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddresss);
        cmdList->DrawIndexedInstanced(item->_indexCount, 1, item->_startIndexLocation, item->_baseVertexLocation, 0);
    }

}

void SimluatorApp::createCubeGeometry(ComPtr<ID3D12Device> device)
{
    // std::array<Vertex, 8> vertices = 
    // {
    //     Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
    //     Vertex({XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
    //     Vertex({XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
    //     Vertex({XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
    //     Vertex({XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(Colors::Blue)}),
    //     Vertex({XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(Colors::Yellow)}),
    //     Vertex({XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(Colors::Cyan)}),
    //     Vertex({XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(Colors::Magenta)}),
    // };

    // std::array<uint16_t, 36> indices = 
    // {
    //     // front face
	// 	0, 1, 2,
	// 	0, 2, 3,

	// 	// back face
	// 	4, 6, 5,
	// 	4, 7, 6,

	// 	// left face
	// 	4, 5, 1,
	// 	4, 1, 0,

	// 	// right face
	// 	3, 2, 6,
	// 	3, 6, 7,

	// 	// top face
	// 	1, 5, 6,
	// 	1, 6, 2,

	// 	// bottom face
	// 	4, 0, 3,
	// 	4, 3, 7
    // };

    // const uint32_t vbSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
    // const uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

    // _boxGeometry = std::make_unique<MeshGeometry>();

    // // auto lh = D3DCreateBlob(vbSize, &_boxGeometry->getVertexBufferCPU());
    // ThrowIfFailed(D3DCreateBlob(vbSize, &_boxGeometry->getVertexBufferCPU()));
    // CopyMemory(_boxGeometry->getVertexBufferCPU()->GetBufferPointer(), vertices.data(), vbSize);

    // ThrowIfFailed(D3DCreateBlob(ibSize, &_boxGeometry->getIndexBufferCPU()));
    // CopyMemory(_boxGeometry->getIndexBufferCPU()->GetBufferPointer(), indices.data(), ibSize);
    
    // //TODO:  uploadcommandList, uploadComandAllocation and uploadQueue 
    // auto deviceCmdList = _device.getUploadCommandList();
    // auto deviceCmdAllocation = _device.getUploadCommandAllocation();
    // auto deviceCmdQueue = _device.getUploadCommandQueue();
    // auto deviceFence = _device.getUploadFence();
    // auto deviceFenceValue = _device.getUploadFenceValue();

    // auto vertexbuffer = Nix::Utils::createUploadBuffer(device, deviceCmdList, 
    //                                         deviceCmdAllocation, deviceCmdQueue, 
    //                                         deviceFence, deviceFenceValue, 
    //                                         vertices.data(), vbSize, _boxGeometry->getVertexBufferUploader());

    // // auto vertexBuffer = Nix::Utils::createBuffer(device, deviceCmdList, vertices.data(), vbSize);

                            
    // _boxGeometry->setVertexBufferGPU(vertexbuffer);

    // auto indexBuffer = Nix::Utils::createUploadBuffer(device, deviceCmdList, 
    //                                         deviceCmdAllocation, deviceCmdQueue, 
    //                                         deviceFence, deviceFenceValue, 
    //                                         indices.data(), ibSize, _boxGeometry->getIndexBufferUploader());

    // _boxGeometry->setIndexBufferGPU(indexBuffer);

    // _boxGeometry->setVertexStride(sizeof(Vertex));
    // _boxGeometry->setVertexBufferSize(vbSize);
    // _boxGeometry->setIndexBufferSize(ibSize);
    // _boxGeometry->setIndexFormat(DXGI_FORMAT_R16_UINT);

    // SubMeshGeometry subMesh;
    // subMesh._indexCount = (uint32_t)indices.size();
    // subMesh._startIndexLocation = 0;
    // subMesh._baseVertexLocation = 0;

    // // auto drawArgs = _boxGeometry->getDrawArgs();
    // // drawArgs["box"] = subMesh;
    // _boxGeometry->setDrawArgsElement("box", subMesh);
}

void SimluatorApp::createWavesGeometryBuffers(ComPtr<ID3D12Device> device)
{
    std::vector<uint16_t> indices(3 * _waves->triangleCount());
    assert(_waves->vertexCount() < 0x0000ffff);
    
    int m = _waves->rowCount();
    int n = _waves->columnCount();

    int k = 0;

    for (int i = 0; i < m - 1; i++)
    {
       for (int j = 0; j < n - 1; j++)
       {
           indices[k] = i * n + j;
           indices[k + 1] = i * n + j + 1;
           indices[k + 2] = (i + 1) * n + j;

           indices[k + 3] = (i + 1) * n + j;
           indices[k + 4] = i * n + j  + 1;
           indices[k + 5] = (i + 1) * n + j + 1;

           k += 6;
       }
    }

    uint32_t vbSize = _waves->vertexCount() * sizeof(Vertex);
    uint32_t ibSize = (uint32_t)indices.size() * sizeof(uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->setName("Ocean Geometry");

    ThrowIfFailed(D3DCreateBlob(ibSize, &geo->getIndexBufferCPU()));
    CopyMemory(geo->getIndexBufferCPU()->GetBufferPointer(), indices.data(), ibSize);

    auto deviceCmdList = _device.getUploadCommandList();
    auto deviceCmdAllocation = _device.getUploadCommandAllocation();
    auto deviceCmdQueue = _device.getUploadCommandQueue();
    auto deviceFence = _device.getUploadFence();
    auto deviceFenceValue = _device.getUploadFenceValue();


    auto buffer = Nix::Utils::createUploadBuffer(device, deviceCmdList, 
                                            deviceCmdAllocation, deviceCmdQueue, 
                                            deviceFence, deviceFenceValue, 
                                            indices.data(), ibSize, geo->getIndexBufferUploader());
    geo->setIndexBufferGPU(buffer);

    geo->setVertexStride(sizeof(Vertex));
    geo->setVertexBufferSize(vbSize);
    geo->setIndexFormat(DXGI_FORMAT_R16_UINT);
    geo->setIndexBufferSize(ibSize);

    SubMeshGeometry subMesh;
    subMesh._indexCount = (uint32_t)indices.size();
    subMesh._startIndexLocation = 0;
    subMesh._baseVertexLocation =0;

    geo->setDrawArgsElement("grid", subMesh);


    _geometries["water"] = std::move(geo);
}

void SimluatorApp::createLandGeometry()
{

}

void SimluatorApp::updateWaves(const float dt)
{
    static float base = 0.0f;
    if((_timer.totalTime() - base) >= 0.25f)
    {
        base += 0.25f;

        auto i = MathHelper::Rand(4, _waves->rowCount() - 5);
        auto j = MathHelper::Rand(4, _waves->columnCount() - 5);

        auto r = MathHelper::RandF(0.2f, 0.5f);

        _waves->disturb(i, j , r);
    }

    _waves->update(dt);

    auto currWavesVB = _currFrameResource->_waveVertexBuffer.get();

    for(auto i = 0; i < _waves->vertexCount(); ++i)
    {
        Vertex v;

        v.pos = _waves->position(i);
        v.color = OCEANCOLOR;

        currWavesVB->copyData(i, v);
    }

    _wavesRenderItem->_geo->setVertexBufferGPU(currWavesVB->Resource());
}


void SimluatorApp::resize(uint32_t width, uint32_t height)
{
    if(width <= 0 && height <= 0) return;

    if(!_swapChain) {
        // _swapChain = _device.createSwapChian(width, height);
        // _device.createRtvAndDsvDescriptorHeaps();
    }
    else {
        
        ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

        _device.flushGraphicsCommandQueue();

        auto cmdList = _device.getGraphicsCmdList(_flightIndex);
        cmdList->Reset(_device.getGraphicsCmdAllocator(_flightIndex).Get(), nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart();

        for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
            _renderTargets[flightIndex].Reset();
        }

        _depthStencilBuffer.Reset();

        HRESULT rst = _swapChain->ResizeBuffers( MaxFlightCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
        if( FAILED(rst)) OutputDebugString(L"Error!");

        // 不一定需要重置当前render target flight index.
        // _flightIndex = 0;

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

        // ComPtr<ID3D12GraphicsCommandList> commandList = _device.draw(_flightIndex, nullptr);
        // ComPtr<ID3D12GraphicsCommandList> commandList = _device.getUploadCommandList();

        ComPtr<ID3D12GraphicsCommandList> commandList = _device.getGraphicsCmdList(_flightIndex);

        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

        ThrowIfFailed(commandList->Close());

        ID3D12CommandList *cmdLists[] = { commandList.Get() };

        //TODO: 可以尝试使用upload command queue
        auto cmdQueue = _device.getGraphicsCmdQueue();
        // auto uploadCmdQueue = _device.getUploadCommandQueue();

        cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        // uploadCmdQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

        _device.flushGraphicsCommandQueue();
    }

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

    // XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    // XMStoreFloat4x4(&_proj, p);

}

void SimluatorApp::release()
{

}

void SimluatorApp::tick(float dt)
{
    // auto x = _radius * sinf(_phi) * cosf(_theta);
    // auto y = _radius * cosf(_phi);
    // auto z = _radius * sinf(_phi) * sinf(_theta);

    // XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    // XMVECTOR target = XMVectorZero();
    // XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    // XMStoreFloat4x4(&_view, view);

    // XMMATRIX world = XMLoadFloat4x4(&_world);
    // XMMATRIX proj = XMLoadFloat4x4(&_proj);
    // XMMATRIX worldViewProj = world * view * proj;

    // // update the constant buffer view the lastst worldviewproj.
    // ConstantObject obj;
    // XMStoreFloat4x4(&obj.world, XMMatrixTranspose(worldViewProj));
    // _cbvObj->copyData(0, obj);


    updateCamera(dt);

    _flightIndex = (_flightIndex + 1) % MaxFlightCount;
    _currFrameResource = _framesResources[_flightIndex].get();
    _device.waitForFlight(_flightIndex);

    updateObjectConstantBuffers(dt);
    updateMainPassConstantBuffer(dt);
    updateWaves(dt);
}

void SimluatorApp::updateObjectConstantBuffers(const float dt)
{
    auto currCB = _currFrameResource->_objectConstantBuffer.get();
    for(auto &iter : _allRenderItems)
    {
        if(iter->_numFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&iter->_world);
            ConstantObject constants;
            XMStoreFloat4x4(&constants.world, XMMatrixTranspose(world));
            currCB->copyData(iter->_objContantBufferIndex, constants);
            iter->_numFramesDirty--;
        }
    }
}

void SimluatorApp::updateMainPassConstantBuffer(const float dt)
{
    XMMATRIX view = XMLoadFloat4x4(&_view);
    XMMATRIX proj = XMLoadFloat4x4(&_proj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
    
    XMStoreFloat4x4(&_mainPassConstantBuffer._view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&_mainPassConstantBuffer._invView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&_mainPassConstantBuffer._proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&_mainPassConstantBuffer._invProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&_mainPassConstantBuffer._viewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&_mainPassConstantBuffer._invViewProj, XMMatrixTranspose(invViewProj));

    _mainPassConstantBuffer._eyePosW = _eyePos;
    _mainPassConstantBuffer._renderTargetSize = XMFLOAT2((float)_width, (float)_height);
    _mainPassConstantBuffer._invRenderTargetSize  = XMFLOAT2(1.0f / _width, 1.0f/ _height);
    _mainPassConstantBuffer._nearZ = 1.0f;
    _mainPassConstantBuffer._farZ = 1000.0f;
    _mainPassConstantBuffer._totalTime = _timer.totalTime();
    _mainPassConstantBuffer._deltaTime = _timer.deltaTime();

    auto currPassCb = _currFrameResource->_passConstantBuffer.get();
    currPassCb->copyData(0, _mainPassConstantBuffer);
}

void SimluatorApp::updateCamera(const float dt)
{
    _eyePos.x = _radius * sinf(_phi) * cosf(_theta);
    _eyePos.y = _radius * cosf(_phi);
    _eyePos.z = _radius * sinf(_phi) * sinf(_theta);

    XMVECTOR pos = XMVectorSet(_eyePos.x, _eyePos.y, _eyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&_view, view);
 }

#if MULTIFLIHGT
void SimluatorApp::draw()
{
    if(!_swapChain) return;
    HRESULT result = S_OK;

    _flightIndex = _swapChain->GetCurrentBackBufferIndex();
    _device.waitForFlight(_flightIndex);

    // ComPtr<ID3D12GraphicsCommandList> commandList = _device.draw(_flightIndex, _pipelineStateObject);
    ComPtr<ID3D12GraphicsCommandList> commandList;
    if(_isWireFrame) 
        commandList = _device.draw(_flightIndex, _pipelineStateObjects["opaque_wireframe"]);
    else
        commandList = _device.draw(_flightIndex, _pipelineStateObjects["opaque"]);

    // D3D12_RESOURCE_BARRIER barrier;
    // {
    //     barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //     barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //     barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //     barrier.Transition.pResource = _renderTargets[_flightIndex].Get();
    //     barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //     barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    // }

    // commandList->ResourceBarrier(1, &barrier);
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(), 
                            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // auto rtvDescriptorSize = _device.getRtvDescSize();
    // auto dsvDescriptorSize = _device.getDsvDescSize();
    // CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart(), _flightIndex, rtvDescriptorSize);
    // CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart());
    commandList->ClearRenderTargetView(rtvHandle, Colors::LightSteelBlue, 0, nullptr);
    commandList->ClearDepthStencilView(_device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
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
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
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
#else
void SimluatorApp::draw()
{
    if(!_swapChain) return;
    HRESULT result = S_OK;


    auto cmdListAlloc = _currFrameResource->_cmdListAllocation;

    ThrowIfFailed(cmdListAlloc->Reset());

     _flightIndex = _swapChain->GetCurrentBackBufferIndex();
    // _device.waitForFlight(_flightIndex);

    ComPtr<ID3D12GraphicsCommandList> commandList = _device.getGraphicsCmdList(_flightIndex);
    if(_isWireFrame) 
    {
        ThrowIfFailed(commandList->Reset(cmdListAlloc.Get(), _pipelineStateObjects["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(commandList->Reset(cmdListAlloc.Get(), _pipelineStateObjects["opaque"].Get()));
    }
        

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(), 
                            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    
    auto cbbv = currentBackBufferView();

    commandList->RSSetViewports(1, &_viewPort);
    commandList->RSSetScissorRects(1, &_viewScissor);
    commandList->ClearRenderTargetView(cbbv, Colors::LightSteelBlue, 0, nullptr);
    commandList->ClearDepthStencilView(depthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &cbbv, true, &depthStencilView());
    commandList->SetGraphicsRootSignature(_pipelineRootSignature.Get());

    auto passConstantBuffer = _currFrameResource->_passConstantBuffer->Resource();
    commandList->SetGraphicsRootConstantBufferView(1, passConstantBuffer->GetGPUVirtualAddress());
    drawRenderItem(commandList.Get(), _renderItemLayer[(int)RenderLayer::Opaque]);

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(), 
                            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(commandList->Close());

    ID3D12CommandList *cmdLists[] = { commandList.Get() };
                    

    auto commandQueue = _device.getGraphicsCmdQueue();
    commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    ThrowIfFailed(_swapChain->Present(0, 0));
    
    _currFrameResource->_fences = ++_device._graphicsFenceValues;

    commandQueue->Signal(_device.getGraphicsFences().Get(), _device._graphicsFenceValues);
}
#endif

void SimluatorApp::onKeyEvent(unsigned char key, eKeyEvent event)
{
    //  if(GetAsyncKeyState('1') & 0x8000)
    //     _isWireFrame = true;
    // else
    //     _isWireFrame = false;

    if(key == '1' && event == eKeyEvent::eKeyDown)
    {
        _isWireFrame = !_isWireFrame;
    }
}

void SimluatorApp::onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y)
{
    switch (event)
    {
        case eMouseEvent::MouseDown:
        {
            _lasstMousePos.x = x;
            _lasstMousePos.y = y;

            SetCapture((HWND)_hwnd);
        }
            break;
        case eMouseEvent::MouseUp:
        {
            ReleaseCapture();
        }
            break;
        case eMouseEvent::MouseMove:
        {
            if((btn & eMouseButton::LButtonMouse) != 0)
            {
                auto dx = XMConvertToRadians(0.25f * static_cast<float>(x - _lasstMousePos.x));
                auto dy = XMConvertToRadians(0.25f * static_cast<float>(y - _lasstMousePos.y));

                _theta += dx;
                _phi += dy;

                _phi = MathHelper::Clamp(_phi, 0.1f, MathHelper::Pi - 0.1f);
            }
            else if((btn & eMouseButton::RButtonMouse) != 0)
            {
                auto dx = 0.005f * static_cast<float>(x - _lasstMousePos.x);
                auto dy = 0.00f * static_cast<float>(y - _lasstMousePos.y);

                _radius += dx - dy;
                _radius = MathHelper::Clamp(_radius, 3.0f, 15.0f);
            }
        }
            break;
    }

    _lasstMousePos.x = x;
    _lasstMousePos.y = y;
}

ID3D12Resource *SimluatorApp::currentBackBuffer() const
{
    return _renderTargets[_flightIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE SimluatorApp::currentBackBufferView() 
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(_device.getRtvHeap()->GetCPUDescriptorHandleForHeapStart(), _flightIndex, _device.getRtvDescSize());

}

D3D12_CPU_DESCRIPTOR_HANDLE SimluatorApp::depthStencilView()
{
    return _device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart();
}

void SimluatorApp::onKeyBoardInput(const float dt)
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