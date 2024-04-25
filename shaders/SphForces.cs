#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);

uint GetHash(in uint3 cell)
{
    return ((uint) (cell.x * 73856093) ^ (uint) (cell.y * 19349663) ^ (uint) (cell.z * 83492791)) % g_tableSize;
}

uint3 GetCell(in float3 position)
{
    return floor((position - worldPos) / h);
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
    if (GTid.x >= particlesNum)
    {
      return;
    }

  // Compute pressure force
    float3 pressureGrad = float3(0, 0, 0);
    float3 force = float3(0.f, -9.8f * particles[GTid.x].density, 0);
    float3 viscosity = float3(0, 0, 0);
    int i, j, k;
    float3 localPos, dir;
    uint key, index;
    float d;
    for (i = -1; i <= 1; i++)
    {
        for (j = -1; j <= 1; j++)
        {
            for (k = -1; k <= 1; k++)
            {
                localPos = particles[GTid.x].position + float3(i, j, k) * h;
                key = GetHash(GetCell(localPos));
                index = grid[key];
                if (index != NO_PARTICLE)
                {
                    while (key == particles[index].hash && index < particlesNum)
                    {
                        d = distance(particles[GTid.x].position, particles[index].position);
                        dir = normalize(particles[GTid.x].position - particles[index].position);
                        if (d == 0.f)
                        {
                            dir = float3(0, 0, 0);
                        }

                        if (d < h)
                        {
                            pressureGrad += -dir * mass * (particles[GTid.x].pressure + particles[index].pressure) / (2 * particles[index].density) * spikyGrad * pow(h - d, 2);

                            viscosity += dynamicViscosity * mass * (particles[index].velocity - particles[GTid.x].velocity) / particles[index].density * spikyLap * (h - d);
                        }
                        ++index;
                    }
                }
            }
        }
    }

    particles[GTid.x].force = force + pressureGrad + viscosity;
}
