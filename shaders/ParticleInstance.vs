#include "shaders/SceneCB.h"

struct Particle
{
    float3 position;
    float density;
    float pressure;
    float3 force;
    float3 velocity;
    uint hash;
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
    uint particleIndex : ParticleIndex;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    result.pos = mul(vp, float4(particles[vertex.instanceID].position + vertex.pos, 1));
    result.worldPos = particles[vertex.instanceID].position + vertex.pos;
    result.particleIndex = vertex.instanceID;

    return result;
}
