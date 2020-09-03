#include "Shapes.hpp"
#include "../ThirdPart/Nix/Utility/GeometryGenerator.hpp"


bool Shapes::initialize(void *hwnd, Nix::IArchive *archive)
{
    _hwnd = hwnd;
    _width = 800;
    _height = 600;
    if(!_device) _device = new Device((HWND)hwnd, _width, _height);

    _device->resetGraphicsCmdList();

    buildRootSignature();
    buildShadersAndInputLayout();
    buildShapeGeometry();
    buildRenderItems();
    buildFrameResources();
    buildDescriptorHeaps();
    buildConstantBufferViews();
    buildPipelineObjects();

    _device->closeGraphicsCmdList();

    _device->executeCommand();
    _device->flushCommandQueue();
    

    return true;
}

void Shapes::resize(uint32_t width, uint32_t height)
{
    if(!_device || (width <= 0 && height <= 0)) return;
    _device->onResize(width, height);
    _width = width;
    _height = height;

    //the window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX point = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&_proj, point);
}

void Shapes::release()
{
    OutputDebugStringA("Shapes app release it. \n");
    if(_device) _device->release();
}

void Shapes::tick(float dt)
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
}

void Shapes::draw()
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

    ID3D12DescriptorHeap *decsHeaps[] = { _cbvHeap.Get() };
    graphicsCmdList->SetDescriptorHeaps(_countof(decsHeaps), decsHeaps);
    graphicsCmdList->SetGraphicsRootSignature(_device->getRootSignature());

    int passCbvIndex = _passCbvOffset + _currFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCbvIndex, _cbvSrvUavDescriptorSize);
    graphicsCmdList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    drawRenderItems(graphicsCmdList, _opaqueRenderItems);

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

void Shapes::onKeyEvent(unsigned char key, eKeyEvent event)
{

}

void Shapes::onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y)
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

void Shapes::buildDescriptorHeaps()
{
    uint32_t objCount = (uint32_t)_opaqueRenderItems.size();
    uint32_t numDescs = (objCount + 1) * MaxFlightCount;

    _passCbvOffset = objCount * MaxFlightCount;

    _device->createDescriptorHeap(_cbvHeap, numDescs);
}


void Shapes::buildConstantBufferViews()
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
        int heapIndex = _passCbvOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, _cbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAdress;
        cbvDesc.SizeInBytes = passCBByteSize;

        _device->getD3DDevice()->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void Shapes::buildRootSignature()
{
    if(!_device) return;
    _device->createRootSignature();
}

void Shapes::buildShadersAndInputLayout()
{
    if(!_device) return;
    _device->createShadersAndLayout(_shaders, _inputLayout);
    if(_shaders.empty() || _inputLayout.empty()) assert(true);
}
    
