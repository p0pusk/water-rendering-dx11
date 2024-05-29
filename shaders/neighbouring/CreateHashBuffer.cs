#include "../Sph.hlsli"

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<uint> grid : register(u0);


void CreateTable(uint index)
{
    if (index >= particlesNum)
    {
      return;
    }

    uint currHash;
    currHash = GetHash(GetCell(particles[index].position));
    InterlockedAdd(grid[currHash], 1);
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
    CreateTable(GTid.x);
}

