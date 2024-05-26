#include "shaders/Sph.h"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
StructuredBuffer<Particle> particles : register(t2);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u0);
RWStructuredBuffer<State> state : register(u1);

static const float period = 5.f;
static const float delay = 2.f;

void ForceBoundary(in uint index) {
    float padding = 10 * marchingWidth;
    float boundaryRepulsion = 100 * 2000000;
    float movingPadding = boundaryLen.x * 0.25 * 0.5 * (-cos(2 * PI *
                                                             max(state[0].time - delay, 0) / period ) + 1);

    float3 velocity = diffuse[index].velocity;
    float3 localPos = diffuse[index].position - worldPos;

    if (localPos.y < padding)
    {
        velocity.y += dt * boundaryRepulsion * (padding - localPos.y);
    }

    if (localPos.y > -padding + boundaryLen.y)
    {
        velocity.y -= dt * boundaryRepulsion * (localPos.y + padding - boundaryLen.y);
    }

    if (localPos.x < padding + movingPadding)
    {
        velocity.x += dt * boundaryRepulsion * (movingPadding + padding - localPos.x);
    }

    if (localPos.x > -padding + boundaryLen.x)
    {
        velocity.x -= dt * boundaryRepulsion * (localPos.x + padding - boundaryLen.x);
    }

    if (localPos.z < padding)
    {
        velocity.z += dt * boundaryRepulsion * (padding - localPos.z);
    }

    if (localPos.z > -padding + boundaryLen.z)
    {
        velocity.z -= dt * boundaryRepulsion * (localPos.z + padding - boundaryLen.z); 
    }

    diffuse[index].velocity = velocity;
}


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= state[0].curDiffuseNum) return;

  uint type;
  uint neighbours;
  uint key, index, startIdx, entriesNum;
  float d;
  float3 position = diffuse[DTid.x].position;
  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      for (int k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          index = entries[startIdx + c];
          d = distance(particles[index].position, position);
          if (d < h) {
            ++neighbours;
          }
        }
      }
    }
  }

  if (neighbours < 4) {
    type = 0;
  } else if (neighbours > 20) {
    type = 1;
  } else {
    type = 2;
  }
  diffuse[DTid.x].type = type;
  uint origin = diffuse[DTid.x].origin;

  float3 vf = float3(0, 0 ,0);
  float3 vfTop = float3(0, 0 ,0);
  float3 vfBot = float3(0, 0 ,0);
  if (type == 0) {
    float singleDensity = 21.0f / (16.0f * 3.14159265359f * pow(h, 3)) * mass;
    ForceBoundary(DTid.x);
    diffuse[DTid.x].velocity += float3(0, -9.8f, 0) * dt;
    diffuse[DTid.x].position += dt * diffuse[DTid.x].velocity;
    return;
  }

  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      for (int k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          index = entries[startIdx + c];
          d = distance(particles[index].position, position);
          if (d < h) {
            vfTop += particles[index].velocity * CubicKernel(d, h);
            vfBot += CubicKernel(d, h);
          }
        }
      }
    }
  }

  vf = vfTop / vfBot;

  if (type == 1) {
    diffuse[DTid.x].position += dt * vf;
    diffuse[DTid.x].lifetime -= dt;
    if (diffuse[DTid.x].lifetime <= 0) {
      if (state[0].curDiffuseNum > 0) {
        InterlockedAdd(state[0].curDiffuseNum, -1, index);
        if (index - 1 <= 0) return;
        diffuse[DTid.x] = diffuse[index - 1];
      }
    }
    return;
  }
  else if (type == 2) {
    float kb = 0.5f;
    float kd = 0.5f;
    diffuse[DTid.x].velocity += dt * (-kb * 9.8f  + kd * (vf - diffuse[DTid.x].velocity ) / dt);
    diffuse[DTid.x].position += dt * diffuse[DTid.x].velocity;

  }
}
