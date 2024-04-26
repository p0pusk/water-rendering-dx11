#include "shaders/Sph.h"

RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u0);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
  return;
}
