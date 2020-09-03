#ifndef __SHAPES_HPP__
#define __SHAPES_HPP__

#include <NixApplication.hpp>
#include "Device.hpp"

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "../ThirdPart/Nix/Utility/FrameResource.hpp"
#include "../ThirdPart/Nix/Utility/PassConstants.hpp"
#include "../ThirdPart/Nix/Utility/ConstantObject.hpp"
#include "../ThirdPart/Nix/Utility/RenderItem.hpp"


using namespace Microsoft::WRL;
using namespace DirectX;
using namespace Nix;

class Shapes : public NixApplication
{
private:
    Device *_device;

    std::vector<std::unique_ptr<FrameResource>> _frameResources;
    FrameResource *_currFrameResource = nullptr;

    
    ComPtr<ID3D12DescriptorHeap> _cbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> _srvDescHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> _geometries;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> _shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> _pipelineState;

    std::vector<D3D12_INPUT_ELEMENT_DESC> _inputLayout;
    std::vector<std::unique_ptr<RenderItem>> _allRenderItems;
    std::vector<RenderItem *> _opaqueRenderItems;

    PassConstants _mainPassConstanBuffer;

    XMFLOAT4X4 _proj = MathHelper::Identity4x4();
    XMFLOAT4X4 _view = MathHelper::Identity4x4();
    XMFLOAT3 _eyePos = { 0.0f, 0.0f, 0.0f };
    bool _isWireFrame = false;

    float _theta = 1.5f * XM_PI;
    float _phi = 0.2f * XM_PI;
    float _radius = 15.0f;
    int _currFrameResourceIndex = 0;
    uint32_t _passCbvOffset = 0;

    POINT _lastMousePos;

    uint32_t _rtvDescriptorSize = 0;
    uint32_t _dsvDescriptorSize = 0;
    uint32_t _cbvSrvUavDescriptorSize = 0;

public:
    explicit Shapes(/* args */) {}
    Shapes(const Shapes &rhs) = delete;
    Shapes &operator=(const Shapes &rhs) = delete;

    ~Shapes() {
        //TODO: 刷新主渲染队列
        if(_device != nullptr) _device->flushCommandQueue();
    }

public:
    bool initialize(void *hwnd, Nix::IArchive *archive) override;
    void resize(uint32_t width, uint32_t height) override;
    void release() override;
    void tick(float dt) override;
    void draw() override;
    uint32_t rendererType() override;
    char *title() override { return "Shapes App"; }
    float aspectRatio() const;

    virtual void onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y) override;
    virtual void onKeyEvent(unsigned char key, eKeyEvent event) override;

private:
    void buildDescriptorHeaps();
    void buildConstantBufferViews();
    void buildRootSignature();
    void buildShadersAndInputLayout();
    void buildShapeGeometry();
    void buildPipelineObjects();
    void buildFrameResources();
    void buildRenderItems();
    void drawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &items);


    void updateCamera(float dt);
    void updateObjectConstantBuffers(float dt);
    void updateMainPassConstantBuffer(float dt);
};

NixApplication *getApplication();

#endif