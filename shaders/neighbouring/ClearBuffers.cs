#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<uint> grid : register(u0);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
    if (GTid.x >= g_tableSize + 1) return;
    grid[GTid.x] = 0;
}
