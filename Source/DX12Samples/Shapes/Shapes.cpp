#include "Shapes.hpp"
#include <GeometryGenerator.hpp>
#include <Vertex.hpp>


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

}

void Shapes::draw()
{

}

void Shapes::onKeyEvent(unsigned char key, eKeyEvent event)
{

}

void Shapes::onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y)
{

}

void Shapes::buildDescriptorHeaps()
{

}


void Shapes::buildConstantBufferViews()
{

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
    MeshData box = geoGen.createBox(1.5f, 0.5f, 1.5f, 3);
    MeshData grid = geoGen.createGrid(20.0f, 30.0f, 60, 40);
    MeshData sphere = geoGen.createSphere(0.5f, 20, 20);
    MeshData cylinder = geoGen.createCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    //we are concatenating all the geometry into on big vertex/index buffer.
    //so define the regions in the buffer each submesh covers.

    //cache the vertex offsets to each object in the consatenated vertex buffer.

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

}

void Shapes::buildRenderItems()
{

}

void Shapes::drawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &items)
{

}


void Shapes::updateCamera(float dt)
{

}

void Shapes::updateObjectConstantBuffers(float dt)
{

}

void Shapes::updateMainPassConstantBuffer(float dt)
{
    
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