void Shapes::buildShapeGeometry()
{
    GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.createBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGen.createGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.createSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.createCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    // we are concatenating all the geometry into on big vertex/index buffer.
    // so define the regions in the buffer each submesh covers.

    // cache the vertex offsets to each object in the consatenated vertex buffer.

    uint32_t boxVertexOffset = 0;
    auto gridVertexOffset = (uint32_t)box._vertices.size();
    auto sphereVertexOffset = gridVertexOffset + (uint32_t)grid._vertices.size();
    auto cylinderVertexOffset = sphereVertexOffset + (uint32_t)sphere._vertices.size();


    //cache the starting index for each object in the concatenated index buffer.
    uint32_t boxIndexOffset = 0;
    auto gridIndexOffset = (uint32_t)box._indices32.size();
    auto sphereIndexOffset = gridIndexOffset + (uint32_t)grid._indices32.size();
    auto cylinderIndexOffset = sphereIndexOffset + (uint32_t)sphere._indices32.size();

    //define the submeshGeometry that cover different
    //regions of the vertex/index buffers.

    SubMeshGeometry boxSubMesh;
    boxSubMesh._indexCount = (uint32_t)box._indices32.size();
    boxSubMesh._startIndexLocation = boxIndexOffset;
    boxSubMesh._baseVertexLocation = boxVertexOffset;

    SubMeshGeometry gridSubMesh;
    gridSubMesh._indexCount = (uint32_t)grid._indices32.size();
    gridSubMesh._startIndexLocation = gridIndexOffset;
    gridSubMesh._baseVertexLocation = gridVertexOffset;

    SubMeshGeometry sphereSubMesh;
    sphereSubMesh._indexCount = (uint32_t)sphere._indices32.size();
    sphereSubMesh._startIndexLocation = sphereIndexOffset;
    sphereSubMesh._baseVertexLocation = sphereVertexOffset;

    SubMeshGeometry cylinderSubMesh;
    cylinderSubMesh._indexCount = (uint32_t)cylinder._indices32.size();
    cylinderSubMesh._startIndexLocation = cylinderIndexOffset;
    cylinderSubMesh._baseVertexLocation = cylinderVertexOffset;


    //extrcat the vertex elements we are interested in and pack 
    //the vertices of all meshes into on vertex buffer.

    auto totalVertexCount = box._vertices.size() + grid._vertices.size()
                            + sphere._vertices.size() + cylinder._vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    //TODO: using namespace distinguish Vertex struct.
    
    uint32_t k = 0;
    for (size_t i = 0; i < box._vertices.size(); ++i, ++k)
    {
        vertices[k].pos = box._vertices[i]._position;
        vertices[k].color = XMFLOAT4(Colors::DarkGreen);
    }

    for (size_t i = 0; i < grid._vertices.size(); ++i, ++k)
    {
        vertices[k].pos = grid._vertices[i]._position;
        vertices[k].color = XMFLOAT4(Colors::ForestGreen);
    }

    for (size_t i = 0; i < sphere._vertices.size(); ++i, ++k)
    {
        vertices[k].pos = sphere._vertices[i]._position;
        vertices[k].color = XMFLOAT4(Colors::Crimson);
    }

    for (size_t i = 0; i < cylinder._vertices.size(); ++i, ++k)
    {
        vertices[k].pos = cylinder._vertices[i]._position;
        vertices[k].color = XMFLOAT4(Colors::SteelBlue);
    }
    

    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(box.getIndices16()), std::end(box.getIndices16()));
    indices.insert(indices.end(), std::begin(grid.getIndices16()), std::end(grid.getIndices16()));
    indices.insert(indices.end(), std::begin(sphere.getIndices16()), std::end(sphere.getIndices16()));
    indices.insert(indices.end(), std::begin(cylinder.getIndices16()), std::end(cylinder.getIndices16()));


    const uint32_t vbByteSize = (uint32_t)vertices.size() * sizeof(Vertex);
    const uint32_t ibByteSize = (uint32_t)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->setName("shapeGeo");

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->getVertexBufferCPU()));
    CopyMemory(geo->getVertexBufferCPU()->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->getIndexBufferCPU()));
    CopyMemory(geo->getIndexBufferCPU()->GetBufferPointer(), indices.data(), ibByteSize);

    auto vertexBufferGPU = Utils::createBuffer(_device->getD3DDevice(), 
                                            _device->getGraphicsCmdList(), 
                                            vertices.data(), 
                                            vbByteSize, 
                                            geo->getVertexBufferUploader());

    if(!vertexBufferGPU) assert(!vertexBufferGPU);
    geo->setVertexBufferGPU(vertexBufferGPU);

    auto indexBufferGPU = Utils::createBuffer(_device->getD3DDevice(),
                                            _device->getGraphicsCmdList(),
                                            indices.data(),
                                            ibByteSize,
                                            geo->getIndexBufferUploader());
    if(!indexBufferGPU) assert(!indexBufferGPU);

    geo->setIndexBufferGPU(indexBufferGPU);

    geo->setVertexStride(sizeof(Vertex));
    geo->setVertexBufferSize(vbByteSize);
    geo->setIndexFormat(DXGI_FORMAT_R16_UINT);
    geo->setIndexBufferSize(ibByteSize);

    geo->setDrawArgsElement("box", boxSubMesh);
    geo->setDrawArgsElement("grid", gridSubMesh);
    geo->setDrawArgsElement("sphere", sphereSubMesh);
    geo->setDrawArgsElement("cylinder", cylinderSubMesh);

    _geometries[geo->getName()] = std::move(geo);
}

