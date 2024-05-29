#include "shaders/Sph.h"

RWStructuredBuffer<DiffuseParticle> diffuse : register(u0);
RWStructuredBuffer<State> state : register(u1);

static const float period = 4.f;
static const float delay = 4.f;

void deleteDiffuse(in uint index) {
  uint idx;
  if (state[0].curDiffuseNum >= 1) {
    InterlockedAdd(state[0].curDiffuseNum, -1, idx);
    if (idx - 1 < 0) return;
    diffuse[index] = diffuse[idx - 1];
  }
}

void SplashBoundary(in uint index) {
    float3 padding = marchingWidth;

    float3 velocity = diffuse[index].velocity;
    float3 localPos = diffuse[index].position;
    float movingPadding = boundaryLen.x * 0.33 * 0.5 * (-cos(2 * PI * max(state[0].time - delay, 0) / period ) + 1);

    if (localPos.y < padding.y || localPos.y > -padding.y + boundaryLen.y || 
        localPos.x < padding.x + movingPadding || localPos.x > -padding.x + boundaryLen.x ||
        localPos.z < padding.z || localPos.z > -padding.z + boundaryLen.z) {

    deleteDiffuse(index);
  }
}


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= state[0].curDiffuseNum) return;

  if (diffuse[DTid.x].type == 0) {
    SplashBoundary(DTid.x);
    return;
  }

  if (diffuse[DTid.x].lifetime <= 0) {
    deleteDiffuse(DTid.x);
  }
}
