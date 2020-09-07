// cbuffer cbPerObject : register(b0)
// {
//     float4x4 world;
// };

// cbuffer cbPass : register(b1)
// {
//     float4x4 view;
//     float4x4 invView;
//     float4x4 proj;
//     float4x4 invProj;
//     float4x4 viewProj;
//     float4x4 invViewProj;
//     float3 eyePosW;
//     float cbPerObjectPad1;
//     float2 renderTargetSize;
//     float2 invRenderTargetSize;
//     float nearZ;
//     float farZ;
//     float totalTime;
//     float deltaTime; 
// };

// struct vertexIn
// {
//     float3 pos : POSITION;
//     float4 color : COLOR;
// };


// struct vertexOut
// {
//     float4 pos : SV_POSITION;
//     float4 color : COLOR;
// };

// vertexOut vsMain(vertexIn vin)
// {
//     vertexOut vout;

//     //transform to homegeneous clip space.
//     float4 posW = mul(float4(vin.pos, 1.0f), world);
//     vout.pos = mul(posW, viewProj);

//     vout.color = vin.color;
//     // vout.color = float4(1.0, 0.0, 0.0, 1.0);
//     return vout;
// };


// float4 psMain(vertexOut vin) : SV_Target
// {
//     return vin.color;
// };



//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut vsMain(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

float4 psMain(VertexOut pin) : SV_Target
{
    return pin.Color;
}


