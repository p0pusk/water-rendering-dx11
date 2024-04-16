#include "shaders/SceneCB.h"

struct Particle
{
    float3 position;
    float density;
    float3 pressureGrad;
    float pressure;
    float4 viscosity;
    float4 force;
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
    float3 centerPos : PARTICLE_POS;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    result.pos = mul(vp, float4(particles[vertex.instanceID].position + vertex.pos, 1));
    result.worldPos = particles[vertex.instanceID].position + vertex.pos;
    result.centerPos = particles[vertex.instanceID].position;

    return result;
}
