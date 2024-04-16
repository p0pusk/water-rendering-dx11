#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) g_tableSize / BLOCK_SIZE / GROUPS_NUM);
    uint start = globalThreadId.x * partition;
    uint end = (globalThreadId.x + 1) * partition;
    for (int i = start; i < end; i++)
    {
        grid[i] = NO_PARTICLE;
    }
}
