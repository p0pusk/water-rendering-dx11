#include "shaders/Sph.h"

RWStructuredBuffer<int> voxel_grid : register(u0);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
    // clear voxel grid
    voxel_grid[GTid.x] = false;
}
