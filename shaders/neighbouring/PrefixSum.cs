#include "../Sph.hlsli"

RWStructuredBuffer<uint> grid : register(u0);

[numthreads(1, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
  for (uint i = 1; i < g_tableSize + 1; ++i) {
    grid[i] += grid[i - 1];
  }
}

