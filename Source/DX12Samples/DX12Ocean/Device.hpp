#ifndef __DEVICE_HPP__
#define __DEVICE_HPP__

#include <Common.hpp>



using namespace Microsoft::WRL;

constexpr uint32_t MaxFlightCount = 2;

class Device
{
private:
    /* data */
    ComPtr<ID3D12Debug>                     _debugController;
    ComPtr<ID3D12Device>                    _device;
    ComPtr<IDXGIFactory4>                   _factory;

    //graphics queue
    ComPtr<ID3D12CommandQueue>              _graphicsCommandQueue;
    ComPtr<ID3D12GraphicsCommandList>       _graphicsCommandLists[MaxFlightCount];
    ComPtr<ID3D12CommandAllocator>          _graphicsCommandAllocator[MaxFlightCount];

    //upload queue
    ComPtr<ID3D12CommandQueue>              _uploadQueue;
    ComPtr<ID3D12CommandAllocator>          _uploadCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>       _uploadCommandList;
    ComPtr<ID3D12Fence>                     _uploadFence;
    HANDLE                                  _uploadFenceEvent;

    //cpu and graphics fance
    ComPtr<ID3D12Fence>                     _graphicsFences[MaxFlightCount];
    HANDLE                                  _graphicsFenceEvent;
    uint64_t                                _graphicsFenceValues[MaxFlightCount];

    uint32_t                                _rtvDescriptorSize;
    uint32_t                                _dsvDescriptorSize;

    uint32_t                                _4xMsaaQuality = 0;
    bool                                    _4xMsaaState = false;

    HWND                                    _hwnd;
    uint32_t                                _width;
    uint32_t                                _height;
    uint32_t                                _flightIndex;
    uint64_t                                _uploadFenceValue;

public:
    explicit Device(/* args */);
    ~Device();

    ComPtr<IDXGISwapChain3> createSwapChian(const uint32_t width, const uint32_t height);

    float AspectRatio() const { return static_cast<float>(_width) / _height; }
    void set4xMassState(const bool value);
    bool get4xMassState() const { return _4xMsaaState; }
    void flushGraphicsCommandQueue();
    void waitForFlight(uint32_t flight);
    ComPtr<ID3D12GraphicsCommandList> draw(uint32_t flightIndex, ComPtr<ID3D12PipelineState> &pipelineStete);
    void executeCommand(ComPtr<ID3D12GraphicsCommandList> &commandList);
    
    operator ComPtr<ID3D12Device> () const;

protected:
    void logAdapters();
    void logAdapterOutputs(IDXGIAdapter *dapater);
    void logOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format);
    void createCommnadObjectsAndFence();
    void createUploadCommandObjectsAndFence();
    void check4xMsaaSupport();

public:
    bool initialize(HWND hwnd, uint32_t width = 0, uint32_t height = 0);
};

#endif
