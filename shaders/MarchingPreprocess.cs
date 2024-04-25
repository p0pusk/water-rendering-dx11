#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);
RWStructuredBuffer<int> voxel_grid : register(u0);

bool check_collision(in float3 p, in float3 v)
{
    float min_dist = h / 2.f + 0.3f * h;
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
    uint3 num = ceil(boundaryLen / marchingWidth);

    for (int p = start; p < end; ++p)
    {
      int3 localPos = round((particles[p].position - worldPos) / marchingWidth);
        int radius = h / 2 / marchingWidth;
        for (int i = -radius; i <= radius; ++i)
        {
            for (int j = -radius; j <= radius; ++j)
            {
                for (int k = -radius; k <= radius; ++k)
                {
                    if (i + localPos.x < 1 || j + localPos.y < 1 || k + localPos.z < 1 ||
                        i + localPos.x > num.x - 1 || j + localPos.y > num.y - 1 ||
                        k + localPos.z > num.z - 1) {
                      continue;
                    }
                    int index = i + localPos.x + ((k + localPos.z) * num.y + j + localPos.y) * num.x;

                    if (!voxel_grid[index])
                    {
                      voxel_grid[index] = true;
                    }
                }
            }
        }
    }
}

void update2(in uint index)
{
    voxel_grid[index] = false;
    uint3 num = ceil((float)boundaryLen / marchingWidth);
    uint3 cell = uint3(index % num.x, (index / num.x) % num.y, (index / num.x) / num.y);
    if (cell.x > num.x || cell.y > num.y || cell.z > num.z)
    {
      return;
    }

    float3 globalPos = (cell + float3(marchingWidth / 2, marchingWidth / 2, marchingWidth / 2))* marchingWidth + worldPos;
    uint key, idx;
    float density = 0;
    float d = 0;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            for (int k = -1; k <= 1; k++)
            {
                key = GetHash(GetCell(globalPos + float3(i, j, k) * h));
                idx = hash[key];
                if (idx != NO_PARTICLE)
                {
                    while (key == particles[idx].hash && idx < particlesNum)
                    {
                        d = distance(particles[idx].position, globalPos);
                        if (d < marchingWidth)
                        {
                            density += mass * poly6 * pow(h2 - d * d, 3);
                        }
                        ++idx;
                    }
                }
            }
        }
    }

    voxel_grid[index] = density > 0 ? true : false;
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) particlesNum / BLOCK_SIZE / GROUPS_NUM);
    uint start = globalThreadId.x * partition;
    uint end = (globalThreadId.x + 1) * partition;
    // update_grid(start, end);
  update2(globalThreadId.x);
}
