#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <Common.hpp>

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                    \
{                                                                           \
    HRESULT hr__ = (x);                                                     \
    std::wstring wfn = Nix::AnsiToWString(__FILE__);                        \
    if(FAILED(hr__)) { throw Nix::DxException(hr__, L#x, wfn, __LINE__); }  \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x) { x->Release(); x = 0;}}
#endif

namespace Nix {

inline std::wstring AnsiToWString(const std::string &str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class DxException {
public:
    HRESULT _errorCode = S_OK;
    std::wstring _functionName;
    std::wstring _fileName;
    int _lineNum = -1;
    
public:
    DxException(/* args */) = default;
    DxException(HRESULT result, const std::wstring &functionName, 
            const std::wstring& filename, int lineNumber);

    std::wstring toString() const;


};



class Utils {

    public:

        static uint32_t calcConstantBufferSize(uint32_t size)
        {
            return (size + 255) & ~255;
        }

        

        static Microsoft::WRL::ComPtr<ID3D12Resource> createBuffer(ID3D12Device *device, 
                                                ID3D12GraphicsCommandList *cmdList, 
                                                const void *data, uint64_t byteSize, 
                                                Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer);

        static Microsoft::WRL::ComPtr<ID3D12Resource> createUploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> &device,
                                                Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> &uploadCmdList,
                                                Microsoft::WRL::ComPtr<ID3D12CommandAllocator> &uploadCmdAllocator,
                                                Microsoft::WRL::ComPtr<ID3D12CommandQueue> &uploadCmdQueue,
                                                Microsoft::WRL::ComPtr<ID3D12Fence> &uploadFence,
                                                uint64_t uploadFenceValue,
                                                const void *data,
                                                uint64_t byteSize, 
                                                Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer);

        static Microsoft::WRL::ComPtr<ID3DBlob> compileShader(const std::wstring &filename, 
                                                const D3D_SHADER_MACRO *defines, 
                                                const std::string &entrypoint, 
                                                const std::string &target);
    };

}



#endif