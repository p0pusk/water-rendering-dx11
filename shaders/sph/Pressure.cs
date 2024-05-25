#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u1);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
#ifdef DIFFUSE
  if (DTid.x >= diffuseParticlesNum) return;
#else
  if (DTid.x >= particlesNum) return;
#endif

    float k = 1;
    float p0 = 1000;
#ifdef DIFFUSE
    diffuse[DTid.x].pressure = k * (diffuse[DTid.x].density - p0);
#else
    particles[DTid.x].pressure = k * (particles[DTid.x].density - p0);
#endif
}
