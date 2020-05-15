#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <windows.h>
#include <string>

namespace Nix {

inline std::wstring AnsiToWString(const std::string &str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class DxException
{
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

}



#endif