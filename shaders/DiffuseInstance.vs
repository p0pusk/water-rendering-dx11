#include "SceneCB.hlsli"

struct DiffuseParticle {
  float3 position;
  float3 velocity;
  // 0 - spray, 1 - foam, 2 - bubbles
  uint type;
  uint origin;
  float lifetime;
  uint neighbours;
};


StructuredBuffer<DiffuseParticle> particles : register(t0);

struct VSInput
{
    uint instanceID : SV_InstanceID;
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

    result.pos = mul(vp, float4(particles[vertex.instanceID].position, 1));
    result.worldPos = particles[vertex.instanceID].position;
    uint type = particles[vertex.instanceID].type;
    if (type == 0) {
      result.color = float4(0, 0, 1, 1);
    } else if (type == 1) {
      result.color = float4(0, 1, 0, 1);
    } else {
      result.color = float4(1, 0, 0, 1);
    }

     result.color= float4(0.5, 0.5, 0.5, 0.5);

    return result;
}
