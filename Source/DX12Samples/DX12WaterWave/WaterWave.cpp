#include "WaterWave.hpp"

#include "../ThirdPart/Nix/Utility/GeometryGenerator.hpp"


bool WaterWave::initialize(void *hwnd, Nix::IArchive *archive)
{
    _hwnd = hwnd;
    _width = 800;
    _height = 600;
    if(!_device) _device = new Device((HWND)hwnd, _width, _height);

    _device->resetGraphicsCmdList();

    _waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

    buildRootSignature();
    buildShadersAndInputLayout();
    buildWavesGeometry();
    
    buildRenderItems();
    buildFrameResources();
    // buildDescriptorHeaps();
    // buildConstantBufferViews();
    buildPipelineObjects();

    _device->closeGraphicsCmdList();

    _device->executeCommand();
    _device->flushCommandQueue();
    

    return true;
}

void WaterWave::resize(uint32_t width, uint32_t height)
{
    if(!_device || (width <= 0 && height <= 0)) return;
    _device->onResize(width, height);
    _width = width;
    _height = height;

    //the window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX point = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&_proj, point);
}

void WaterWave::release()
{
    OutputDebugStringA("WaterWave app release it. \n");
    if(_device) _device->release();
}

void WaterWave::tick(float dt)
{
    updateCamera(dt);

    //cycle through the circular frame resouce array.
    _currFrameResourceIndex = (_currFrameResourceIndex + 1) % MaxFlightCount;
    _currFrameResource = _frameResources[_currFrameResourceIndex].get();

    //has the gpu finished processing the commands of the current frame resouce?
    //if not, wait until the gpu has completed comamnds up to this fence point.
    auto fence = _device->getFence();
    if(_currFrameResource->_fences != 0 && fence->GetCompletedValue() < _currFrameResource->_fences)
    {
        auto eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(fence->SetEventOnCompletion(_currFrameResource->_fences, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    updateObjectConstantBuffers(dt);
    updateMainPassConstantBuffer(dt);
    updateWaves(dt);
}

void WaterWave::draw()
{
    auto cmdListAlloc = _currFrameResource->_cmdListAllocation;

    //resue the memory associated with command recording.
    ThrowIfFailed(cmdListAlloc->Reset());

    auto graphicsCmdList = _device->getGraphicsCmdList();
    auto currRenderTarget = _device->currRenderTarget();
    auto currRenderTargetView = _device->currRenderTargetView();
    auto depthView = _device->depthStencilView();
    auto viewPorts = _device->getViewPort();
    auto scissor = _device->getViewScissor();

    if(_isWireFrame)
    {
        ThrowIfFailed(graphicsCmdList->Reset(cmdListAlloc.Get(), _pipelineState["opaque_wireframe"].Get()));
    }   
    else
    {
        ThrowIfFailed(graphicsCmdList->Reset(cmdListAlloc.Get(), _pipelineState["opaque"].Get()));
    }
        
    
    graphicsCmdList->RSSetViewports(1, &viewPorts);
    graphicsCmdList->RSSetScissorRects(1, &scissor);
    graphicsCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    graphicsCmdList->ClearRenderTargetView(currRenderTargetView, Colors::LightSteelBlue, 0, nullptr);
    graphicsCmdList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    //specify the buffers we are going to render to.
    graphicsCmdList->OMSetRenderTargets(1, &currRenderTargetView, true, &depthView);

    // ID3D12DescriptorHeap *decsHeaps[] = { _cbvHeap.Get() };
    // graphicsCmdList->SetDescriptorHeaps(_countof(decsHeaps), decsHeaps);
    graphicsCmdList->SetGraphicsRootSignature(_device->getRootSignature());

    // int passCbvIndex = _passCbvOffset + _currFrameResourceIndex;
    // auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    // passCbvHandle.Offset(passCbvIndex, _cbvSrvUavDescriptorSize);
    // graphicsCmdList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    auto passCb = _currFrameResource->_passConstantBuffer->Resource();
    graphicsCmdList->SetGraphicsRootConstantBufferView(1, passCb->GetGPUVirtualAddress());

    drawRenderItems(graphicsCmdList, _renderItemLayer[(int)RenderLayer::Opaque]);
    // drawRenderItems(graphicsCmdList, _opaqueRenderItems);

    //indicate a state transition on the resouce usage.
    graphicsCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(graphicsCmdList->Close());

    //add the command list to the queue for execution.
    ID3D12CommandList *cmdsList[] = { graphicsCmdList };
    _device->getGraphicsCmdQueue()->ExecuteCommandLists(_countof(cmdsList), cmdsList);

    //swap the back and front buffers.
    ThrowIfFailed(_device->getSwapChina()->Present(0, 0));

    _device->_currFlightIndex = (_device->_currFlightIndex + 1) % MaxFlightCount;

    _currFrameResource->_fences = ++_device->_currentFence;

    _device->getGraphicsCmdQueue()->Signal(_device->getFence().Get(), _device->_currentFence);
}

void WaterWave::onKeyEvent(unsigned char key, eKeyEvent event)
{

}

void WaterWave::onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y)
{
    switch (btn)
    {
    case eMouseButton::LButtonMouse:
    {
        if(event == eMouseEvent::MouseDown) 
        { 
            _lastMousePos.x = x; 
            _lastMousePos.y = y;  
            SetCapture((HWND)_hwnd);
        }
        else if(event == eMouseEvent::MouseMove)
        {
            auto dx = XMConvertToRadians(0.25f * static_cast<float>(x - _lastMousePos.x));
            auto dy = XMConvertToRadians(0.25f * static_cast<float>(y - _lastMousePos.y));
            _theta += dx;
            _phi += dy;

            _phi =  MathHelper::Clamp(_phi, 0.1f, MathHelper::Pi - 0.1f);
        }
        else
        {
            ReleaseCapture();
        }
    }
        break;
    case eMouseButton::RButtonMouse:
    {
        if(event  == eMouseEvent::MouseDown)
        {
            _lastMousePos.x = x; 
            _lastMousePos.y = y;  
            SetCapture((HWND)_hwnd);
        }
        else if (event == eMouseEvent::MouseMove)
        {
            float dx = 0.05f * static_cast<float>(x - _lastMousePos.x);
            float dy = 0.05f * static_cast<float>(y - _lastMousePos.y);

            _radius += dx - dy;
            _radius = MathHelper::Clamp(_radius, 5.0f, 150.f);
        }
        else {
            ReleaseCapture();
        }
    }
        break;
    
    default:
        break;
    }

    _lastMousePos.x = x;
    _lastMousePos.y = y;
}

void WaterWave::buildDescriptorHeaps()
{
    uint32_t objCount = (uint32_t)_opaqueRenderItems.size();
    uint32_t numDescs = (objCount + 1) * MaxFlightCount;

    _passCbvOffset = objCount * MaxFlightCount;

    _device->createDescriptorHeap(_cbvHeap, numDescs);
}


void WaterWave::buildConstantBufferViews()
{
    uint32_t objCbByteSize = Utils::calcConstantBufferSize(sizeof(ConstantObject));

    auto objCount = _opaqueRenderItems.size();

    for (size_t frameIndex = 0; frameIndex < MaxFlightCount; frameIndex++)
    {
        auto objectCb = _frameResources[frameIndex]->_objectConstantBuffer->Resource();
        for (size_t i = 0; i < objCount; i++)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAdress = objectCb->GetGPUVirtualAddress();

            cbAdress += i * objCbByteSize;

            int heapIndex = frameIndex * objCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, _cbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAdress;
            cbvDesc.SizeInBytes = objCbByteSize;

            _device->getD3DDevice()->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    uint32_t passCBByteSize = Utils::calcConstantBufferSize(sizeof(PassConstants));

    for (size_t frameIndex = 0; frameIndex < MaxFlightCount; frameIndex++)
    {
        auto passCB = _frameResources[frameIndex]->_passConstantBuffer->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAdress = passCB->GetGPUVirtualAddress();

        //offset to the pass cbv in the descriptor heap.
        int heapIndex = _passCbvOffset + (int)frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, _cbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAdress;
        cbvDesc.SizeInBytes = passCBByteSize;

        _device->getD3DDevice()->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void WaterWave::buildRootSignature()
{
    if(!_device) return;
    _device->createRootSignature();
}

void WaterWave::buildShadersAndInputLayout()
{
    if(!_device) return;
    _device->createShadersAndLayout(_shaders, _inputLayout, 
                                L"bin\\shaders\\WaterWave\\water.hlsl", 
                               L"bin\\shaders\\WaterWave\\water.hlsl");

    if(_shaders.empty() || _inputLayout.empty()) assert(true);
}
    
void WaterWave::buildWavesGeometry()
{
  std::vector<std::uint16_t> indices(3 * _waves->triangleCount());
  assert(_waves->vertexCount() < 0x0000ffff);

  auto m = _waves->rowCount();
  auto n = _waves->columnCount();
  auto k = 0;

  for (int i = 0; i < m - 1; ++i)
  { 
    for (int j = 0; j < n - 1; ++j)
    {
        indices[k] = i * n + j;
        indices[k + 1] = i * n + j + 1;
        indices[k + 2] = (i + 1) * n + j;
        indices[k + 3] = (i + 1) * n + j;
        indices[k + 4] = i * n + j + 1;
        indices[k + 5] = (i + 1) * n + j + 1;

        k += 6;
    }
  }


  uint32_t vbByteSize = _waves->vertexCount() * sizeof(Vertex);
  uint32_t ibByteSize = (uint32_t)indices.size() * sizeof(std::uint16_t);

  auto geo = std::make_unique<MeshGeometry>();
  geo->setName("waterGeo");

  geo->setVertexBufferCPU(nullptr);
  geo->setVertexBufferGPU(nullptr);

  ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->getIndexBufferCPU()));
  CopyMemory(geo->getIndexBufferCPU()->GetBufferPointer(), indices.data(), ibByteSize);
  
  auto uploadIndexGPUBuffer = Utils::createBuffer(_device->getD3DDevice(), 
                                        _device->getGraphicsCmdList(), 
                                        indices.data(), 
                                        ibByteSize,
                                        geo->getIndexBufferUploader());

  geo->setIndexBufferGPU(uploadIndexGPUBuffer);

  geo->setVertexStride(sizeof(Vertex));
  geo->setVertexBufferSize(vbByteSize);
  geo->setIndexFormat(DXGI_FORMAT_R16_UINT);
  geo->setIndexBufferSize(ibByteSize);


  SubMeshGeometry subMesh;
  subMesh._indexCount = (uint32_t)indices.size();
  subMesh._startIndexLocation = 0;
  subMesh._baseVertexLocation = 0;

  geo->setDrawArgsElement("grid", subMesh);

  _geometries["waterGeo"] = std::move(geo);
}

void WaterWave::buildPipelineObjects()
{
    //pipeline state object for opaque objects;
    if(!_device) return;
    _device->createPipelineStateObjects(_pipelineState["opaque"], _pipelineState["opaque_wireframe"], _shaders, _inputLayout);
}


void WaterWave::buildFrameResources()
{
    for (size_t i = 0; i < MaxFlightCount; i++)
    {
        _frameResources.push_back(std::make_unique<FrameResource>(_device->getD3DDevice(), 1, (uint32_t)_allRenderItems.size(), _waves->vertexCount()));
    }
    
}

void WaterWave::buildRenderItems()
{
    auto wavesItem = std::make_unique<RenderItem>();
    wavesItem->_world = MathHelper::Identity4x4();
    wavesItem->_objContantBufferIndex = 0;
    wavesItem->_geo = _geometries["waterGeo"].get();
    wavesItem->_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wavesItem->_indexCount = wavesItem->_geo->getDrawArgsSubMeshByName("grid")->_indexCount;
    wavesItem->_startIndexLocation = wavesItem->_geo->getDrawArgsSubMeshByName("grid")->_startIndexLocation;
    wavesItem->_baseVertexLocation = wavesItem->_geo->getDrawArgsSubMeshByName("grid")->_baseVertexLocation;

    _waveRenderItem = wavesItem.get();

    _renderItemLayer[(int)RenderLayer::Opaque].push_back(wavesItem.get());
    _opaqueRenderItems.push_back(std::move(wavesItem.get())); 

    _allRenderItems.push_back(std::move(wavesItem));

    // auto gridItem = std::make_unique<RenderItem>();
    // gridItem->_world = MathHelper::Identity4x4();
    // gridItem->_objContantBufferIndex = 1;
    // gridItem->_geo = _geometries["shapeGeo"].get();
    // gridItem->_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    // auto gridSubMesh = gridItem->_geo->getDrawArgsSubMeshByName("grid");
    // gridItem->_indexCount = gridSubMesh->_indexCount;
    // gridItem->_startIndexLocation = gridSubMesh->_startIndexLocation;
    // gridItem->_baseVertexLocation = gridSubMesh->_baseVertexLocation;
    // _renderItemLayer[(int)RenderLayer::Opaque].push_back(gridItem.get());
    // _opaqueRenderItems.push_back(std::move(gridItem.get()));
    // _allRenderItems.push_back(std::move(gridItem));
    

}

void WaterWave::drawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &items)
{
    uint32_t objectConstantByteSize = Utils::calcConstantBufferSize(sizeof(ConstantObject));

    auto objectCb = _currFrameResource->_objectConstantBuffer->Resource();

    for (size_t i = 0; i < items.size(); i++)
    {
        auto renderItem = items[i];
        cmdList->IASetVertexBuffers(0, 1,  &renderItem->_geo->vertexBufferView());
        cmdList->IASetIndexBuffer(&renderItem->_geo->indexBufferView());
        cmdList->IASetPrimitiveTopology(renderItem->_primitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCbAddress = objectCb->GetGPUVirtualAddress();
        objCbAddress += renderItem->_objContantBufferIndex;

        cmdList->SetGraphicsRootConstantBufferView(0, objCbAddress);
        cmdList->DrawIndexedInstanced(renderItem->_indexCount, 1, renderItem->_startIndexLocation, renderItem->_baseVertexLocation, 0);

    }
    
}


void WaterWave::updateCamera(float dt)
{
    _eyePos.x = _radius * sinf(_phi) * cosf(_theta);
    _eyePos.z = _radius * sinf(_phi) * sinf(_theta);
    _eyePos.y = _radius * cosf(_phi);

    XMVECTOR pos = XMVectorSet(_eyePos.x, _eyePos.y, _eyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&_view, view);
}

void WaterWave::updateObjectConstantBuffers(float dt)
{
    auto currObjectCb = _currFrameResource->_objectConstantBuffer.get();
    for(auto &e : _allRenderItems)
    {
        if(e->_numFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->_world);
            ConstantObject objConstants;
            XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));
            currObjectCb->copyData(e->_objContantBufferIndex, objConstants);

            e->_numFramesDirty--;
        }
    }
}

