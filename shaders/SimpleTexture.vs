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
    float3 tang : TANGENT;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 worldPos : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    float4 worldPos = mul(model, float4(vertex.pos, 1.0));

    result.pos = mul(vp, worldPos);
    result.worldPos = worldPos;
    result.uv = float2(vertex.uv.x, 1 - vertex.uv.y);
    result.tang = mul(norm, float4(vertex.tang, 0)).xyz;
    result.norm = mul(norm, float4(vertex.norm, 0)).xyz;

    return result;
}
