#ifndef __OCEAN_NEW_HPP__
#define __OCEAN_NEW_HPP__

#include <NixApplication.hpp>
#include "Device.hpp"

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


using namespace Microsoft::WRL;
using namespace DirectX;

class OceanNew : public NixApplication
{
private:
   Device _device;
public:
    explicit OceanNew(/* args */) {}
    ~OceanNew() {}


public:
    void resize(uint32_t width, uint32_t height) override;
    void release() override;
    void tick(float dt) override;
    void draw() override;
    uint32_t rendererType() override;

private:

};


NixApplication *getApplication();

#endif