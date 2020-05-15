#include "Utils.hpp"
#include <comdef.h>
#include <fstream>

namespace Nix {
    DxException::DxException(HRESULT result, const std::wstring &functionName, 
            const std::wstring& filename, int lineNumber)
        :_errorCode(result)
        ,_functionName(functionName)
        ,_fileName(filename)
        ,_lineNum(lineNumber)
    {
        
    }
}


