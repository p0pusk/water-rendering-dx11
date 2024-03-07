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
RWStructuredBuffer<bool> voxel_grid : register(u1);

void update_grid()
{
    int size = cubeNum.x * cubeNum.y * cubeNum.z;
    for (int i = 0; i < size; i++)
    {
        voxel_grid[i] = false;
    }

    float cubeWidth = h / 2;
    float3 len = cubeNum * cubeLen;
    uint3 num = len / cubeWidth + 1;
    for (int i = 0; i < particlesNum; i++)
    {
        int x = particles[i].position.x / cubeWidth - worldPos.x;
        int y = particles[i].position.y / cubeWidth - worldPos.y;
        int z = particles[i].position.z / cubeWidth - worldPos.z;

        int radius = h / cubeWidth;
        for (int i = x - radius; i < num.x && i >= 0 && i <= x + radius; i++)
        {
            for (int j = y - radius; j < num.y && j >= 0 && j <= y + radius; j++)
            {
                for (int k = z - radius; k < num.z && k >= 0 && k <= z + radius;
             k++)
                {
                    int index = i + (k * num.y + j) * num.x;
                    voxel_grid[index] = true;
                }
            }
        }
    }

   //for (int x = 0; x < m_num.x; x++) {
   //  for (int y = 0; y < m_num.y; y++) {
   //    for (int z = 0; z < m_num.z; z++) {
   //      int index = x + (z * m_num.y + y) * m_num.x;
   //      Vector3 worldPos = m_width * Vector3(x, y, z) + m_worldPos;
   //      m_voxel_grid.data[index] = check_collision(worldPos);
   //    }
   //  }
   //}
}

void march(std::vector<Vector3>& vertex)
{
    vertex.clear();
    update_grid();

    for (int y = 0; y < m_num.y - 1; y++)
    {
        for (int x = 0; x < m_num.x - 1; x++)
        {
            for (int z = 0; z < m_num.z - 1; z++)
            {
                march_cube(XMINT3(x, y, z), vertex);
            }
        }
    }
}

void march_cube(XMINT3 pos, ::vector<Vector3>& vertex)
{
    auto trianglulation = get_triangulations(pos.x, pos.y, pos.z);
    for (int i = 0; i < 15; i++)
    {
        int edge_index = trianglulation[i];
        if (edge_index < 0)
        {
            break;
        }

        auto point_indecies = EDGES_TABLE[edge_index];
        assert(edge_index < 12);
        XMINT3 p1 = POINTS_TABLE[point_indecies[0]];
        XMINT3 p2 = POINTS_TABLE[point_indecies[1]];

        Vector3 worldP1 =
        (Vector3(p1.x, p1.y, p1.z) + Vector3(pos.x, pos.y, pos.z)) * m_width +
        m_worldOffset;
        Vector3 worldP2 =
        (Vector3(p2.x, p2.y, p2.z) + Vector3(pos.x, pos.y, pos.z)) * m_width +
        m_worldOffset;

        Vector3 final_point = (worldP1 + worldP2) / 2;
        vertex.push_back(final_point);
    }
}

const int* MarchingCube::get_triangulations(
UINT x, UINTy,
UINT z) {
  /*

  indecies:
           (5)-------(6)
         .  |      .  |
      (4)-------(7)   |
       |    |    |    |
       |   (1)---|---(2)
       | .       | .
      (0)-------(3)

  */

UINT idx = 0;
  idx |= (UINT)(!m_voxel_grid.get(x, y, z)) << 0;
  idx |= (UINT)(!m_voxel_grid.get(x, y, z + 1)) << 1;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y, z + 1)) << 2;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y, z)) << 3;
  idx |= (UINT)(!m_voxel_grid.get(x, y + 1, z)) << 4;
  idx |= (UINT)(!m_voxel_grid.get(x, y + 1, z + 1)) << 5;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y + 1, z + 1)) << 6;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y + 1, z)) << 7;

  assert(idx < 256);

  return
TRIANGULATIONS[ idx];
}

bool check_collision(in float3point) {
float min_dist = m_particle_radius;
  for (auto& p : m_particles) {
float d = Vector3::Distance(point, p.position);
    if (d <= min_dist) {
      return true;
    }
  }
  return false;
}

[numthreads(64, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    
}