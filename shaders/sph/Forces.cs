#include "../Sph.hlsli"

StructuredBuffer<uint> hash : register(t0);
RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<Potential> potentials : register(u1);


static const float surfaceTensionCoefficient = 0.00728f;


[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID)
{
  if (DTid.x >= particlesNum) return;

  // Compute pressure force
  float3 pressureGrad = float3(0, 0, 0);
  float3 force = float3(0.f, -9.8f * particles[DTid.x].density, 0);
  float3 viscosity = float3(0, 0, 0);
  float3 colorFieldGrad = float3(0, 0, 0);
  float colorFieldLap = 0;
  float3 surfaceTension = float3(0, 0, 0);
  int i, j, k;
  float3 localPos, dir;
  float d;
  uint key, startIdx, entriesNum, c;

  float3 position = particles[DTid.x].position;
  float3 pressure = particles[DTid.x].pressure;
  float3 velocity = particles[DTid.x].velocity;

  for (i = -1; i <= 1; ++i) {
    for (j = -1; j <= 1; ++j) {
      for (k = -1; k <= 1; ++k) {
        key = GetHash(GetCell(position + float3(i, j, k) * h));
        startIdx = hash[key];
        entriesNum = hash[key + 1] - startIdx;
        for (c = 0; c < entriesNum; ++c) {
          d = distance(position, particles[startIdx + c].position);
          dir = normalize(position - particles[startIdx + c].position);
          if (d == 0.f) {
            dir = float3(0, 0, 0);
          }

          if (d < h && startIdx + c != DTid.x) {
            pressureGrad += -dir * mass * (pressure + particles[startIdx +
              c].pressure) / (2 * particles[startIdx + c].density) * GradW(d, h);
            viscosity += dynamicViscosity * mass * (particles[startIdx +
              c].velocity - velocity) * particles[startIdx + c].density * W(d, h);
            colorFieldGrad += mass / particles[startIdx + c].density * GradW(d, h);
            colorFieldLap += mass / particles[startIdx + c].density * LapW(d, h);
          }
        }
      }
    }
  }

  particles[DTid.x].normal = normalize(colorFieldGrad);

  if (length(colorFieldGrad) > 1e-5f) {
     surfaceTension += surfaceTensionCoefficient * colorFieldLap * normalize(colorFieldGrad);
     potentials[DTid.x].curvature = -colorFieldLap / length(colorFieldGrad);
  }


  particles[DTid.x].force = force + pressureGrad + viscosity + surfaceTension;
}
