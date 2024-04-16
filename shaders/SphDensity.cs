#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);


uint GetHash(in uint3 cell) {
  return ((uint)(cell.x * 73856093) ^ (uint)(cell.y * 19349663) ^ (uint)(cell.z * 83492791)) % g_tableSize;
}

uint3 GetCell(in float3 position) {
  return (position - worldPos) / h;
}

void update(in uint pId)
{
    particles[pId].density = 0;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            for (int k = -1; k <= 1; k++)
            {
                uint hash = GetHash(GetCell(particles[pId].position + float3(i, j, k) * h));
                uint index = grid[hash];
                if (index != NO_PARTICLE)
                {
                    while (hash == particles[index].hash && index < particlesNum)
                    {
                        float d = distance(particles[index].position, particles[pId].position);
                        if (d < h)
                        {
                            particles[pId].density += mass * poly6 * pow(h2 - d * d, 3);
                        }
                        ++index;
                    }
                }
            }
        }
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
  if (GTid.x >= particlesNum) return;
  update(GTid.x);
}