void WaterWave::updateMainPassConstantBuffer(float dt)
{
    XMMATRIX view = XMLoadFloat4x4(&_view);
    XMMATRIX proj = XMLoadFloat4x4(&_proj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&_mainPassConstanBuffer._view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&_mainPassConstanBuffer._invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&_mainPassConstanBuffer._proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&_mainPassConstanBuffer._invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&_mainPassConstanBuffer._viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&_mainPassConstanBuffer._invViewProj, XMMatrixTranspose(invViewProj));
	_mainPassConstanBuffer._eyePosW = _eyePos;
	_mainPassConstanBuffer._renderTargetSize = XMFLOAT2((float)_width, (float)_height);
	_mainPassConstanBuffer._invRenderTargetSize = XMFLOAT2(1.0f / _width, 1.0f / _height);
	_mainPassConstanBuffer._nearZ = 1.0f;
	_mainPassConstanBuffer._farZ = 1000.0f;
	_mainPassConstanBuffer._totalTime = _timer.totalTime();
	_mainPassConstanBuffer._deltaTime = _timer.deltaTime();

	auto currPassCB = _currFrameResource->_passConstantBuffer.get();
	currPassCB->copyData(0, _mainPassConstanBuffer);
}

void WaterWave::updateWaves(float dt)
{
    static float base = 0.0f;
    if((_timer.totalTime() - base) >= 0.25f)
    {
        base += 0.25f;

        int i = MathHelper::Rand(4, _waves->rowCount() - 5);
        int j = MathHelper::Rand(4, _waves->columnCount() - 5);

        float r = MathHelper::RandF(0.2f, 0.5f);
        _waves->disturb(i, j, r);
    }

    _waves->update(_timer.deltaTime());

    auto currWaveVb = _currFrameResource->_waveVertexBuffer.get();
    for(int i = 0; i < _waves->vertexCount(); ++i)
    {
        Vertex v;
        v.pos = _waves->position(i);
        v.color = XMFLOAT4(DirectX::Colors::Blue);

        currWaveVb->copyData(i, v);
    }

    _waveRenderItem->_geo->setVertexBufferGPU(currWaveVb->Resource());

}

uint32_t WaterWave::rendererType()
{
    return 0;
}

float WaterWave::aspectRatio() const
{
    return static_cast<float>(_width / _height);
}




WaterWave theApp;

NixApplication *getApplication() { return &theApp;}