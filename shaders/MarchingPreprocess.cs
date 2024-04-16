#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<int> voxel_grid : register(u0);

bool check_collision(in float3 p, in float3 v)
{
    float min_dist = h;
    float d = distance(p, v);
    if (d < min_dist)
    {
        return true;
    }
    return false;
}

void update_grid(uint start, uint end)
{
    if (end > particlesNum)
    {
        end = particlesNum;
    }
    float3 len = (cubeNum + uint3(1, 1, 1)) * cubeLen;
    uint3 num = ceil(len / marchingWidth);

    for (int p = start; p < end; ++p)
    {
      int3 localPos = round((particles[p].position - worldPos) / marchingWidth);
        int radius = ceil(h / 2 / marchingWidth);
        for (int i = -radius; i <= radius; ++i)
        {
            for (int j = -radius; j <= radius; ++j)
            {
                for (int k = -radius; k <= radius; ++k)
                {
                    if (i + localPos.x < 0 || j + localPos.y < 0 || k + localPos.z < 0 ||
                        i + localPos.x >= num.x || j + localPos.y >= num.y ||
                        k + localPos.z >= num.z) {
                      continue;
                    }
                    int index = i + localPos.x + ((k + localPos.z) * num.y + j + localPos.y) * num.x;

                    if (!voxel_grid[index])
                    {
                      voxel_grid[index] = check_collision(float3(localPos.x + i, localPos.y + j, localPos.z + k) * marchingWidth + worldPos, particles[p].position);
                    }
                }
            }
        }
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    // update grid
    uint partition = ceil((float) particlesNum / BLOCK_SIZE / GROUPS_NUM);
    uint start = globalThreadId.x * partition;
    uint end = (globalThreadId.x + 1) * partition;
    update_grid(start, end);
}
