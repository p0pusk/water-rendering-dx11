#include "shaders/SceneCB.h"

struct Triangle
{
    float3 v[3];
    float3 normal;
};

StructuredBuffer<Triangle> triangles : register(t0);

struct VSOutput
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION;
    float4 color : COLOR;
    float3 norm : NORMAL;
};

VSOutput vs(uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID) {
  VSOutput result;

  float3 pos = triangles[instanceID].v[vertexID];

  result.pos = mul(vp, float4(pos, 1));
  result.worldPos = pos;
  result.color = float4(0.2, 0.65, 0.75, 0.1);
  result.norm = triangles[instanceID].normal;

  return result;
}
