cbuffer cbPerObject:register(b0)
{
    float4x4 _world;
};


cbuffer cbPass:register(b1)
{
    float4x4 _view;
    float4x4 _invView;
    float4x4 _proj;
    float4x4 _invProj;
    float4x4 _viewProj;
    float4x4 _invViewProj;
    float3 _eyePosW;
    float _cbPerObjectPad1;
    float2 _renderTargetSize;
    float2 _invRenderTargetSize;
    float _nearZ;
    float _farZ;
    float _totalTime;
    float _deltaTime;
};


struct VertexIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};


VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    float4 posW = mul(float4(vin.pos, 1.0f), _world);
    vout.pos = mul(posW, _viewProj);
    vout.color = vin.color;
    return vout;
}

float4 PS(VertexOut pin) :SV_Target
{
    return pin.color;
}




