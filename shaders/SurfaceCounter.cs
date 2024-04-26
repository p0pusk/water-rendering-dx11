#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);

RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u0);

void count(in uint index)
{
    uint3 num = ceil((float)boundaryLen / marchingWidth);
    uint3 cell = uint3(index % num.x, (index / num.x) % num.y, (index / num.x) / num.y);
    if (cell.x > num.x || cell.y > num.y || cell.z > num.z)
    {
      return;
    }

    float3 globalPos = (cell + float3(marchingWidth / 2, marchingWidth / 2, marchingWidth / 2))* marchingWidth + worldPos;
    uint key, idx;
    uint count = 0;
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
                          count++;
                        }
                        ++idx;
                    }
                }
            }
        }
    }

    if (count > 0) {
      InterlockedAdd(surfaceBuffer[0].usedCells, 1);
      InterlockedAdd(surfaceBuffer[0].sum, count);
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
  count(globalThreadId.x);
}
