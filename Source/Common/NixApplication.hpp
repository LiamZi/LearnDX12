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
#include <Windows.h>
#else
#endif

namespace Nix {
    class IRenderer;
    class IArchive;
}

static int frameCount = 0;
static float timeElapsed = 0.0f;

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


protected:
    Nix::Timer _timer;
    void *_hwnd;
    bool _minimized = false;
    bool _maximized = false;
    bool _resizing = false;
    bool _fullScreenState = false;
    bool _appPaused = false;
    uint32_t _width = 0;
    uint32_t _height = 0;

protected:
    void calculateFrameStats()
    {
        frameCount++;

        if(_timer.totalTime() - timeElapsed >= 1.0f) {
            float fps = (float)frameCount;
            auto mspf = 1000.0f / fps;

            auto fpsStr = std::to_string(fps);
            auto mspfStr = std::to_string(mspf);
            
            std::string title(title());
            auto text = title + "    fps: " + fpsStr + "    mspf: " + mspfStr;
            SetWindowTextA((HWND)_hwnd, text.c_str());
            // printf("%s\n", text.c_str());

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
    virtual void tick(float dt) = 0;
    virtual void draw() = 0;
    virtual char *title() { return "App"; };
    virtual uint32_t rendererType() = 0;

    virtual void pause() {};
    virtual void resume(void *hwn, uint32_t widht, uint32_t height) {};
    virtual void onKeyEvent(unsigned char key, eKeyEvent event) {};
    virtual void onMouseEvent(eMouseButton btn, eMouseEvent event, int x, int y) {};

public:
    inline Nix::Timer &getTimer() { return _timer; }
    inline void setAppPaused(const bool isPaused) { _appPaused = isPaused; }
    inline bool isAppPaused() const { return _appPaused; }
    inline void setMinimized(const bool isMinimized) { _minimized = isMinimized; }
    inline bool isMinimized() const { return _minimized; }
    inline void setMaximized(const bool isMaximized) { _maximized = isMaximized; }
    inline bool isMaximized() const { return _maximized; }
    inline void setResizing(const bool isResizing) { _resizing = isResizing; }
    inline bool isResizing() const { return _resizing; }
    inline void setFullScreen(const bool isFull) { _fullScreenState = isFull; }
    inline bool isFullScreen() const { return _fullScreenState; }
    inline void setWidth(const uint32_t width) { _width = width; }
    inline uint32_t getWidth() const { return _width; }
    inline void setHeight(const uint32_t height) { _height = height; }
    inline uint32_t getHeight() const { return _height; }


    int run() {
        MSG msg = {0};
        /* program main loop */
        bool bQuit = false;
        _timer.reset();
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
            {   
                _timer.tick();
                if(!_appPaused) {
                    calculateFrameStats();
                    tick(_timer.deltaTime());
                    draw();
                    // printf("calc Frame stats run ... \n");
                }
                else {
                    //eglSwapBuffers(context.display, context.surface);
                    Sleep(100);
                    // printf("Sleep 100. wait... \n");
                }
            }
        }

        return (int)msg.wParam;
    };
};

NixApplication *getApplication();