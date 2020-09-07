#ifndef __WATER_WAVE_HPP__
#define __WATER_WAVE_HPP__

#include <NixApplication.hpp>
#include "Device.hpp"
#include "Waves.hpp"

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


class WaterWave : public NixApplication
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
    std::vector<RenderItem *> _renderItemLayer[(int)RenderLayer::Count];

    PassConstants _mainPassConstanBuffer;
    std::unique_ptr<Waves> _waves;
    RenderItem *_waveRenderItem = nullptr;

    XMFLOAT4X4 _proj = MathHelper::Identity4x4();
    XMFLOAT4X4 _view = MathHelper::Identity4x4();
    XMFLOAT3 _eyePos = { 0.0f, 0.0f, 0.0f };
    bool _isWireFrame = true;

    float _theta = 1.5f * XM_PI;
    float _phi = XM_PIDIV2 - 0.1f;
    float _radius = 50.0f;
    float _sunTheta = 1.25f * XM_PI;
    float _sunPhi = XM_PIDIV4;
    int _currFrameResourceIndex = 0;
    uint32_t _passCbvOffset = 0;

    POINT _lastMousePos;

    uint32_t _rtvDescriptorSize = 0;
    uint32_t _dsvDescriptorSize = 0;
    uint32_t _cbvSrvUavDescriptorSize = 0;



public:
    explicit WaterWave(/* args */) {}
    WaterWave(const WaterWave &rhs) = delete;
    WaterWave &operator=(const WaterWave &rhs) = delete;

    ~WaterWave() {
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
    char *title() override { return "WaterWave App"; }
    float aspectRatio() const;

    virtual void onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y) override;
    virtual void onKeyEvent(unsigned char key, eKeyEvent event) override;

private:
    void buildDescriptorHeaps();
    void buildConstantBufferViews();
    void buildRootSignature();
    void buildShadersAndInputLayout();
    void buildWavesGeometry();
    void buildPipelineObjects();
    void buildFrameResources();
    void buildRenderItems();
    void drawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &items);


    void updateCamera(float dt);
    void updateObjectConstantBuffers(float dt);
    void updateMainPassConstantBuffer(float dt);
    void updateWaves(float dt);

    float getHillsHeight(float x, float z) const;
    XMFLOAT3 getHillsNormal(float x, float z) const;
};

NixApplication *getApplication();

#endif