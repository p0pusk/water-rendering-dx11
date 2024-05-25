#include "shaders/Sph.h"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u1);
RWStructuredBuffer<State> state : register(u2);

void CheckBoundary(in uint index)
{
    float padding = 4 * marchingWidth;
#ifdef DIFFUSE
    float3 localPos = diffuse[index].position - worldPos;
    float3 velocity = diffuse[index].velocity;
#else
    float3 localPos = particles[index].position - worldPos;
    float3 velocity = particles[index].velocity;
#endif

    if (localPos.y < padding)
    {
        localPos.y = -localPos.y + 2 * padding;
        velocity *= -dampingCoeff;
    }

    if (localPos.y > -padding + boundaryLen.y)
    {
        localPos.y = -localPos.y + 2 * (-padding + boundaryLen.y);
        velocity.y *= -dampingCoeff;
    }

    if (localPos.x < padding)
    {
        localPos.x = -localPos.x + 2 * padding;
        velocity.x *= -dampingCoeff;
    }

    if (localPos.x > -padding + boundaryLen.x)
    {
        localPos.x = -localPos.x + 2 * (-padding + boundaryLen.x);
        velocity.x *= -dampingCoeff;
    }

    if (localPos.z < padding)
    {
        localPos.z = -localPos.z + 2 * padding;
        velocity.z *= -dampingCoeff;
    }

    if (localPos.z > -padding + boundaryLen.z)
    {
        localPos.z = -localPos.z + 2 * (-padding + boundaryLen.z);
        velocity.z *= -dampingCoeff;
    }

#ifdef DIFFUSE
    diffuse[index].velocity = velocity;
    diffuse[index].position = localPos + worldPos;
#else
    particles[index].velocity = velocity;
    particles[index].position = localPos + worldPos;
#endif
}


static const float period = 2.f;
static const float delay = 2.f;

void ForceBoundary(in uint index) {
    float padding = 10 * marchingWidth;
    float boundaryRepulsion = 100 * 100000;
    float movingPadding = boundaryLen.x * 0.25 * 0.5 * (-cos(2 * PI *
                                                             max(state[0].time.x - delay, 0) / period ) + 1);

#ifdef DIFFUSE
  float3 force = diffuse[index].force;
  float3 localPos = diffuse[index].position - worldPos;
#else
  float3 force = particles[index].force;
  float3 localPos = particles[index].position - worldPos;
#endif

    if (localPos.y < padding)
    {
        force.y += boundaryRepulsion * (padding - localPos.y);
    }

    if (localPos.y > -padding + boundaryLen.y)
    {
        force.y -= boundaryRepulsion * (localPos.y + padding - boundaryLen.y);
    }

    if (localPos.x < padding + movingPadding)
    {
        force.x += boundaryRepulsion * (movingPadding + padding - localPos.x);
    }

    if (localPos.x > -padding + boundaryLen.x)
    {
        force.x -= boundaryRepulsion * (localPos.x + padding - boundaryLen.x);
    }

    if (localPos.z < padding)
    {
        force.z += boundaryRepulsion * (padding - localPos.z);
    }

    if (localPos.z > -padding + boundaryLen.z)
    {
        force.z -= boundaryRepulsion * (localPos.z + padding - boundaryLen.z);
    }

#ifdef DIFFUSE
  diffuse[index].force += force;
#else
  particles[index].force += force;
#endif
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
#ifdef DIFFUSE
  if (DTid.x >= diffuseParticlesNum) return;
#else
  if (DTid.x >= particlesNum) return;
  if (DTid.x == 0) {
    state[0].time.x += dt.x;
  }
#endif

    ForceBoundary(DTid.x);

#ifdef DIFFUSE
    diffuse[DTid.x].lifetime -= dt.x;
    if (diffuse[DTid.x].lifetime <= 0) {
      InterlockedAdd(state[0].curDiffuseNum, -1);
    }
    diffuse[DTid.x].velocity += dt.x * diffuse[DTid.x].force / diffuse[DTid.x].density;
    diffuse[DTid.x].position += dt.x * diffuse[DTid.x].velocity;
#else
    particles[DTid.x].velocity += dt.x * particles[DTid.x].force / particles[DTid.x].density;
    particles[DTid.x].position += dt.x * particles[DTid.x].velocity;
#endif
}
