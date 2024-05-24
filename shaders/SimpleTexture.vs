#include "shaders/SceneCB.h"

cbuffer GeomBuffer : register (b1)
{
    float4x4 model;
    float4x4 norm;
    float4 shine; // x - shininess
};

struct VSInput
{
    float3 pos : SV_POSITION;
    float3 norm : NORMAL;
    float4 tang : TANGENT;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    float4 worldPos = mul(model, float4(vertex.pos, 1));

    result.pos = mul(vp, worldPos);
    result.worldPos = worldPos.xyz;
    result.uv = float2(vertex.uv.x, 1 - vertex.uv.y);
    result.tang = normalize(mul(norm, vertex.tang).xyz);
    result.norm = normalize(mul(norm, float4(vertex.norm, 0)));

    return result;
}
