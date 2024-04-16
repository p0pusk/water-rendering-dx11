#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);


void CheckBoundary(in uint index)
{
    float3 len = cubeNum * cubeLen;

    if (particles[index].position.y < h + worldPos.y)
    {
        particles[index].position.y = -particles[index].position.y + 2 * (h + worldPos.y);
        particles[index].velocity.y = -particles[index].velocity.y * dampingCoeff;
    }

    if (particles[index].position.x < h + worldPos.x)
    {
        particles[index].position.x = -particles[index].position.x + 2 * (worldPos.x + h);
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (particles[index].position.x > -h + worldPos.x + len.x)
    {
        particles[index].position.x = -particles[index].position.x + 2 * (-h + worldPos.x + len.x);
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (particles[index].position.z < h + worldPos.z)
    {
        particles[index].position.z = -particles[index].position.z + 2 * (h + worldPos.z);
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }

    if (particles[index].position.z > -h + worldPos.z + len.z)
    {
        particles[index].position.z = -particles[index].position.z + 2 * (-h + worldPos.z + len.z);
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }
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
