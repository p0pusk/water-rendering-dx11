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
    particles[GTid.x].pressureGrad = float3(0, 0, 0);
    particles[GTid.x].force = float4(0, -9.8f * particles[GTid.x].density, 0, 0);
    particles[GTid.x].viscosity = float4(0, 0, 0, 0);
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            for (int k = -1; k <= 1; k++)
            {
                float3 localPos = particles[GTid.x].position + float3(i, j, k) * h;
                uint key = GetHash(GetCell(localPos));
                uint index = grid[key];
                if (index != NO_PARTICLE)
                {
                    while (key == particles[index].hash && index < particlesNum)
                    {
                        float d = distance(particles[GTid.x].position, particles[index].position);
                        float3 dir = normalize(particles[GTid.x].position - particles[index].position);
                        if (d == 0.f)
                        {
                            dir = float3(0, 0, 0);
                        }

                        if (d < h)
                        {
                            particles[GTid.x].pressureGrad += dir * mass * (particles[GTid.x].pressure + particles[index].pressure) / (2 * particles[index].density) * spikyGrad * pow(h - d, 2);

                            particles[GTid.x].viscosity += dynamicViscosity * mass * float4(particles[index].velocity - particles[GTid.x].velocity, 0) / particles[index].density * spikyLap * (h - d);
                        }
                        ++index;
                    }
                }
            }
        }
    }
}
