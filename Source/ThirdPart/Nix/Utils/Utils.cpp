#include "Utils.hpp"
#include <comdef.h>
#include <fstream>

using namespace Microsoft::WRL;
namespace Nix {
    DxException::DxException(HRESULT result, const std::wstring &functionName, 
            const std::wstring& filename, int lineNumber)
        :_errorCode(result)
        ,_functionName(functionName)
        ,_fileName(filename)
        ,_lineNum(lineNumber)
    {
        
    }


    Microsoft::WRL::ComPtr<ID3D12Resource> Utils::createBuffer(ID3D12Device *device, 
                                            ID3D12GraphicsCommandList *cmdList, 
                                            const void *data, uint64_t byteSize, 
                                            Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer)
    {

        Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

        //create the actual default buffer resouce.
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
                            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(buffer.GetAddressOf())));
        //in order to copy cpu memory data into our default buffer, we need to create an intermediate upload heap.
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize),  
                            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

        //describe the data we want to copy into the default buffer.
        D3D12_SUBRESOURCE_DATA subResourceData = {};
        {
            subResourceData.pData = data;
            subResourceData.RowPitch = byteSize;
            subResourceData.SlicePitch = subResourceData.RowPitch;
        }

        //schedule to copy the data to the default buffer resource. at a high level, the helper function UpdateSubresource
        //will copy the cpu memory into the intermediate upload heap. then using ID3D12CommandList::CopySubresourceRegion,
        //the intermediate upload heap data willl be copied to buffer.
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

        UpdateSubresources<1>(cmdList, buffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

        return buffer;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> Utils::createUploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> &device,
                                                ComPtr<ID3D12GraphicsCommandList> &uploadCmdList,
                                                ComPtr<ID3D12CommandAllocator> &uploadCmdAllocator,
                                                ComPtr<ID3D12CommandQueue> &uploadCmdQueue,
                                                Microsoft::WRL::ComPtr<ID3D12Fence> &uploadFence,
                                                uint64_t uploadFenceValue,
                                                const void *data,
                                                uint64_t byteSize,
                                                Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

        // Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer;

          //create the actual default buffer resouce.
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
                            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(buffer.GetAddressOf())));

        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize),  
                            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
        
        //describe the data we want to copy into the default buffer.
        D3D12_SUBRESOURCE_DATA subResourceData = {};
        {
            subResourceData.pData = data;
            subResourceData.RowPitch = byteSize;
            subResourceData.SlicePitch = subResourceData.RowPitch;
        }

        uploadBuffer->SetName(L"Geometry vertex buffer staging buffer.");
        void *mappedPtr = nullptr;
        uploadBuffer->Map(0, nullptr, &mappedPtr);

        //store vertex buffer in staging buffer;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT resourceFootprint;
        {
            uint32_t numRow[1];
            uint64_t rowSizesInBytes[1];
            uint64_t byteTotal;
            device->GetCopyableFootprints(&buffer->GetDesc(), 0, 
                                        1, 0, &resourceFootprint,
                                        numRow, rowSizesInBytes, &byteTotal);
        }

        CopyMemory(mappedPtr, data, byteSize);

        uploadCmdAllocator->Reset();
        uploadCmdList->Reset(uploadCmdAllocator.Get(), nullptr);
        uploadCmdList->CopyBufferRegion(buffer.Get(), 0, uploadBuffer.Get(), 0, byteSize);

        auto barrierDesc = CD3DX12_RESOURCE_BARRIER::Transition( buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ );
        uploadCmdList->ResourceBarrier(1, &barrierDesc);
        uploadCmdList->Close();

        ID3D12CommandList *lists[] = { uploadCmdList.Get() };
        uploadCmdQueue->ExecuteCommandLists(1, lists);
        uploadCmdQueue->Signal(uploadFence.Get(), ++uploadFenceValue);

        if(uploadFence->GetCompletedValue() < uploadFenceValue)
        {
            auto event = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
            auto result = uploadFence->SetEventOnCompletion(uploadFenceValue, event);
            if(FAILED(result)) { assert(false); return nullptr; }

            WaitForSingleObject(event, INFINITE);
        }

        return buffer;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> Utils::compileShader(const std::wstring &filename, 
                                            const D3D_SHADER_MACRO *defines, 
                                            const std::string &entrypoint, 
                                            const std::string &target)
    {
        uint32_t compileFlag = 0;
    #if defined(DEBUG) || defined(_DEBUG)
        compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_VALIDATION;
    #endif

        HRESULT result = S_OK;
        ComPtr<ID3DBlob> byteCode = nullptr;
        ComPtr<ID3DBlob> errors;

        result = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), target.c_str(), compileFlag, 0, &byteCode, &errors);

        if(errors != nullptr) OutputDebugStringA((char *)errors->GetBufferPointer());

        ThrowIfFailed(result);

        return byteCode;
    }
}


