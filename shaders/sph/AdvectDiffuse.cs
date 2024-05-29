#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<uint> entries : register(t1);
StructuredBuffer<Particle> particles : register(t2);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u0);
RWStructuredBuffer<State> state : register(u1);

static const float period = 4.f;
static const float delay = 4.f;

void deleteDiffuse(in uint index) {
  uint idx;
  if (state[0].curDiffuseNum >= 1) {
    InterlockedAdd(state[0].curDiffuseNum, -1, idx);
    if (idx - 1 <= 0) return;
    diffuse[index] = diffuse[idx - 1];
  }
}

void SplashBoundary(in uint index) {
    float3 padding = 10 * marchingWidth;

    float3 velocity = diffuse[index].velocity;
    float3 localPos = diffuse[index].position;

    if (localPos.y < padding.y || localPos.y > -padding.y + boundaryLen.y || 
        localPos.x < padding.x || localPos.x > -padding.x + boundaryLen.x ||
        localPos.z < padding.z || localPos.z > -padding.z + boundaryLen.z) {

    deleteDiffuse(index);
  }
}


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= state[0].curDiffuseNum) return;

  uint type;
  uint neighbours;
  uint key, index, startIdx, entriesNum;
  float d;
  float3 position = diffuse[DTid.x].position + marchingWidth;
  for (int i = -1; i <= 1; ++i) {
    for (int j = -1; j <= 1; ++j) {
      for (int k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        entriesNum = hash[key + 1] - hash[key];
        neighbours += entriesNum;
      }
    }
  }

  if (neighbours < 6) {
    type = 0;
    diffuse[DTid.x].lifetime = 0.f;
  } else if (neighbours < 20) {
    type = 1;
    diffuse[DTid.x].lifetime = 0.f;
  } else {
    type = 2;
  }
  

  diffuse[DTid.x].type = type;
  diffuse[DTid.x].neighbours = neighbours;
  uint origin = diffuse[DTid.x].origin;

  float3 vf = float3(0, 0 ,0);
  float3 vfTop = float3(0, 0 ,0);
  float3 vfBot = float3(0, 0 ,0);

  if (type == 0) {
    diffuse[DTid.x].velocity += float3(0, -9.8f, 0) * dt;
    diffuse[DTid.x].position += dt * diffuse[DTid.x].velocity;
  } else {
    for (int i = -1; i <= 1; ++i) {
      for (int j = -1; j <= 1; ++j) {
        for (int k = -1; k <= 1; ++k) {
          key = GetHash(GetCell(position + float3(i, j, k) * h));
          startIdx = hash[key];
          entriesNum = hash[key + 1] - startIdx;
          for (uint c = 0; c < entriesNum; ++c) {
            index = entries[startIdx + c];
            d = distance(particles[index].position, position);
            vfTop += particles[index].velocity * W(d, h);
            vfBot += W(d, h);
          }
        }
      }
    }

    if (vfBot.x != 0 && vfBot.y != 0 && vfBot.z != 0) {
      vf = vfTop / vfBot;
    }

    if (type == 1) {
      diffuse[DTid.x].position += dt * vf;
      diffuse[DTid.x].lifetime -= dt;
    } else if (type == 2) {
      float kb = 0.4f;
      float kd = 0.7f;
      diffuse[DTid.x].velocity += dt * (-kb * float3(0, -9.8f, 0)  + kd * (vf - diffuse[DTid.x].velocity ) / dt);
      diffuse[DTid.x].position += dt * diffuse[DTid.x].velocity;
    }
  }
}
