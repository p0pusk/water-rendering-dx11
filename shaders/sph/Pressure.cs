#include "../Sph.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

    float k = 1;
    float p0 = 1000;
    particles[DTid.x].pressure = k * (particles[DTid.x].density - p0);
}
