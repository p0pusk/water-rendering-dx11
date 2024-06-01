#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  uint key, startIdx, entriesNum;
  float d;
  float density = 0.f;
  float3 position = particles[DTid.x].position;

  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      for (int k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          d = distance(particles[startIdx + c].position, position);
          density += mass * W(d, h);
        }
      }
    }
  }

  particles[DTid.x].density = density;

  float k = 1;
  float p0 = 1000;
  particles[DTid.x].pressure = k * (particles[DTid.x].density - p0);
}
