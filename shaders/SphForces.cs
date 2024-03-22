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

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    uint partition = ceil((float) particlesNum / GROUPS_NUM / BLOCK_SIZE);

    uint startIndex = globalThreadId.x * partition;
    uint endIndex = (globalThreadId.x + 1) * partition;
    if (endIndex > particlesNum)
    {
        endIndex = particlesNum;
    }

  // Compute pressure force
    for (uint p = startIndex; p < endIndex; p++)
    {
        particles[p].pressureGrad = float3(0, 0, 0);
        particles[p].force = float4(0, -9.8f * particles[p].density, 0, 0);
        particles[p].viscosity = float4(0, 0, 0, 0);
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                for (int k = -1; k <= 1; k++)
                {
                    float3 localPos = particles[p].position + float3(i, j, k) * h;
                    uint key = GetHash(GetCell(localPos));
                    uint index = grid[key];
                    if (index != NO_PARTICLE)
                    {
                        while (key == particles[index].hash && index < particlesNum)
                        {
                            float d = distance(particles[p].position, particles[index].position);
                            float3 dir = normalize(particles[p].position - particles[index].position);
                            if (d == 0.f)
                            {
                                dir = float3(0, 0, 0);
                            }

                            if (d < h)
                            {
                                particles[p].pressureGrad += dir * mass * (particles[p].pressure + particles[index].pressure) / (2 * particles[index].density) * spikyGrad * pow(h - d, 2);

                                particles[p].viscosity += dynamicViscosity * mass * float4(particles[index].velocity - particles[p].velocity, 0) / particles[index].density * spikyLap * (h - d);
                            }
                            ++index;
                        }
                    }
                }
            }
        }
    }
}
