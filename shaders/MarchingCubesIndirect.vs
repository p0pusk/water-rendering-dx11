cbuffer SceneBuffer : register(b0)
{
    float4x4 vp;
};

struct Triangle
{
    float3 v[3];
};

StructuredBuffer<Triangle> triangles : register(t0);

struct VSInput
{
    uint instanceID : SV_InstanceID;
    uint vertexID : SV_VertexID;
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

    float3 pos = triangles[vertex.instanceID].v[vertex.vertexID];

    result.pos = mul(vp, float4(pos, 1));
    result.worldPos = pos;
    float c = abs(0.05 - pos.y) * 100;
    // result.color = lerp(float4(0, 0.25, 0.75, 0.5), float4(1, 0, 0, 1), c);
    result.color = float4(0.2, 0.85, 0.95, 0.3);

    return result;
}
