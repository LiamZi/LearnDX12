#include <NixApplication.h>
#include <cstdio>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <algorithm>
#include <wrl/client.h>


constexpr uint32_t MaxFlightCount = 3;

using namespace Microsoft::WRL;

class DeviceDX12 {
public:
    DeviceDX12() {};

    ComPtr<IDXGISwapChain3> createSwapChian(HWND hwnd, uint32_t width, uint32_t height);
    ComPtr<ID3D12GraphicsCommandList> onTick(uint64_t dt, uint32_t flightIndex);
    void executeCommand(ComPtr<ID3D12GraphicsCommandList> &commandList);
    void waitForFlight(uint32_t flight);
    void waitCopyQueue();
    void flushGraphicsQueue();
    ComPtr<ID3D12Resource> createVertexBuffer(const void *data, size_t length);
    void uploadBuffer(ComPtr<ID3D12Resource> vertexBuffer, const void *data, size_t length);
    ComPtr<ID3D12Resource> createTexture();

    operator ComPtr<ID3D12Device> () const;

private:
    bool initialize();

private:
    ComPtr<ID3D12Debug>                     _debugController;
    ComPtr<IDXGIFactory4>                   _factory;
    ComPtr<IDXGIAdapter>                    _warpAdapter;
    ComPtr<IDXGIAdapter1>                   _hardwareAdapter;
    ComPtr<ID3D12Device>                    _device;

    //graphics queue
    ComPtr<ID3D12CommandQueue>              _graphicsCommandQueue;
    ComPtr<ID3D12GraphicsCommandList>       _graphicsCommandLists[MaxFlightCount];
    ComPtr<ID3D12CommandAllocator>          _graphicsCommandAllocator[MaxFlightCount];

    //upload queue

    ComPtr<ID3D12CommandQueue>              _uploadQueue;
    ComPtr<ID3D12CommandAllocator>          _uploadCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>       _uploadCommandList;

    ComPtr<ID3D12Fence>                     _graphicsFences[MaxFlightCount];
    HANDLE                                  _graphicsFenceEvent;
    uint64_t                                _graphicsFenceValues[MaxFlightCount];

    bool                                    _running;
    ComPtr<ID3D12Fence>                     _uploadFence;
    HANDLE                                  _uploadFenceEvent;
    uint64_t                                _uploadFenceValue;
    uint32_t                                _flightIndex;
};

class TriangleDelux : public NixApplication
{
private:
    Nix::IArchive                           *_archive;
    DeviceDX12                              _deivce;
    void                                    *_hwnd;
    
    //initialize 
    ComPtr<IDXGISwapChain3>                 _swapChain;
    ComPtr<ID3D12DescriptorHeap>            _rtvDescrpitorHeap;
    ComPtr<ID3D12DescriptorHeap>            _pipelineDescriptorHeap;
    ComPtr<ID3D12Resource>                  _renderTargets[MaxFlightCount];

    uint32_t                                _rtvDescriptorSize;

    ID3D12PipelineState                     *_pipelineStateObject;
    ID3D12RootSignature                     *_rootSignature;
    D3D12_VIEWPORT                          _viewPort;
    D3D12_RECT                              _viewScissor;

    ComPtr<ID3D12Resource>                  _vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                _vertexBufferView;
    ComPtr<ID3D12Resource>                  _simpleTexture;



public:
    explicit TriangleDelux(/* args */);
    ~TriangleDelux();

    virtual bool initialize(void *wnd, Nix::IArchive *);
    virtual void resize(uint32_t width, uint32_t height);
    virtual void release();
    virtual void tick();
    virtual const char *title();
    virtual uint32_t rendererType();

};

NixApplication *getApplication();
