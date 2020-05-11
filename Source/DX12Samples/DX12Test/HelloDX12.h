#include <NixApplication.hpp>
//#include <nix/io/archieve.h>
#include <cstdio>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <algorithm>

constexpr uint32_t MaxFlightCount = 3;

class HelloDX12 : public NixApplication {
private:
    void*                   m_hwnd;
    IDXGIFactory4*          m_dxgiFactory;
    IDXGIAdapter1*          m_dxgiAdapter;
    ID3D12Device*           m_dxgiDevice;
    ID3D12CommandQueue*     m_commandQueue;
    IDXGISwapChain3*        m_swapchain;
    uint32_t                m_flightIndex;
    //
    ID3D12DescriptorHeap*   m_rtvDescriptorHeap;
    uint32_t                m_rtvDescriptorSize;
    ID3D12Resource*         m_renderTargets[MaxFlightCount];
    ID3D12CommandAllocator* m_commandAllocators[MaxFlightCount];
    ID3D12GraphicsCommandList* m_commandList;
    ID3D12Fence*            m_fence[MaxFlightCount];
    //
    HANDLE                  m_fenceEvent;
    UINT64                  m_fenceValue[MaxFlightCount];
    bool                    m_running;

public:
	virtual bool initialize(void* _wnd, Nix::IArchive*) override;
	virtual void resize(uint32_t _width, uint32_t _height) override;
	virtual void release() override;
	virtual void tick(double dt) override;
    virtual void draw() override;
	virtual char * title() override;
	virtual uint32_t rendererType() override;
};

NixApplication* getApplication();