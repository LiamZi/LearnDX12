#ifndef __OCEAN_FFT_HPP__
#define __OCEAN_FFT_HPP__

#include <NixApplication.hpp>
#include "Device.hpp"

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


using namespace Microsoft::WRL;
using namespace DirectX;

class OceanFFT : public NixApplication
{
private:
   Device _device;
public:
    explicit OceanFFT(/* args */) {}
    ~OceanFFT() {}


public:
    void resize(uint32_t width, uint32_t height) override;
    void release() override;
    void tick(float dt) override;
    void draw() override;
    uint32_t rendererType() override;
    virtual char *title() { return "OceanTTF"; };

private:

};


NixApplication *getApplication();

#endif