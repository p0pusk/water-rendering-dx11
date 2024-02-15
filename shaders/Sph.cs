
cbuffer SphCB : register(b0)
{
    float3 worldPos;
    uint particlesNum;
    uint3 cubeNum;
    float cubeLen;
    float h;
    float mass;
    float dynamicViscosity;
    float dampingCoeff;
};

cbuffer SphDB : register(b1)
{
    float4 dt; // only x
}

struct Particle
{
    float3 position;
    float density;
    float3 pressureGrad;
    float pressure;
    float4 viscosity;
    float4 force;
    float3 velocity;
    float hash;
};

RWStructuredBuffer<Particle> particles : register(u0);

static const float PI = 3.14159265f;
static const float poly6 = 315.0f / (64.0f * PI * pow(h, 9));
static const float spikyGrad = -45.0f / (PI * pow(h, 6));
static const float spikyLap = 45.0f / (PI * pow(h, 6));
static const float h2 = pow(h, 2);

static const uint hash_size = 10000;
static const uint NO_PARTICLE = 0xFFFFFFFF;


float GetHash(in float3 cell)
{
    return ((uint) (cell.x * 73856093) ^ (uint) (cell.y * 19349663) ^ (uint) (cell.z * 83492791)) % hash_size;
}

uint3 GetPos(in float3 position)
{
    return floor((position + worldPos) / h);
}

static uint grid[hash_size];

void CreateTable()
{
    for (uint i = 0; i < hash_size; i++)
    {
        grid[i] = NO_PARTICLE;
    }
    uint prevHash = NO_PARTICLE;
    for (i = 0; i < particlesNum; i++)
    {
        uint currHash = particles[i].hash;
        if (currHash != prevHash)
        {
            grid[currHash] = i;
        }
        prevHash = currHash;
    }
}


void CheckBoundary(in uint index)
{
    float3 len = cubeNum * cubeLen;

    if (particles[index].position.y < h + worldPos.y)
    {
        particles[index].position.y = -particles[index].position.y + 2 * (h + worldPos.y);
        particles[index].velocity.y = -particles[index].velocity.y * dampingCoeff;
    }

    if (particles[index].position.x < h + worldPos.x)
    {
        particles[index].position.x = -particles[index].position.x + 2 * (worldPos.x + h);
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (particles[index].position.x > -h + worldPos.x + 1.5f * len.x)
    {
        particles[index].position.x = -particles[index].position.x + 2 * (-h + worldPos.x + 1.5f * len.x);
        particles[index].velocity.x = -particles[index].velocity.x * dampingCoeff;
    }

    if (particles[index].position.z < h + worldPos.z)
    {
        particles[index].position.z = -particles[index].position.z + 2 * (h + worldPos.z);
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }

    if (particles[index].position.z > -h + worldPos.z + len.z)
    {
        particles[index].position.z = -particles[index].position.z + 2 * (-h + worldPos.z + len.z);
        particles[index].velocity.z = -particles[index].velocity.z * dampingCoeff;
    }
}

void update(in uint startIndex, in uint endIndex)
{
    if (endIndex >= particlesNum)
    {
        endIndex = particlesNum;
    }

    for (uint p = startIndex; p < endIndex; p++)
    {
        particles[p].density = 0;
        for (uint n = 0; n < particlesNum; n++)
        {
            float d = distance(particles[n].position, particles[p].position);
            if (d * d < h2)
            {
                particles[p].density += mass * poly6 * pow(h2 - d * d, 3);
            }
        }
    }

    AllMemoryBarrierWithGroupSync();

  // Compute pressure
    for (uint i = startIndex; i < endIndex; i++)
    {
        float k = 1;
        float p0 = 1000;
        particles[i].pressure = k * (particles[i].density - p0);
    }

    AllMemoryBarrierWithGroupSync();

  // Compute pressure force
    for (uint p = startIndex; p < endIndex; p++)
    {
        particles[p].pressureGrad = float3(0, 0, 0);
        particles[p].force.xyz = float3(0, -9.8 * particles[p].density, 0);
        particles[p].viscosity.xyz = float3(0, 0, 0);
        for (uint n = 0; n < particlesNum; n++)
        {
            float d = distance(particles[p].position, particles[n].position);
            float3 dir = normalize(particles[p].position - particles[n].position);
            if (isnan(dir).x || isnan(dir).y || isnan(dir).z)
            {
                dir = float3(0, 0, 0);
            }

            if (d < h)
            {
                particles[p].pressureGrad += dir * mass * (particles[p].pressure + particles[n].pressure) /
                          (2 * particles[n].density) * spikyGrad * pow(h - d, 2);


                particles[p].viscosity += dynamicViscosity * mass *
                       float4(particles[n].velocity - particles[p].velocity, 0) / particles[n].density * spikyLap *
                       (h - d);
            }
        }
    }

    AllMemoryBarrierWithGroupSync();

  // TimeStep
    for (uint p = startIndex; p < endIndex; p++)
    {
        particles[p].velocity.xyz += dt.x * (particles[p].pressureGrad + particles[p].force.xyz + particles[p].viscosity.xyz) / particles[p].density;
        particles[p].position += dt.x * particles[p].velocity;

    // boundary condition
        CheckBoundary(p);
    }
}


[numthreads(64, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    int partitionSize = round(particlesNum / 64 + 0.5);
    update(globalThreadId.x * partitionSize, (globalThreadId.x + 1) * partitionSize);
}


