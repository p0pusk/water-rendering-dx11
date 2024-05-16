#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);
StructuredBuffer<uint> entries : register(t2);

RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u0);

void count(in uint index)
{
  uint3 num = ceil((float)boundaryLen / marchingWidth);
  uint3 cell = uint3(index % num.x, (index / num.x) % num.y, (index / num.x) / num.y);
  if (cell.x > num.x || cell.y > num.y || cell.z > num.z)
  {
    return;
  }

  float3 globalPos = (cell - float3(0.5f, 0.5f, 0.5f))* marchingWidth + worldPos;
  uint key, idx, startIdx, entriesNum;
  uint count = 0;
  float d = 0;
  int i, j, k;

  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(globalPos + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          idx = entries[startIdx + c];
          d = distance(globalPos, particles[idx].position);
          if (d < marchingWidth / 2) {
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
