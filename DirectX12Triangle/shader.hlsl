cbuffer VSConstants : register(b0)
{
    float4x4 gMVP;     // world * view * proj (transposed on CPU)
};

struct VSIn {
    float3 pos   : POSITION;
    float3 color : COLOR;
};

struct VSOut {
    float4 posH  : SV_Position;
    float3 col   : COLOR;
};

VSOut VSMain(VSIn vin)
{
    VSOut o;
    o.posH = mul(float4(vin.pos, 1.0f), gMVP);
    o.col  = vin.color;
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
    return float4(i.col, 1.0f);
}
