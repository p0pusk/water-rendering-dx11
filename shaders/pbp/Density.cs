#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
RWStructuredBuffer<PBParticle> particles : register(u0);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  uint key, startIdx, entriesNum, index;
  float d;
  float density = 0.f;
  float3 position = particles[DTid.x].predictedPosition;

  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      for (int k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          index = entries[startIdx + c];
          d = distance(particles[index].predictedPosition, position);
          if (d < h) {
            density += mass * W(d, h);
          }
        }
      }
    }
  }

  particles[DTid.x].density = density;
}
