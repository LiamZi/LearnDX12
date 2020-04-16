/*
 * @Author: yanzi1221
 * @Email: yanzi1221@hotmail.com
 * @Date: 2020-04-08 18:19:24
 * @Last Modified by: yanzi1221
 * @Last Modified time: 2020-04-08 23:17:34
 * @Description:  此头文件我的朋友 bhlzlx 感谢他的贡献
 */


#pragma once
#include <cstdint>

#ifdef _WIN32
#include <WINDOWS.h>
#else
#endif

namespace Nix {
    class IRenderer;
    class IArchive;
}

class NixApplication {
    
    public:
        enum eKeyEvent
        {
            eKeyDown,
            eKeyUp,
        };

        enum eMouseButton
        {
            MouseButtonNone,
            LButtonMouse,
            MButtonMouse,
            RButtonMouse,
        };

        enum eMouseEvent
        {
            MouseMove,
            MouseDown,
            MouseUp,
        };


        virtual bool initialize(void * wnd, Nix::IArchive *archive) = 0;
        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual void release() = 0;
        virtual void tick() = 0;
        virtual const char *title() = 0;
        virtual uint32_t rendererType() = 0;

        virtual void pause() {};
        virtual void resume(void *hwn, uint32_t widht, uint32_t height) {};
        virtual void onKeyEvent(unsigned char key, eKeyEvent event) {};
        virtual void onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y) {};
};

NixApplication *getApplication();