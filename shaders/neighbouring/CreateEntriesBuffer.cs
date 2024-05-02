#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<uint> grid : register(u0);
RWStructuredBuffer<uint> entries : register(u1);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  uint hash = GetHash(GetCell(particles[DTid.x].position));
  uint original;
  InterlockedAdd(grid[hash], -1, original);
  entries[original - 1] = DTid.x;
}


