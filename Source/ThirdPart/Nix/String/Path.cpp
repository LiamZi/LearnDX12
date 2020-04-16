#include "Path.h"

namespace Nix
{
    std::string FormatFilePath(const std::string &filePath)
    {
        auto nSec = 0;
        std::string curSec = "";
        std::string fPath = ""; 
        if(filePath[0] == '/') { fPath.push_back('/'); }

        const char *ptr = filePath.c_str();
        while (*ptr != 0)
        {
            if(*ptr == '\\' || *ptr == '/')
            {
                if(curSec.length() > 0)
                {
                    if(curSec == ".") {}
                    else if(curSec == ".." && nSec >= 2)
                    {
                        int secleft = 2;
                        while (!(fPath.empty() && secleft == 0))
                        {
                            if(fPath.back() == '\\' || fPath.back() == '/')
                            {
                                --secleft;
                                break;
                            }
                            fPath.pop_back();
                        }
                        fPath.pop_back();
                    }
                    else 
                    {
                        if(!fPath.empty() && fPath.back() != '/') fPath.push_back('/');
                        fPath.append(curSec);
                        ++nSec;
                    }
                    curSec.clear();
                }
            }
            else 
            {
                curSec.push_back(*ptr);
                if(*ptr == ':') --nSec;
            }
            ++ptr;
        }

        if(curSec.length() > 0)
        {
            if(!fPath.empty() && fPath.back() != '/') fPath.push_back('/');
            fPath.append(curSec);
        }

        return fPath;
    }
} // namespace Nix