#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<State> state : register(u1);

void CheckBoundary(in uint index)
{
    float3 padding = 4 * marchingWidth;
    float3 localPos = particles[index].position - worldPos;
    float3 velocity = particles[index].velocity;

    if (localPos.y < padding.y)
    {
        localPos.y = -localPos.y + 2 * padding.y;
        velocity *= -dampingCoeff;
    }

    if (localPos.y > -padding.y + boundaryLen.y)
    {
        localPos.y = -localPos.y + 2 * (-padding.y + boundaryLen.y);
        velocity.y *= -dampingCoeff;
    }

    if (localPos.x < padding.y)
    {
        localPos.x = -localPos.x + 2 * padding.y;
        velocity.x *= -dampingCoeff;
    }

    if (localPos.x > -padding.y + boundaryLen.x)
    {
        localPos.x = -localPos.x + 2 * (-padding.y + boundaryLen.x);
        velocity.x *= -dampingCoeff;
    }

    if (localPos.z < padding.z)
    {
        localPos.z = -localPos.z + 2 * padding.z;
        velocity.z *= -dampingCoeff;
    }

    if (localPos.z > -padding.z + boundaryLen.z)
    {
        localPos.z = -localPos.z + 2 * (-padding.z + boundaryLen.z);
        velocity.z *= -dampingCoeff;
    }

    particles[index].velocity = velocity;
    particles[index].position = localPos + worldPos;
}


static const float period = 4.f;
static const float delay = 4.f;

void ForceBoundary(in uint index) {
    float3 padding = float3(10, 10, 10) * marchingWidth;
    float boundaryRepulsion = 100 * 4000;
    float movingPadding = boundaryLen.x * 0.33 * 0.5 * (-cos(2 * PI * max(state[0].time - delay, 0) / period ) + 1);

    float3 force = particles[index].force;
    float3 localPos = particles[index].position - worldPos;

    if (localPos.y < padding.y)
    {
        force.y += boundaryRepulsion * (padding.y - localPos.y);
    }

    if (localPos.y > -padding.y + boundaryLen.y)
    {
        force.y -= boundaryRepulsion * (localPos.y + padding.y - boundaryLen.y);
    }

    if (localPos.x < padding.y + movingPadding)
    {
        force.x += boundaryRepulsion * (movingPadding + padding.x - localPos.x);
    }

    if (localPos.x > -padding.x + boundaryLen.x)
    {
        force.x -= boundaryRepulsion * (localPos.x + padding.x - boundaryLen.x);
    }

    if (localPos.z < padding.z)
    {
        force.z += boundaryRepulsion * (padding.z - localPos.z);
    }

    if (localPos.z > -padding.z + boundaryLen.z)
    {
        force.z -= boundaryRepulsion * (localPos.z + padding.z - boundaryLen.z);
    }

    particles[index].force = force;
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;
  if (DTid.x == 0) {
    state[0].time += dt;
  }

    ForceBoundary(DTid.x);

    particles[DTid.x].velocity += dt * particles[DTid.x].force / particles[DTid.x].density;
    particles[DTid.x].position += dt * particles[DTid.x].velocity;
}
