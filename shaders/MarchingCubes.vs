cbuffer SceneBuffer : register(b0)
{
    float4x4 vp;
};

struct VSInput
{
    float3 pos : POSITION;
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

    result.pos = mul(vp, float4(vertex.pos, 1));
    result.worldPos = vertex.pos;
    float c = abs(0.05 - vertex.pos.y) * 100;
    // result.color = lerp(float4(0, 0.25, 0.75, 0.5), float4(1, 0, 0, 1), c);
    result.color = float4(0.25, 0.5, 0.75, 0.5);

    return result;
}
