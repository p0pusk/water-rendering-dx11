#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);
StructuredBuffer<uint> entries : register(t2);

RWStructuredBuffer<float> voxel_grid : register(u0);

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
    for (int p = start; p < end; ++p)
    {
      int3 localPos = round((particles[p].position - worldPos) / marchingWidth);
        int radius = marchingWidth / h / 2;
        for (int i = -radius; i <= radius; ++i)
        {
            for (int j = -radius; j <= radius; ++j)
            {
                for (int k = -radius; k <= radius; ++k)
                {
                    if (i + localPos.x < 1 || j + localPos.y < 1 || k + localPos.z < 1 ||
                        i + localPos.x > MC_DIMENSIONS.x - 1 || j + localPos.y > MC_DIMENSIONS.y - 1 ||
                        k + localPos.z > MC_DIMENSIONS.z - 1) {
                      continue;
                    }
                    int index = i + localPos.x + ((k + localPos.z) *
            MC_DIMENSIONS.y + j + localPos.y) * MC_DIMENSIONS.x;

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
    voxel_grid[index] = 0.f;
    uint3 cell = GetMCCell(index);
    if (cell.x > MC_DIMENSIONS.x || cell.y > MC_DIMENSIONS.y || cell.z > MC_DIMENSIONS.z)
    {
      return;
    }

    float3 globalPos = ((float3)cell - float3(0.5f, 0.5f, 0.5f)) * marchingWidth + worldPos;
    uint key, idx, startIdx, entriesNum;
    float density = 0;
    float d = 0;
    int i, j, k;

    for (i = -1; i <= 1; ++i) {
      for (j = -1; j <= 1; ++j) {
        for (k = -1; k <= 1; ++k) {
          key = GetHash(GetCell(globalPos + float3(i, j, k) * h));
          startIdx = hash[key];
          entriesNum = hash[key + 1] - startIdx;
          for (uint c = 0; c < entriesNum; ++c) {
            idx = entries[startIdx + c];
            d = distance(particles[idx].position, globalPos);
            if (d < marchingWidth) {
              density += mass * poly6 * pow(h2 - d*d, 3);
            }
          }
        }
      }
    }

    voxel_grid[index] = 1000 / density;
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
  update2(globalThreadId.x);
}
