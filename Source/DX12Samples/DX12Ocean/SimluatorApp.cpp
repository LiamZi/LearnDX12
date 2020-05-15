#include "SimluatorApp.hpp"

#include "../ThirdPart/Nix/Utils/Utils.hpp"


SimluatorApp::SimluatorApp()
{

}


bool SimluatorApp::initialize(void *hwnd, Nix::IArchive *archive)
{
    if(!hwnd) return false;
    
    _hwnd = hwnd;

    return true;
}


void SimluatorApp::resize(uint32_t _width, uint32_t _height)
{

}

void SimluatorApp::release()
{

}

void SimluatorApp::tick(float dt)
{

}

void SimluatorApp::draw()
{

}

char * SimluatorApp::title()
{
    return "Ocean Simluator";
}

uint32_t SimluatorApp::rendererType()
{
    return 0;
}

SimluatorApp theapp;

NixApplication* getApplication() {
    return &theapp;
}