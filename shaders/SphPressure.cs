#include "../shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) particlesNum / GROUPS_NUM / BLOCK_SIZE);
    uint startIndex = globalThreadId.x * partition;
    uint endIndex = (globalThreadId.x + 1) * partition;
    if (endIndex > particlesNum)
    {
        endIndex = particlesNum;
    }

    for (uint i = startIndex; i < endIndex; i++)
    {
        float k = 1;
        float p0 = 1000;
        particles[i].pressure = k * (particles[i].density - p0);
    }
}
