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
  result.color = float4(1, 1, 1, 0.02);
  result.norm = triangles[instanceID].normal;

  return result;
}
