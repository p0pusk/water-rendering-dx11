cbuffer SphCB : register(b0) {
  float3 worldPos;
  uint particlesNum;
  float3 boundaryLen;
  float h;
  float mass;
  float dynamicViscosity;
  float dampingCoeff;
  float marchingWidth;
  uint g_tableSize;
  uint diffuseParticlesNum;
  float2 dt;
};

struct Particle {
  float3 position;
  float density;
  float pressure;
  float3 force;
  float3 velocity;
  uint hash;
};

struct DiffuseParticle {
  float3 position;
  float density;
  float pressure;
  float3 force;
  float3 velocity;
  float lifetime;
};

struct SurfaceBuffer {
  uint sum;
  uint usedCells;
};

struct State {
  float3 time;
  uint curDiffuseNum;
};

static const uint BLOCK_SIZE = 1024;
static const uint GROUPS_NUM = ceil(particlesNum / (float)BLOCK_SIZE);

static const float PI = 3.14159265f;
static const float poly6 = 315.0f / (64.0f * PI * pow(h, 9));
static const float spikyGrad = -45.0f / (PI * pow(h, 6));
static const float spikyLap = 45.0f / (PI * pow(h, 6));
static const float h2 = pow(h, 2);

static const uint3 MC_DIMENSIONS = uint3(256, 256, 256) + uint3(1, 1, 1);

uint3 GetMCCell(uint index) {
  return uint3(index % MC_DIMENSIONS.x,
               (index / MC_DIMENSIONS.x) % MC_DIMENSIONS.y,
               (index / MC_DIMENSIONS.x) / MC_DIMENSIONS.y);
}

uint GetHash(in uint3 cell) {
  return ((uint)(cell.x * 92837111) ^ (uint)(cell.y * 689287499) ^
          (uint)(cell.z * 283923481)) %
         g_tableSize;
}

uint3 GetCell(in float3 position) { return (position - worldPos) / h; }
