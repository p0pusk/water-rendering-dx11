
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
    float4 dt;
};

struct Particle
{
    float3 position;
    float density;
    float3 pressureGrad;
    float pressure;
    float4 viscosity;
    float4 force;
    float3 velocity;
    uint hash;
};

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> grid : register(u1);

static const float PI = 3.14159265f;
static const float poly6 = 315.0f / (64.0f * PI * pow(h, 9));
static const float spikyGrad = -45.0f / (PI * pow(h, 6));
static const float spikyLap = 45.0f / (PI * pow(h, 6));
static const float h2 = pow(h, 2);

static const uint NO_PARTICLE = 0xFFFFFFFF;
static const uint TABLE_SIZE = 262144;

uint GetHash(in uint3 cell)
{
    return ((uint) (cell.x * 73856093) ^ (uint) (cell.y * 19349663) ^ (uint) (cell.z * 83492791)) % TABLE_SIZE;
}

uint3 GetCell(in float3 position)
{
    return floor((position + worldPos) / h);
}

void CreateTable()
{
    for (uint i = 0; i < TABLE_SIZE; i++)
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

    if (particles[index].position.x > -h + worldPos.x + len.x)
    {
        particles[index].position.x = -particles[index].position.x + 2 * (-h + worldPos.x + len.x);
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
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                for (int k = -1; k <= 1; k++)
                {
                    float3 localPos = particles[p].position + float3(i, j, k) * h;
                    uint hash = GetHash(GetCell(localPos));
                    uint index = grid[hash];
                    if (index != NO_PARTICLE)
                    {
                        while (hash == particles[index].hash && index < endIndex)
                        {
                            float d = distance(particles[index].position, particles[p].position);
                            if (d < h)
                            {
                                particles[p].density += mass * poly6 * pow(h2 - d * d, 3);
                            }
                            ++index;
                        }
                    }
                }
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
        particles[p].force = float4(0, -9.8f * particles[p].density, 0, 0);
        particles[p].viscosity = float4(0, 0, 0, 0);
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                for (int k = -1; k <= 1; k++)
                {
                    float3 localPos = particles[p].position + float3(i, j, k) * h;
                    uint key = GetHash(GetCell(localPos));
                    uint index = grid[key];
                    if (index != NO_PARTICLE)
                    {
                        while (key == particles[index].hash && index < endIndex)
                        {
                            float d = distance(particles[p].position, particles[index].position);
                            float3 dir = normalize(particles[p].position - particles[index].position);
                            if (d == 0.f)
                            {
                                dir = float3(0, 0, 0);
                            }

                            if (d < h)
                            {
                                particles[p].pressureGrad += dir * mass * (particles[p].pressure + particles[index].pressure) / (2 * particles[index].density) * spikyGrad * pow(h - d, 2);

                                particles[p].viscosity += dynamicViscosity * mass * float4(particles[index].velocity - particles[p].velocity, 0) / particles[index].density * spikyLap * (h - d);
                            }
                            ++index;
                        }
                    }
                }
            }
        }
    }

    AllMemoryBarrierWithGroupSync();

  // TimeStep
    for (uint p = startIndex; p < endIndex; p++)
    {
        particles[p].velocity += dt.x * (particles[p].pressureGrad + particles[p].force.xyz + particles[p].viscosity.xyz) / particles[p].density;
        particles[p].position += dt.x * particles[p].velocity;

    // boundary condition
        CheckBoundary(p);
    }
}


[numthreads(64, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    int partitionSize = round(particlesNum / 64.0f + 0.5f);
    if (globalThreadId.x == 0 && globalThreadId.y == 0 && globalThreadId.z == 0)
    {
        CreateTable();
    }
    update(globalThreadId.x * partitionSize, (globalThreadId.x + 1) * partitionSize);
}


