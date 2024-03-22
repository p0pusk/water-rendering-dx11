#include "../shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);

uint GetHash(in uint3 cell)
{
    return ((uint) (cell.x * 73856093) ^ (uint) (cell.y * 19349663) ^ (uint) (cell.z * 83492791)) % TABLE_SIZE;
}

uint3 GetCell(in float3 position)
{
    return floor((position + worldPos) / h);
}

void CreateTable(uint start, uint end)
{
    if (end > particlesNum)
    {
        end = particlesNum;
    }

    for (uint i = start; i < end; i++)
    {
        uint currHash = particles[i].hash;
        uint original;
        InterlockedMin(grid[currHash], i, original);
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) particlesNum / GROUPS_NUM / BLOCK_SIZE);
    CreateTable(globalThreadId.x * partition, (globalThreadId.x + 1) * partition);
}


