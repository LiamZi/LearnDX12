#include "SimluatorApp.hpp"

#include "../ThirdPart/Nix/Utils/Utils.hpp"


SimluatorApp::SimluatorApp()
{

}


bool SimluatorApp::initialize(void *hwnd, Nix::IArchive *archive)
{
    if(!hwnd) return false;
    
    _hwnd = hwnd;
    _archive = archive;

    if(!this->_device.initialize((HWND)hwnd)) return false;

    ComPtr<ID3D12Device> device = (ComPtr<ID3D12Device>)_device;

    createRootSignature(device);
    createShaderAndLayout(device);
    createFrameResouce(device);
    createPipelineStateObjects(device);
    
    createLandGeometry();
    createWavesGeometryBuffers();

    return true;
}

void SimluatorApp::createRootSignature(ComPtr<ID3D12Device> device)
{
    CD3DX12_ROOT_PARAMETER rootParameter[2];

    //create root cbv.
    rootParameter[0].InitAsConstantBufferView(0);
    rootParameter[1].InitAsConstantBufferView(1);

    //a root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC desc(2, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    //create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    auto result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
    if(errorBlob != nullptr) ::OutputDebugStringA((char *)errorBlob->GetBufferPointer());

    ThrowIfFailed(result);

    ThrowIfFailed(device->CreateRootSignature(0, serializedRootSignature.GetAddressOf(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(_pipelineRootSignature->GetAddressOf())))
}

void SimluatorApp::createShaderAndLayout(ComPtr<ID3D12Device> device)
{

}

void SimluatorApp::createPipelineStateObjects(ComPtr<ID3D12Device> device)
{

}

void SimluatorApp::createFrameResouce(ComPtr<ID3D12Device> device)
{

}

void SimluatorApp::createWavesGeometryBuffers()
{

}

void SimluatorApp::createLandGeometry()
{

}

void SimluatorApp::updateWaves(const float dt)
{

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