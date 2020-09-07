#ifndef __DEVICE_HPP__
#define __DEVICE_HPP__


#include <Common.hpp>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace Nix;

class Device
{
private:
    ComPtr<IDXGIFactory4> _dxgiFactory;
    ComPtr<IDXGISwapChain> _swapChain;
    ComPtr<ID3D12Device> _d3dDevice;
    ComPtr<ID3D12Fence> _fence;
    ComPtr<ID3D12CommandQueue> _graphicsCmdQueue;
    ComPtr<ID3D12CommandAllocator> _graphicsCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> _graphicsCmdList;
    ComPtr<ID3D12Resource> _renderTargets[MaxFlightCount];
    ComPtr<ID3D12Resource> _depthStencilTargets;
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12DescriptorHeap> _dsvHeap;
    ComPtr<ID3D12RootSignature> _rootSignature;

    D3D12_VIEWPORT _viewPort;
    D3D12_RECT     _viewScissor;

    D3D_DRIVER_TYPE  _d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT _renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT _depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    void *_hwnd;

private:
   

    uint32_t _rtvDescSize = 0;
    uint32_t _dsvDescSize = 0;
    uint32_t _cbvSrvUavDescSize = 0;

    int _width = 800;
    int _height = 600;

    bool _4xMsaaState = false;
    bool _4xMsaaQuality = 0;

public:
    uint64_t _currentFence = 0;
    uint32_t _currFlightIndex = 0;

public:
    explicit Device(void *hwnd, uint32_t width, uint32_t height)
                                                            :_hwnd(hwnd)
                                                            ,_width(width)
                                                            ,_height(height) 
    {
        initialize();
    }

    Device(const Device &rhs) = delete;
    Device &operator=(const Device &rhs) = delete;

    ~Device() {}

    operator ComPtr<ID3D12Device> () const;

protected:
    bool initialize();
    void createCommandObjects();
    void createSwapChain();
    void createRtvAndDsvDescriptorHeaps();
    void logAdapters();
    void logAdapterOutputs(IDXGIAdapter *adapter);
    void logOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format);
   

public:
    void flushCommandQueue();
    void onResize(const uint32_t width = 0, const uint32_t height = 0);
    void swapChainResizeBuffer(const uint32_t width, const uint32_t height);
    void executeCommand();
    void resetGraphicsCmdList();
    void closeGraphicsCmdList();
    void createRootSignature();
    void createPipelineStateObjects(ComPtr<ID3D12PipelineState> &opaque, 
                                ComPtr<ID3D12PipelineState> &wire, 
                                std::unordered_map<std::string, ComPtr<ID3DBlob>> &shaders,
                                std::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout);

    void createShadersAndLayout(std::unordered_map<std::string, ComPtr<ID3DBlob>> &shaders, 
                            std::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout,
                            const std::wstring &vertexfilePath,
                            const std::wstring &fragFilePath);
                            
    void createDescriptorHeap(ComPtr<ID3D12DescriptorHeap> &cbvHeap, uint32_t descNum);
    void setViewPorts(uint32_t numViewPort);
    void setScissorRects(uint32_t scissorNum);

    void release();
    
public:
    ID3D12Resource *currRenderTarget() const;
    D3D12_CPU_DESCRIPTOR_HANDLE currRenderTargetView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView() const;

public:
    D3D12_VIEWPORT &getViewPort() { return _viewPort; }
    D3D12_RECT &getViewScissor() { return _viewScissor; }
    IDXGISwapChain *getSwapChina() { return _swapChain.Get(); }
    ID3D12Device *getD3DDevice() { return _d3dDevice.Get(); }
    ID3D12RootSignature *getRootSignature() { return _rootSignature.Get(); }
    ID3D12GraphicsCommandList *getGraphicsCmdList() { return _graphicsCmdList.Get(); }
    ComPtr<ID3D12Fence> &getFence() { return _fence; }
    ComPtr<ID3D12CommandQueue> &getGraphicsCmdQueue() { return _graphicsCmdQueue; }
};


#endif