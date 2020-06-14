#ifndef __SIMULATOR_APP_HPP__
#define __SIMULATOR_APP_HPP__

#include <NixApplication.hpp>
#include "Device.hpp"


#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cstdio>
#include <memory>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace Nix;

class MeshGeometry;

struct ConstantObject
{
    DirectX::XMFLOAT4X4 worldViewProj = Nix::MathHelper::Identity4x4();
};


class SimluatorApp : public NixApplication
{

private:
    Device _device;
    Nix::IArchive *_archive;
    // ComPtr<IDXGIFactory4>                       _mdxgiFactory;
    ComPtr<IDXGISwapChain3>                      _swapChain;
    // ComPtr<ID3D12Device>                        _device;
    // ComPtr<ID3D12Fence>                         _fence;
    // ComPtr<ID3D12CommandQueue>                  _commandQueue;
    // ComPtr<ID3D12CommandAllocator>              _directCommandListAlloc;
    // ComPtr<ID3D12GraphicsCommandList>           _commandList;
    ComPtr<ID3D12Resource>                      _renderTargets[MaxFlightCount];
    ComPtr<ID3D12Resource>                      _depthStencilBuffer;

    ComPtr<ID3D12DescriptorHeap>                _cbvHeap;
    ComPtr<ID3D12PipelineState>                 _pipelineStateObject;
    ComPtr<ID3D12RootSignature>                 _pipelineRootSignature;

    ComPtr<ID3DBlob>                            _vsShader;
    ComPtr<ID3DBlob>                            _fragShader;

    std::vector<D3D12_INPUT_ELEMENT_DESC>       _inputLayout;

    D3D12_VIEWPORT                              _viewPort;
    D3D12_RECT                                  _viewScissor;

    std::unique_ptr<MeshGeometry>               _boxGeometry;

    // std::unique_ptr<UploadBuffer<ObjectConstants>> _cbv;

    // uint64_t                                    _currFence = 0;
    // uint32_t                                    _currBackBuffer = 0;
    uint32_t                                    _flightIndex = 0;

    std::unique_ptr<UploadBuffer<ConstantObject>> _cbvObj;

    DirectX::XMFLOAT4X4 _world  = Nix::MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _view   = Nix::MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 _proj   = Nix::MathHelper::Identity4x4();

    float _theta                = 1.5 * DirectX::XM_PI;
    float _phi                  = DirectX::XM_PIDIV4;
    float _radius               = 5.0f;

    POINT _lasstMousePos;

   
public:
    explicit SimluatorApp();
    virtual bool initialize(void* _wnd, Nix::IArchive*) override;
	virtual void resize(uint32_t _width, uint32_t _height) override;
	virtual void release() override;
	virtual void tick(float dt) override;
    virtual void draw() override;
	virtual char * title() override;
	virtual uint32_t rendererType() override;
    virtual void onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y) override;

private:
    void createRootSignature(ComPtr<ID3D12Device> device);
    void createShaderAndLayout(ComPtr<ID3D12Device> device);
    void createPipelineStateObjects(ComPtr<ID3D12Device> device);
    void createFrameResouce(ComPtr<ID3D12Device> device);
    void createWavesGeometryBuffers();
    void createLandGeometry();
    void createCubeGeometry(ComPtr<ID3D12Device> device);
    void createConstantBuffers(ComPtr<ID3D12Device> device);
    void createDescriptorHeap(ComPtr<ID3D12Device> device);
    

    void updateWaves(const float dt);

    float AspectRatio() const { return static_cast<float>(_width) / _height; }
};

NixApplication* getApplication();

#endif