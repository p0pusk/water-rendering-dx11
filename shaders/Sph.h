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
  float dt;
  float diffuseEnabled;
  float2 trappedAirThreshold;
  float2 wavecrestThreshold;
  float2 energyThreshold;
  float2 padding;
};

struct Particle {
  float3 position;
  float density;
  float pressure;
  float3 force;
  float3 velocity;
  uint hash;
  float3 normal;
  float3 externalForces;
};

struct Potential {
  float waveCrest;
  float trappedAir;
  float energy;
};

struct DiffuseParticle {
  float3 position;
  float3 velocity;
  // 0 - spray, 1 - foam, 2 - bubbles
  uint type;
  uint origin;
  float lifetime;
};

struct SurfaceBuffer {
  uint sum;
  uint usedCells;
};

struct State {
  float time;
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

float poly6Grad(float r, float h) {
  float c = 645 / (32 * PI * h * h);
  return r * c * pow(h * h - r * r, 2);
}

float poly6Lap(float r, float h) {
  float c = 945.0f / (8.0f * PI * pow(h, 9.0f));
  float r2 = r * r;
  return c * (h2 - r2) * (r2 - 3.f / 4.f * (h2 - r2));
}

float CubicKernel(float distance, float smoothingLength) {
  float q = distance / smoothingLength;
  float sigma = 1.0 / (3.14159265359 *
                       pow(smoothingLength, 3)); // Normalization factor for 3D
  if (q >= 0.0 && q < 1.0) {
    return sigma * (1.0 - (3.0 / 2.0) * q * q + (3.0 / 4.0) * q * q * q);
  } else if (q >= 1.0 && q < 2.0) {
    float factor = 2.0 - q;
    return sigma * (1.0 / 4.0) * factor * factor * factor;
  } else {
    return 0.0;
  }
}

// Wendland C2 kernel function
float W(float r, float h) {
  float q = r / h;
  if (q > 1.0f)
    return 0.0f;
  float alpha_d = 21.0f / (16.0f * 3.14159265359f * pow(h, 3));
  float factor = pow(1.0f - q, 4) * (4.0f * q + 1.0f);
  return alpha_d * factor;
}

// Gradient of the Wendland C2 kernel
float GradW(float r, float h) {
  float q = r / h;
  if (q > 2.0f)
    return 0;
  float alpha_d = 21.0f / (16.0f * 3.14159265359f);
  float factor = -5.f / pow(h, 5) * q * pow(1.f - q / 2, 3);
  return alpha_d * factor;
}

// Laplacian of the Wendland C2 kernel
float LapW(float r, float h) {
  float q = r / h;
  if (q > 1.0f)
    return 0.0f;
  float alpha_d = 21.0f / (16.0f * 3.14159265359f * pow(h, 3));
  float term1 = pow(1.0f - q, 4);
  float term2 = pow(1.0f - q, 3) * (4.0f * q + 1.0f);
  float laplacian = (4.0f / (h * h)) * (term1 - term2);
  return alpha_d * laplacian;
}

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
