#include "../shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<int> voxel_grid : register(u1);
RWStructuredBuffer<float3> vertex : register(u2);

bool check_collision(in float3 p)
{
    float min_dist = marchingWidth + 0.001f;
    for (int i = 0; i < particlesNum; i++)
    {
        float d = distance(p, particles[i].position);
        if (d <= min_dist)
        {
            return true;
        }
    }
    return false;
}

void update_grid(uint start, uint end)
{
    if (end > particlesNum)
    {
        end = particlesNum;
    }
    float3 len = cubeNum * cubeLen;
    uint3 num = ceil(len / marchingWidth);

    for (int i = start; i < end; i++)
    {
        int radius = h / marchingWidth;
        int3 localPos = (particles[i].position - worldPos) / marchingWidth;
        for (int i = localPos.x - radius; i < num.x && i >= 0 && i <= localPos.x + radius; i++)
        {
            for (int j = localPos.y - radius; j < num.y && j >= 0 && j <= localPos.y + radius; j++)
            {
                for (int k = localPos.z - radius; k < num.z && k >= 0 && k <= localPos.z + radius; k++)
                {
                    int index = i + (k * num.y + j) * num.x;
                    voxel_grid[index] = check_collision(float3(i, j, k) * marchingWidth + worldPos);
                }
            }
        }
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint size = vertex.IncrementCounter();

    uint partition = ceil(size / GROUPS_NUM / BLOCK_SIZE);
    uint start = globalThreadId.x * partition;
    uint end = (globalThreadId.x + 1) * partition;
    if (end < size)
    {
        end = size;
    }

    // clear vertex buffer
    for (uint i = start; i < end; ++i)
    {
        vertex[i] = 0;
    }
    
    // clear voxel grid
    float3 len = cubeNum * cubeLen;
    uint3 num = ceil(len / marchingWidth);
    int grid_size = num.x * num.y * num.z;
    partition = ceil((float) grid_size / BLOCK_SIZE / GROUPS_NUM);
    start = globalThreadId.x * partition;
    end = (globalThreadId.x + 1) * partition;
    for (int i = start; i < end; ++i)
    {
        voxel_grid[i] = false;
    }

    // update grid
    partition = ceil((float) particlesNum / BLOCK_SIZE / GROUPS_NUM);
    start = globalThreadId.x * partition;
    end = (globalThreadId.x + 1) * partition;
    update_grid(start, end);
}
