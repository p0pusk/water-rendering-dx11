#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
StructuredBuffer<Particle> particles : register(t1);
RWStructuredBuffer<DiffuseParticle> diffuse : register(u0);
RWStructuredBuffer<State> state : register(u1);

static const float period = 4.f;
static const float delay = 4.f;

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= state[0].curDiffuseNum) return;

  uint type;
  uint neighbours;
  uint key, startIdx, entriesNum;
  float d;
  float3 position = diffuse[DTid.x].position + marchingWidth;
  int i, j, k;

  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
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
    return;
  }

  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (uint c = 0; c < entriesNum; ++c) {
          d = distance(particles[startIdx + c].position, position);
          vfTop += particles[startIdx + c].velocity * W(d, h);
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
