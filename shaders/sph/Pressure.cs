#include "shaders/Sph.h"

RWStructuredBuffer<Particle> particles : register(u0);


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 GTid : SV_DispatchThreadID)
{
    if (GTid.x >= particlesNum) return;

    float k = 1;
    float p0 = 1000;
    particles[GTid.x].pressure = k * (particles[GTid.x].density - p0);
}
