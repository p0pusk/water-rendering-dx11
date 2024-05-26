#include "shaders/SceneCB.h"

struct Particle {
  float3 position;
  float density;
  float pressure;
  float3 force;
  float3 velocity;
  uint hash;
  float3 normal;
  float3 externalForces;
};


StructuredBuffer<Particle> particles : register(t0);

struct VSInput
{
    float3 pos : POSITION;
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

    result.pos = mul(vp, float4(particles[vertex.instanceID].position + vertex.pos, 1));
    result.worldPos = particles[vertex.instanceID].position + vertex.pos;

    float min = 30.f;
    float max = 3000.f;
    float t = smoothstep(min, max, particles[vertex.instanceID].density);
    result.color = lerp(float4(0, 0.9, 1, 1), float4(0, 0, 1, 1), t);

    return result;
}
