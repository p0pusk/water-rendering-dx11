#include "./SceneCB.hlsli"

cbuffer GeomBuffer : register (b1)
{
    float4x4 model;
    float4x4 normM;
    float4 shine; // x - shininess
    float4 color;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 worldPos : POSITION;
    float4 color : COLOR;
    float3 norm : NORMAL;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    float4 worldPos = mul(model, float4(vertex.pos, 1.0));

    result.pos = mul(vp, worldPos);
    result.worldPos = worldPos;
    result.norm = normalize(mul(normM, float4(vertex.norm, 0)).xyz);
    result.color = color;

    return result;
}
