#ifndef __SIMULATOR_APP_HPP__
#define __SIMULATOR_APP_HPP__

#include <NixApplication.hpp>
#include "Device.hpp"


#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cstdio>

using namespace Microsoft::WRL;


class SimluatorApp : public NixApplication
{

private:
    Device _device;
    // ComPtr<IDXGIFactory4>                       _mdxgiFactory;
    // ComPtr<IDXGISwapChain>                      _swapChian;
    // ComPtr<ID3D12Device>                        _device;
    // ComPtr<ID3D12Fence>                         _fence;
    // ComPtr<ID3D12CommandQueue>                  _commandQueue;
    // ComPtr<ID3D12CommandAllocator>              _directCommandListAlloc;
    // ComPtr<ID3D12GraphicsCommandList>           _commandList;
    // ComPtr<ID3D12Resource>                      _swapChainBuffer[MaxFlightCount];
    // ComPtr<ID3D12Resource>                      _depthStencilBuffer;

    // ComPtr<ID3D12DescriptorHeap>                _rtvHeap;
    // ComPtr<ID3D12DescriptorHeap>                _dsvHeap;

    // D3D12_VIEWPORT                              _viewPort;
    // D3D12_RECT                                  _viewScissor;

    // uint64_t                                    _currFence = 0;
    // uint32_t                                    _currBackBuffer = 0;
    // uint32_t                                    _rtvDescriptorSize = 0;
    // uint32_t                                    _dsvDescriptorSize = 0;
    // uint32_t                                    _cbvSrvUavDescriptorSize = 0;
   
    
public:
    explicit SimluatorApp();
    virtual bool initialize(void* _wnd, Nix::IArchive*) override;
	virtual void resize(uint32_t _width, uint32_t _height) override;
	virtual void release() override;
	virtual void tick(float dt) override;
    virtual void draw() override;
	virtual char * title() override;
	virtual uint32_t rendererType() override;

public:
   
    
};

NixApplication* getApplication();

#endif