#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);


void CreateTable(uint start, uint end)
{
    if (end > particlesNum)
    {
        end = particlesNum;
    }

    uint currHash, original;
    for (uint i = start; i < end; i++)
    {
        currHash = particles[i].hash;
        original;
        InterlockedMin(grid[currHash], i, original);
        particles[i].density = 0;
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) particlesNum / GROUPS_NUM / BLOCK_SIZE);
    CreateTable(globalThreadId.x * partition, (globalThreadId.x + 1) * partition);
}


