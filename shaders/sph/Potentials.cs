#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<Particle> particles : register(t1);
RWStructuredBuffer<Potential> potentials : register(u0);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;
  int i, j, k;
  float3 localPos, dir;
  float d;
  uint key, startIdx, entriesNum, c;
  float velocityDiff = 0.f;
  float curvature = potentials[DTid.x].curvature;

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
          d = distance(position, particles[startIdx + c].position);
          dir = normalize(position - particles[startIdx + c].position);
          if (d == 0.f) {
            dir = float3(0, 0, 0);
          }

          if (d < h && startIdx + c != DTid.x) {
            float3 v_ij = velocity - particles[startIdx + c].velocity;
            float lengthV = length(v_ij);
            if (lengthV != 0 && d != 0) {
              velocityDiff += lengthV * (1 - dot(normalize(v_ij),
                                                 normalize(position -
                                                           particles[startIdx +
                                                           c].position))) * (1 - d / h);
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
