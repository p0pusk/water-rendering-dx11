#include "shaders/Sph.h"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<State> state : register(u1);

void CheckBoundary(in uint index)
{
    float padding = 4 * marchingWidth;
    float3 localPos = particles[index].position - worldPos;
    float3 velocity = particles[index].velocity;

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

    particles[index].velocity = velocity;
    particles[index].position = localPos + worldPos;
}


static const float period = 5.f;
static const float delay = 2.f;

void ForceBoundary(in uint index) {
    float padding = 10 * marchingWidth;
    float boundaryRepulsion = 100 * 20000;
    float movingPadding = boundaryLen.x * 0.25 * 0.5 * (-cos(2 * PI *
                                                             max(state[0].time - delay, 0) / period ) + 1);

    float3 force = particles[index].force;
    float3 localPos = particles[index].position - worldPos;

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

    particles[index].force = force;
    particles[index].externalForces = force;
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;
  if (DTid.x == 0) {
    state[0].time += dt;
  }

    ForceBoundary(DTid.x);

    particles[DTid.x].velocity += dt * particles[DTid.x].force / particles[DTid.x].density;
    particles[DTid.x].position += dt * particles[DTid.x].velocity;
}
