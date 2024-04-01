#include "../shaders/Sph.h"

RWStructuredBuffer<int> voxel_grid : register(u0);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    // clear voxel grid
    uint start = globalThreadId.x;
    uint end = globalThreadId.x + 1;
    for (int i = start; i < end; ++i)
    {
        voxel_grid[i] = false;
    }
}
