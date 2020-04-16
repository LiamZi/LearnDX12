#include <NixApplication.h>
#include <cstdio>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <algorithm>


constexpr uint32_t MaxFlightCount = 3;

class HelloTriangle : public NixApplication
{
public:
   explicit HelloTriangle(/* args */);
    ~HelloTriangle();

public:
	virtual bool initialize(void* _wnd, Nix::IArchive*) override;
	virtual void resize(uint32_t _width, uint32_t _height) override;
	virtual void release() override;
	virtual void tick() override;
	virtual const char * title() override;
	virtual uint32_t rendererType() override;

private:
    void                    *m_hwnd;
    IDXGIFactory4           *m_dxgiFactory;
    IDXGIAdapter1           *m_dxgiAdapter;
    ID3D12Device            *m_dxgiDevice;
    ID3D12CommandQueue      *m_commandQueue;
    IDXGISwapChain3         *m_swapchain;
    uint32_t                m_flightIndex;
    //
    ID3D12DescriptorHeap    *m_rtvDescriptorHeap;
    uint32_t                m_rtvDescriptorSize;
    ID3D12Resource          *m_renderTargets[MaxFlightCount];
    ID3D12CommandAllocator  *m_commandAllocators[MaxFlightCount];
    ID3D12GraphicsCommandList *m_commandList;
    ID3D12Fence             *m_fence[MaxFlightCount];
    //
    HANDLE                  m_fenceEvent;
    UINT64                  m_fenceValue[MaxFlightCount];
    bool                    m_running;
    Nix::IArchive           *_archive;

    //pso 状态对象
    ID3D12PipelineState     *_pipelineStateObject;
    //root signature defines data shaders will access.
    ID3D12RootSignature     *_pipelineRootSignature;
    //视口
    D3D12_VIEWPORT          _viewPort;
    //默认的裁剪矩形
    D3D12_RECT              _scissorRect;
    //默认的顶点资源集
    ID3D12Resource          *_vertexBuffer;
    //a structer containing a pointer to  the vertex data in gpu memory
    //the total size of the buffer, and the size of each element(vertex)
    //相当于GL 的vertex buffer
    D3D12_VERTEX_BUFFER_VIEW _vertexBufferView; 


};