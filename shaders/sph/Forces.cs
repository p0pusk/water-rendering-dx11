#include "shaders/Sph.h"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
RWStructuredBuffer<Particle> particles : register(u0);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  // Compute pressure force
  float3 pressureGrad = float3(0, 0, 0);
  float3 force = float3(0.f, -9.8f * particles[DTid.x].density, 0);
  float3 viscosity = float3(0, 0, 0);
  int i, j, k;
  float3 localPos, dir;
  float d;
  uint key, startIdx, entriesNum, index, c;
  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(particles[DTid.x].position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (c = 0; c < entriesNum; ++c) {
          index = entries[startIdx + c];
          d = distance(particles[DTid.x].position, particles[index].position);
          dir = normalize(particles[DTid.x].position - particles[index].position);
          if (d == 0.f) {
            dir = float3(0, 0, 0);
          }

          if (d < h) {
            pressureGrad += -dir * mass * (particles[DTid.x].pressure + particles[index].pressure) / (2 * particles[index].density) * spikyGrad * pow(h - d, 2);
            viscosity += dynamicViscosity * mass * (particles[index].velocity - particles[DTid.x].velocity) / particles[index].density * spikyLap * (h - d);
          }
        }
      }
    }
  }
  particles[DTid.x].force = force + pressureGrad + viscosity;
}
