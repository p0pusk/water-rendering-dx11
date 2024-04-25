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
  float3 dt;
};

struct Particle {
  float3 position;
  float density;
  float pressure;
  float3 force;
  float3 velocity;
  uint hash;
};

static const uint BLOCK_SIZE = 1024;
static const uint GROUPS_NUM = ceil(particlesNum / (float)BLOCK_SIZE);

static const uint NO_PARTICLE = 0xFFFFFFFF;

static const float PI = 3.14159265f;
static const float poly6 = 315.0f / (64.0f * PI * pow(h, 9));
static const float spikyGrad = -45.0f / (PI * pow(h, 6));
static const float spikyLap = 45.0f / (PI * pow(h, 6));
static const float h2 = pow(h, 2);
