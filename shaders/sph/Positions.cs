#include "shaders/Sph.h"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
RWStructuredBuffer<Particle> particles : register(u0);

void CheckBoundary(in uint index)
{
    float3 localPos = particles[index].position - worldPos;
    float padding = 4 * h;

    if (localPos.y < padding)
    {
        localPos.y = -localPos.y + 2 * padding;
        particles[index].velocity.y = -particles[index].velocity.y * dampingCoeff;
    }

    if (localPos.y > -padding + boundaryLen.y)
    {
        localPos.y = -localPos.y + 2 * (-padding + boundaryLen.y);
        particles[index].velocity.y = -particles[index].velocity.y * dampingCoeff;
    }

    if (localPos.x < padding)
    {
        localPos.x = -localPos.x + 2 * padding;
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (localPos.x > -padding + boundaryLen.x)
    {
        localPos.x = -localPos.x + 2 * (-padding + boundaryLen.x);
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (localPos.z < padding)
    {
        localPos.z = -localPos.z + 2 * padding;
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }

    if (localPos.z > -padding + boundaryLen.z)
    {
        localPos.z = -localPos.z + 2 * (-padding + boundaryLen.z);
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }

    particles[index].position = localPos + worldPos;
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= particlesNum)
    {
      return;
    }

    particles[DTid.x].velocity += dt.x * particles[DTid.x].force / particles[DTid.x].density;
    particles[DTid.x].position += dt.x * particles[DTid.x].velocity;

    // boundary condition
    CheckBoundary(DTid.x);
}
