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
#include <string> 
#include "../ThirdPart/Nix/Timer/Timer.h"

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

    bool _appPaused = false;

protected:
    Nix::Timer _timer;
    void *_hwnd;

protected:
    void calculateFrameStats()
    {
        static int frameCount = 0;
        static float timeElapsed = 0.0f;

        frameCount++;

        if(_timer.totalTime() - timeElapsed >= 1.0f) {
            float fps = (float)frameCount;
            auto mspf = 1000.0f / fps;

            auto fpsStr = std::to_string(fps);
            auto mspfStr = std::to_string(mspf);
            
            std::string title(title());
            auto text = title + "    fps: " + fpsStr + "    mspf: " + mspfStr;
            SetWindowTextA((HWND)_hwnd, text.c_str());

            frameCount = 0;
            timeElapsed += 1.0f;
        }
    };
    

public:
    explicit NixApplication():_hwnd(nullptr) {}
    virtual bool initialize(void * wnd, Nix::IArchive *archive)  {
        _hwnd = wnd;
        return true;
    };

    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void release() = 0;
    virtual void tick(double dt) = 0;
    virtual void draw() = 0;
    virtual char *title() { return "App"; };
    virtual uint32_t rendererType() = 0;

    virtual void pause() {};
    virtual void resume(void *hwn, uint32_t widht, uint32_t height) {};
    virtual void onKeyEvent(unsigned char key, eKeyEvent event) {};
    virtual void onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y) {};

    int run() {
        MSG msg = {0};
        /* program main loop */
        bool bQuit = false;
        while (!bQuit)
        {
            /* check for messages */
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                /* handle or dispatch messages */
                if (msg.message == WM_QUIT)
                {
                    bQuit = TRUE;
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else
            {   _timer.tick();
                if(!_appPaused) {
                    calculateFrameStats();
                    tick(_timer.deltaTime());
                    draw();
                }
                else {
                    //eglSwapBuffers(context.display, context.surface);
                    Sleep(100);
                }
            }
        }

        return (int)msg.wParam;
    };
};

NixApplication *getApplication();