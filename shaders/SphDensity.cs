#include "../shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);


uint GetHash(in uint3 cell)
{
    return ((uint) (cell.x * 73856093) ^ (uint) (cell.y * 19349663) ^ (uint) (cell.z * 83492791)) % TABLE_SIZE;
}

uint3 GetCell(in float3 position)
{
    return floor((position + worldPos) / h);
}

void update(in uint startIndex, in uint endIndex)
{
    if (endIndex >= particlesNum)
    {
        endIndex = particlesNum;
    }

    for (uint p = startIndex; p < endIndex; p++)
    {
        particles[p].density = 0;
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                for (int k = -1; k <= 1; k++)
                {
                    float3 localPos = particles[p].position + float3(i, j, k) * h;
                    uint hash = GetHash(GetCell(localPos));
                    uint index = grid[hash];
                    if (index != NO_PARTICLE)
                    {
                        while (hash == particles[index].hash && index < particlesNum)
                        {
                            float d = distance(particles[index].position, particles[p].position);
                            if (d < h)
                            {
                                particles[p].density += mass * poly6 * pow(h2 - d * d, 3);
                            }
                            ++index;
                        }
                    }
                }
            }
        }
    }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) particlesNum / GROUPS_NUM / BLOCK_SIZE);
    update(globalThreadId.x * partition, (globalThreadId.x + 1) * partition);
}
