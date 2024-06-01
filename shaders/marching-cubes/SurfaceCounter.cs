#include "../Sph.hlsli"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);

RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u0);

void count(in uint index)
{
  uint3 cell = GetMCCell(index);
    if (cell.x >= MC_DIMENSIONS.x || cell.y >= MC_DIMENSIONS.y || cell.z >= MC_DIMENSIONS.z)
    {
      return;
    }

  float3 globalPos = ((float3)cell + float3(0.5f, 0.5f, 0.5f)) * marchingWidth + worldPos;
  uint key, idx, startIdx, entriesNum;
  uint count = 0;
  float d = 0;
  int i, j, k;

  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(globalPos + float3(i, j, k) * marchingWidth / 2));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          d = distance(globalPos, particles[startIdx + c].position);
          if (d <= length(marchingWidth / 2)) {
            count++;
          }
        }
      }
    }
  }

  if (count > 0) {
    InterlockedAdd(surfaceBuffer[0].usedCells, 1);
    InterlockedAdd(surfaceBuffer[0].sum, count);
  }
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
  count(globalThreadId.x);
}
