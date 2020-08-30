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
    ComPtr<IDXGISwapChain3>                 _swapChain;

    //graphics queue
    ComPtr<ID3D12CommandQueue>              _graphicsCommandQueue;
    // ComPtr<ID3D12GraphicsCommandList>       _graphicsCommandLists[MaxFlightCount];
    // ComPtr<ID3D12CommandAllocator>          _graphicsCommandAllocator[MaxFlightCount];
    ComPtr<ID3D12CommandAllocator>          _graphicsCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>       _graphicsCommandLists;

    //upload queue
    ComPtr<ID3D12CommandQueue>              _uploadQueue;
    ComPtr<ID3D12CommandAllocator>          _uploadCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>       _uploadCommandList;
    ComPtr<ID3D12Fence>                     _uploadFence;
    HANDLE                                  _uploadFenceEvent;

    ComPtr<ID3D12DescriptorHeap>            _rtvHeap;
    ComPtr<ID3D12DescriptorHeap>            _dsvHeap;

    //cpu and graphics fance
#if MULTIFLIHGT
    ComPtr<ID3D12Fence>                     _graphicsFences[MaxFlightCount];
#else
    ComPtr<ID3D12Fence>                     _graphicsFences;
#endif

    HANDLE                                  _graphicsFenceEvent;


    uint32_t                                _rtvDescriptorSize = 0;
    uint32_t                                _dsvDescriptorSize = 0;
    uint32_t                                _cbvSrvUavDescriptorSize = 0;

    uint32_t                                _4xMsaaQuality = 0;
    bool                                    _4xMsaaState = false;

    HWND                                    _hwnd;
    uint32_t                                _width;
    uint32_t                                _height;
    uint32_t                                _flightIndex = 0;
    uint64_t                                _uploadFenceValue = 0;
   
public:
#if MULTIFLIHGT
    uint64_t                                _graphicsFenceValues[MaxFlightCount];
#else
    uint64_t                                _graphicsFenceValues = 0;
#endif

   
public:
    explicit Device(/* args */);
    ~Device();

    ComPtr<IDXGISwapChain3> createSwapChian(const uint32_t width, const uint32_t height);

    void set4xMassState(const bool value);
    bool get4xMassState() const { return _4xMsaaState; }
    uint32_t get4xMassQuality() const { return _4xMsaaQuality; }
    void flushGraphicsCommandQueue();

#if MULTIFLIHGT
    void waitForFlight(uint32_t flight);
#else
    void waitForFlight(uint32_t flight = 0);
#endif
    // ComPtr<ID3D12GraphicsCommandList> draw(uint32_t flightIndex, ComPtr<ID3D12PipelineState> &pipelineStete);
    void executeCommand(ComPtr<ID3D12GraphicsCommandList> &commandList);
    void createRtvAndDsvDescriptorHeaps();
    
    operator ComPtr<ID3D12Device> () const;

    inline ComPtr<IDXGISwapChain3> &getSwapChain()
    {
        return _swapChain;
    }

    inline const uint32_t getRtvDescSize() const
    {
        return _rtvDescriptorSize;
    }

    inline const uint32_t getDsvDescSize() const 
    {
        return _dsvDescriptorSize;
    }

    inline const uint32_t getCbvDescSize() const 
    {
        return _cbvSrvUavDescriptorSize;
    }

    inline ComPtr<ID3D12GraphicsCommandList> getUploadCommandList() 
    {
        return _uploadCommandList;
    }

    inline ComPtr<ID3D12CommandAllocator> getUploadCommandAllocation()
    {
        return _uploadCommandAllocator;
    }

    inline ComPtr<ID3D12CommandQueue> getUploadCommandQueue()
    {
        return _uploadQueue;
    }

    inline ComPtr<ID3D12Fence> getUploadFence()
    {
        return _uploadFence;
    }

    inline void setUploadFenceValue(const uint64_t value)
    {
        _uploadFenceValue = value;
    }

    inline uint64_t getUploadFenceValue()
    {
        return _uploadFenceValue;
    }

    inline ComPtr<ID3D12DescriptorHeap> &getRtvHeap()
    {
        return _rtvHeap;
    }

    inline ComPtr<ID3D12DescriptorHeap> &getDsvHeap()
    {
        return _dsvHeap;
    }

    inline ComPtr<ID3D12CommandQueue> &getGraphicsCmdQueue()
    {
        return _graphicsCommandQueue;
    }

    inline ComPtr<ID3D12GraphicsCommandList> &getGraphicsCmdList(const uint32_t flightIndex)
    {
#if MULTIFLIHGT
        return _graphicsCommandLists[flightIndex];
#else
        return _graphicsCommandLists;
#endif
    }

    inline ComPtr<ID3D12CommandAllocator> &getGraphicsCmdAllocator(const uint32_t flightIndex)
    {
#if MULTIFLIHGT
        // return _graphicsCommandAllocator[flightIndex];
#else
        return _graphicsCommandAllocator;
#endif
    }

    inline ComPtr<ID3D12Fence> &getGraphicsFences()
    {
        return _graphicsFences;
    }

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
