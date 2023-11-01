cbuffer SceneBuffer : register(b0)
{
    float4x4 vp;
};

struct VSInput
{
    float3 pos : POSITION;
    uint instanceID : SV_InstanceID;
    float3 worldPos : INSTANCEPOS;
    float density : INSTANCEDENSITY;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION;
    float4 color : COLOR;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    result.pos = mul(vp, float4(vertex.pos + vertex.worldPos, 1));
    result.worldPos = vertex.worldPos;
    float c = clamp(0.25, 1, log10(vertex.density - 7));
    result.color = float4(c, 0, 1 - c, 1);

    return result;
}
