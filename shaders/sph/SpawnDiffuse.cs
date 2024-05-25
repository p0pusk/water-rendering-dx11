#include "shaders/Sph.h"

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u0);
RWStructuredBuffer<State> state : register(u1);

static const float velocityThreshold = 1.f;
static const float defaultLifetime = 1.f;

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  float velocityMagnitude = length(particles[DTid.x].velocity);
  if (velocityMagnitude > velocityThreshold) {
    uint original;
    InterlockedAdd(state[0].curDiffuseNum, 1, original); 
    if (original < diffuseParticlesNum) {
      diffuse[original].position = particles[DTid.x].position;
      diffuse[original].density = particles[DTid.x].density;
      diffuse[original].pressure = particles[DTid.x].pressure;
      diffuse[original].force = particles[DTid.x].force;
      diffuse[original].velocity = particles[DTid.x].velocity;
      diffuse[original].lifetime = defaultLifetime;
    } else {
      InterlockedAdd(state[0].curDiffuseNum, -1);
      if (state[0].curDiffuseNum < 0) {
        state[0].curDiffuseNum = 0;
      }
    }
  }
}
