#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<Potential> potentials : register(t1);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u0);
RWStructuredBuffer<State> state : register(u1);

struct OrthogonalVectors
{
    float3 v1;
    float3 v2;
};

OrthogonalVectors ComputeOrthogonalVectors(float3 normal)
{
    float3 v1, v2;

    if (abs(normal.x) > abs(normal.y))
    {
        v1 = float3(-normal.z, 0, normal.x);
    }
    else
    {
        v1 = float3(0, normal.z, -normal.y);
    }
    v1 = normalize(v1);

    v2 = normalize(cross(normal, v1));

    OrthogonalVectors result;
    result.v1 = v1;
    result.v2 = v2;

    return result;
}

float Hash(uint seed)
{
    seed = (seed << 13u) ^ seed;
    return 1.0f - ((seed * (seed * seed * 15731u + 789221u) + 1376312589u) & 0x7fffffffu) / 1073741824.0f;
}

float RandomFloat(uint seed)
{
    return frac(Hash(seed));
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  float3 velocity = particles[DTid.x].velocity;
  if (velocity.x == 0 && velocity.y == 0 && velocity.z == 0) {
    return;
  }

  uint kTA = 5;
  uint kWC = 10;
  int toSpawn = potentials[DTid.x].energy * (kTA *
    potentials[DTid.x].trappedAir + kWC * potentials[DTid.x].waveCrest);
  int index = diffuseParticlesNum - state[0].curDiffuseNum;
  int n = min(toSpawn, index);
  for (int i = 0; i < n ; ++i) {
    InterlockedAdd(state[0].curDiffuseNum, 1, index);
    OrthogonalVectors basis = ComputeOrthogonalVectors(normalize(velocity));
    float r = h * sqrt(RandomFloat(3 * index));
    float theta = RandomFloat(3 * index + 1) * 2 * PI;
    float dist = RandomFloat(3 * index + 2) * length(dt * velocity);
    diffuse[index].position = particles[DTid.x].position + r * cos(theta) * basis.v1
      + r * sin(theta) * basis.v2 + dist * normalize(velocity);
    diffuse[index].velocity = particles[DTid.x].position + r * cos(theta) * basis.v1
      + r * sin(theta) * basis.v2 + velocity;
    uint neighbours = particles[DTid.x].density / mass / W(h/2, h);
    diffuse[index].lifetime = 1.f;
  }
}
