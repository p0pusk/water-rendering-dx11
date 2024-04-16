#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);


void CheckBoundary(in uint index)
{
    float3 localPos = particles[index].position - worldPos;

    if (localPos.y < h)
    {
        localPos.y = -localPos.y + 2 * h;
        particles[index].velocity.y = -particles[index].velocity.y * dampingCoeff;
    }

    if (localPos.y > -h + boundaryLen.y)
    {
        localPos.y = -localPos.y + 2 * (-h + boundaryLen.y);
        particles[index].velocity.y = -particles[index].velocity.y * dampingCoeff;
    }

    if (localPos.x < h)
    {
        localPos.x = -localPos.x + 2 * h;
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (localPos.x > -h + boundaryLen.x)
    {
        localPos.x = -localPos.x + 2 * (-h + boundaryLen.x);
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (localPos.z < h)
    {
        localPos.z = -localPos.z + 2 * h;
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }

    if (localPos.z > -h + boundaryLen.z)
    {
        localPos.z = -localPos.z + 2 * (-h + boundaryLen.z);
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

    particles[DTid.x].velocity += dt.x * (particles[DTid.x].pressureGrad + particles[DTid.x].force.xyz + particles[DTid.x].viscosity.xyz) / particles[DTid.x].density;
    particles[DTid.x].position += dt.x * particles[DTid.x].velocity;

    // boundary condition
    CheckBoundary(DTid.x);
}