void Shapes::buildPipelineObjects()
{
    //pipeline state object for opaque objects;
    if(!_device) return;
    _device->createPipelineStateObjects(_pipelineState["opaque"], _pipelineState["opaque_wireframe"], _shaders, _inputLayout);
}


void Shapes::buildFrameResources()
{
    for (size_t i = 0; i < MaxFlightCount; i++)
    {
        _frameResources.push_back(std::make_unique<FrameResource>(_device->getD3DDevice(), 1, (uint32_t)_allRenderItems.size(), 0));
    }
    
}

void Shapes::buildRenderItems()
{
    auto boxItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxItem->_world, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    boxItem->_objContantBufferIndex = 0;
    boxItem->_geo = _geometries["shapeGeo"].get();
    boxItem->_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    auto subMesh = boxItem->_geo->getDrawArgsSubMeshByName("box");
    boxItem->_indexCount = subMesh->_indexCount;
    boxItem->_startIndexLocation = subMesh->_startIndexLocation;
    boxItem->_baseVertexLocation = subMesh->_baseVertexLocation;
    // _allRenderItems.push_back(std::move(boxItem));

    auto gridItem = std::make_unique<RenderItem>();
    gridItem->_world = MathHelper::Identity4x4();
    gridItem->_objContantBufferIndex = 1;
    gridItem->_geo = _geometries["shapeGeo"].get();
    gridItem->_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    auto gridSubMesh = gridItem->_geo->getDrawArgsSubMeshByName("grid");
    gridItem->_indexCount = gridSubMesh->_indexCount;
    gridItem->_startIndexLocation = gridSubMesh->_startIndexLocation;
    gridItem->_baseVertexLocation = gridSubMesh->_baseVertexLocation;
    _allRenderItems.push_back(std::move(gridItem));

    for(auto &e : _allRenderItems) _opaqueRenderItems.push_back(e.get());

}

void Shapes::drawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &items)
{
    uint32_t objectConstantByteSize = Utils::calcConstantBufferSize(sizeof(ConstantObject));

    auto objectCb = _currFrameResource->_objectConstantBuffer->Resource();

    for (size_t i = 0; i < items.size(); i++)
    {
        auto renderItem = items[i];
        cmdList->IASetVertexBuffers(0, 1,  &renderItem->_geo->vertexBufferView());
        cmdList->IASetIndexBuffer(&renderItem->_geo->indexBufferView());
        cmdList->IASetPrimitiveTopology(renderItem->_primitiveType);

        uint32_t cbvIndex = _currFrameResourceIndex * (uint32_t)_opaqueRenderItems.size() + renderItem->_objContantBufferIndex;
        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(_cbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(cbvIndex, _cbvSrvUavDescriptorSize);
        
        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
        cmdList->DrawIndexedInstanced(renderItem->_indexCount, 1, renderItem->_startIndexLocation, renderItem->_baseVertexLocation, 0);
    }
    
}


void Shapes::updateCamera(float dt)
{
    _eyePos.x = _radius * sinf(_phi) * cosf(_theta);
    _eyePos.z = _radius * sinf(_phi) * cosf(_theta);
    _eyePos.y = _radius * cosf(_phi);

    XMVECTOR pos = XMVectorSet(_eyePos.x, _eyePos.y, _eyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&_view, view);
}

void Shapes::updateObjectConstantBuffers(float dt)
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

void Shapes::updateMainPassConstantBuffer(float dt)
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
	_mainPassConstanBuffer._totalTime = dt;
	_mainPassConstanBuffer._deltaTime = dt;

	auto currPassCB = _currFrameResource->_passConstantBuffer.get();
	currPassCB->copyData(0, _mainPassConstanBuffer);
}

uint32_t Shapes::rendererType()
{
    return 0;
}

float Shapes::aspectRatio() const
{
    return static_cast<float>(_width / _height);
}


Shapes theApp;

NixApplication *getApplication() { return &theApp;}