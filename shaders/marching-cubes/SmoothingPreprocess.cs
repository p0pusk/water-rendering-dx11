#include "../Sph.hlsli"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<uint> hash : register(t1);

RWStructuredBuffer<float> voxel_grid : register(u0);
RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u1);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
    uint3 cell = GetMCCell(DTid.x);
    if (cell.x >= MC_DIMENSIONS.x || cell.y >= MC_DIMENSIONS.y || cell.z >= MC_DIMENSIONS.z)
    {
      return;
    }

    voxel_grid[DTid.x] = 0.f;

    float3 globalPos = ((float3)cell + float3(0.5f, 0.5f, 0.5f)) * marchingWidth + worldPos;
    uint key, idx, startIdx, entriesNum;
    uint sum = 0;
    float d = 0;
    int i, j, k;

    for (i = -1; i <= 1; ++i) {
      for (j = -1; j <= 1; ++j) {
        for (k = -1; k <= 1; ++k) {
          key = GetHash(GetCell(globalPos + float3(i, j, k) * marchingWidth / 2));
          startIdx = hash[key];
          entriesNum = hash[key + 1] - startIdx;
          for (uint c = 0; c < entriesNum; ++c) {
            d = distance(particles[startIdx + c].position, globalPos);
            if (d <= length(marchingWidth / 2)) {
              sum++;
            }
          }
        }
      }
    }

    float m = (float)surfaceBuffer[0].sum / (float)surfaceBuffer[0].usedCells;
    voxel_grid[DTid.x] = min(sum, m) / m;
}
