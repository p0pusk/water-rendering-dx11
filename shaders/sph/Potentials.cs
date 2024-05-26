#include "shaders/Sph.h"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
StructuredBuffer<Particle> particles : register(t2);
RWStructuredBuffer<Potential> potentials : register(u0);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;
  int i, j, k;
  float3 localPos, dir;
  float d;
  uint key, startIdx, entriesNum, index, c;
  float velocityDiff = 0.f;
  float curvature = 0.f;

  float3 position = particles[DTid.x].position;
  float3 normal = particles[DTid.x].normal;
  float3 velocity = particles[DTid.x].velocity;

  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (c = 0; c < entriesNum; ++c) {
          index = entries[startIdx + c];
          d = distance(position, particles[index].position);
          dir = normalize(position - particles[index].position);
          if (d == 0.f) {
            dir = float3(0, 0, 0);
          }

          if (d < h && index != DTid.x) {
            float3 v_ij = velocity - particles[index].velocity;
            float lengthV = length(v_ij);
            float lenghtX = length(position - particles[index].position);
            if (lengthV != 0 && lenghtX != 0) {
              velocityDiff += lengthV * (1 - dot(v_ij / lengthV, (position - particles[index].position) / lenghtX)) * W(d, h);
            }

            if (dot(particles[index].position - position, normal) < 0) {
              curvature += (1 - dot(normal, particles[index].normal)) * W(d, h);
            }
          }
        }
      }
    }
  }

  float energy = 0.5f * mass * pow(length(velocity), 2);
  potentials[DTid.x].energy = (min(energy, energyThreshold.y) - min(energy, energyThreshold.x)) / (energyThreshold.y - energyThreshold.x);
  potentials[DTid.x].trappedAir = (min(velocityDiff, trappedAirThreshold.y) - min(velocityDiff, trappedAirThreshold.x)) / (trappedAirThreshold.y - trappedAirThreshold.x);
  if (dot(velocity, normal) >= 0.6f) {
    potentials[DTid.x].waveCrest = (min(curvature, wavecrestThreshold.y) - min(curvature, wavecrestThreshold.x)) / (wavecrestThreshold.y - wavecrestThreshold.x);
  }
}
