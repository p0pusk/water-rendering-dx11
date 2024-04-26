#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);
RWStructuredBuffer<int> voxel_grid : register(u0);
RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u1);



[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
    uint index = GTid.x;
    voxel_grid[index] = false;
    uint3 num = ceil((float)boundaryLen / marchingWidth);
    uint3 cell = uint3(index % num.x, (index / num.x) % num.y, (index / num.x) / num.y);
    if (cell.x > num.x || cell.y > num.y || cell.z > num.z)
    {
      return;
    }

    float3 globalPos = (cell + float3(marchingWidth / 2, marchingWidth / 2, marchingWidth / 2))* marchingWidth + worldPos;
    uint key, idx;
    uint sum = 0;
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
                          sum++;
                        }
                        ++idx;
                    }
                }
            }
        }
    }

    float c = 0.25f;
    float m = (float)surfaceBuffer[0].usedCells / (float)surfaceBuffer[0].sum;
    float gamma = min(sum, m) / m;
    voxel_grid[index] = gamma - c < 0 ? true : false;
}
