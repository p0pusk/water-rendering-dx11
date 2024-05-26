#include "shaders/Sph.h"
#include "shaders/LookUpTable.h"


struct Triangulation
{
    int data[15];
};

struct Edges
{
    int data[2];
};

struct Triangle
{
    float3 pos[3];
    float3 norm[3];
};

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<float> voxel_grid : register(u0);
AppendStructuredBuffer<Triangle> triangles : register(u1);
RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u2);

static const float g_c = 0.25;

float voxel_get(in uint x, in uint y, in uint z)
{
    return voxel_grid[x + y * MC_DIMENSIONS.x + z * MC_DIMENSIONS.x * MC_DIMENSIONS.y];
}



Triangulation get_triangulations(in uint x, in uint y, in uint z)
{
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

    uint idx = 0;
    idx |= (uint) !(voxel_get(x, y, z) > g_c) << 0;
    idx |= (uint) !(voxel_get(x, y, z + 1) > g_c) << 1;
    idx |= (uint) !(voxel_get(x + 1, y, z + 1) > g_c) << 2;
    idx |= (uint) !(voxel_get(x + 1, y, z) > g_c) << 3;
    idx |= (uint) !(voxel_get(x, y + 1, z) > g_c) << 4;
    idx |= (uint) !(voxel_get(x, y + 1, z + 1) > g_c) << 5;
    idx |= (uint) !(voxel_get(x + 1, y + 1, z + 1) > g_c) << 6;
    idx |= (uint) !(voxel_get(x + 1, y + 1, z) > g_c) << 7;

    return TRIANGULATIONS[idx];
}

float3 get_gradient(in uint3 pos) {
  float3 gradient;

  float xp, xn, yp, yn, zp, zn;
  if (pos.x + 1 < MC_DIMENSIONS.x) {
    xp = voxel_grid[pos.x + 1 + pos.y * MC_DIMENSIONS.x + pos.z *
      MC_DIMENSIONS.x * MC_DIMENSIONS.y];
  }

  if (pos.y + 1 < MC_DIMENSIONS.y) {
    yp = voxel_grid[pos.x + (pos.y + 1) * MC_DIMENSIONS.x + pos.z *
      MC_DIMENSIONS.x * MC_DIMENSIONS.y];
  }

  if (pos.z + 1 < MC_DIMENSIONS.z) {
    zp = voxel_grid[pos.x + pos.y * MC_DIMENSIONS.x + (pos.z + 1) *
      MC_DIMENSIONS.x * MC_DIMENSIONS.y];
  }

  if (pos.x - 1 >= 0) {
    xn = voxel_grid[pos.x - 1 + pos.y * MC_DIMENSIONS.x + pos.z *
      MC_DIMENSIONS.x * MC_DIMENSIONS.y];
  }

  if (pos.y - 1 >= 0) {
    yn = voxel_grid[pos.x + (pos.y - 1) * MC_DIMENSIONS.x + pos.z *
      MC_DIMENSIONS.x * MC_DIMENSIONS.y];
  }

  if (pos.x - 1 >= 0) {
    zn = voxel_grid[pos.x + pos.y * MC_DIMENSIONS.x + (pos.z - 1) *
      MC_DIMENSIONS.x * MC_DIMENSIONS.y];
  }

  gradient.x = (xp - xn) / 2;
  gradient.y = (yp - yn) / 2;
  gradient.z = (zp - zn) / 2;
 
  return gradient;
}

struct Vertex {
  float3 pos;
  float3 norm;
};

Vertex get_point(in uint edge_index, in uint3 pos)
{
    Edges point_indecies = EDGES_TABLE[edge_index];
    uint3 p1 = POINTS_TABLE[point_indecies.data[0]];
    uint3 p2 = POINTS_TABLE[point_indecies.data[1]];

    float3 worldP1 =
        (p1 + pos + float3(0.5f, 0.5f, 0.5f)) * marchingWidth + worldPos;
    float3 worldP2 =
        (p2 + pos + float3(0.5f, 0.5f, 0.5f)) * marchingWidth + worldPos;

    uint3 cell1 = p1 + pos;
    uint index1 = cell1.x + cell1.y * MC_DIMENSIONS.x + cell1.z * MC_DIMENSIONS.x * MC_DIMENSIONS.y;
    uint3 cell2 = p2 + pos;
    uint index2 = cell2.x + cell2.y * MC_DIMENSIONS.x + cell2.z * MC_DIMENSIONS.x * MC_DIMENSIONS.y;
    float alpha = (g_c - voxel_grid[index1]) / (voxel_grid[index2] - voxel_grid[index1]);


    Vertex res;

    res.pos = (1 - alpha) * worldP1 + alpha * worldP2;

    float t = (g_c - voxel_grid[index1]) / (voxel_grid[index2] - voxel_grid[index1]);
    float3 normal1 = get_gradient(cell1);
    float3 normal2 = get_gradient(cell2);

    res.norm = normalize(lerp(normal1, normal2, t));

    return res;
}

void march_cube(in uint3 pos)
{
    Triangulation edges = get_triangulations(pos.x, pos.y, pos.z);
    for (int i = 0; i < 15 && edges.data[i] != -1; i += 3)
    {
        Triangle res;
        Vertex v;
        v = get_point(edges.data[i], pos);
        res.pos[0] = v.pos;
        res.norm[0] = v.norm;
        v = get_point(edges.data[i + 1], pos);
        res.pos[1] = v.pos;
        res.norm[1] = v.norm;
        v = get_point(edges.data[i + 2], pos);
        res.pos[2] = v.pos;
        res.norm[2] = v.norm;
        triangles.Append(res);
    }
}


void march(in uint index)
{
    uint3 cell = GetMCCell(index);

    if (cell.x >= MC_DIMENSIONS.x - 1 || cell.y >= MC_DIMENSIONS.y - 1 || cell.z >= MC_DIMENSIONS.z - 1)
    {
      return;
    }

    march_cube(cell);
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
    march(globalThreadId.x);
}
