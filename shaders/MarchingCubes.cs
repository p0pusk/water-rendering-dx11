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
    float3 v[3];
};

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<int> voxel_grid : register(u0);
AppendStructuredBuffer<Triangle> triangles : register(u1);
RWStructuredBuffer<SurfaceBuffer> surfaceBuffer : register(u2);

static const float g_c = 0.f;

bool voxel_get(in uint x, in uint y, in uint z)
{
    uint3 num = ceil(boundaryLen / marchingWidth);
    return voxel_grid[x + y * num.x + z * num.x * num.y] - g_c;
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
    idx |= (uint) (!voxel_get(x, y, z)) << 0;
    idx |= (uint) (!voxel_get(x, y, z + 1)) << 1;
    idx |= (uint) (!voxel_get(x + 1, y, z + 1)) << 2;
    idx |= (uint) (!voxel_get(x + 1, y, z)) << 3;
    idx |= (uint) (!voxel_get(x, y + 1, z)) << 4;
    idx |= (uint) (!voxel_get(x, y + 1, z + 1)) << 5;
    idx |= (uint) (!voxel_get(x + 1, y + 1, z + 1)) << 6;
    idx |= (uint) (!voxel_get(x + 1, y + 1, z)) << 7;

    return TRIANGULATIONS[idx];
}

float3 get_point(in uint edge_index, in uint3 pos)
{
    Edges point_indecies = EDGES_TABLE[edge_index];
    uint3 p1 = POINTS_TABLE[point_indecies.data[0]];
    uint3 p2 = POINTS_TABLE[point_indecies.data[1]];

    float3 worldP1 =
        (p1 + pos) * marchingWidth + float3(marchingWidth / 2, marchingWidth / 2, marchingWidth / 2) + worldPos;
    float3 worldP2 =
        (p2 + pos) * marchingWidth + float3(marchingWidth / 2, marchingWidth / 2, marchingWidth / 2) + worldPos;

    uint3 num = ceil(boundaryLen / marchingWidth);
    uint3 cell1 = p1 + pos;
    uint index1 = cell1.x + cell1.y * num.x + cell1.z * num.y * num.z;
    uint3 cell2 = p2 + pos;
    uint index2 = cell2.x + cell2.y * num.x + cell2.z * num.y * num.z;
    float alpha = (g_c - voxel_grid[index1]) / (voxel_grid[index2] - voxel_grid[index1]);

    return (1 - alpha) * worldP1 + alpha * worldP2;
}

void march_cube(in uint3 pos)
{
    Triangulation edges = get_triangulations(pos.x, pos.y, pos.z);
    for (int i = 0; i < 15 && edges.data[i] != -1; i += 3)
    {
        Triangle res;
        res.v[0] = get_point(edges.data[i], pos);
        res.v[1] = get_point(edges.data[i + 1], pos);
        res.v[2] = get_point(edges.data[i + 2], pos);
        triangles.Append(res);
    }
}


void march(in uint index)
{
    uint3 num = ceil(boundaryLen / marchingWidth);
    uint x = index % num.x;
    uint y = (index / num.x) % num.y;
    uint z = (index / num.x) / num.y;

    if (x > num.x - 2 || y > num.y - 2 || z > num.z - 2)
    {
      return;
    }

    // if (voxel_grid[index]) {
    //   Triangle res;
    //   res.v[0] = float3(x, y, z) * marchingWidth;
    //   res.v[1] = float3(x, y, z) * marchingWidth;
    //   res.v[2] = float3(x, y, z) * marchingWidth;
    //   triangles.Append(res);
    // }
    march_cube(uint3(x, y, z));
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID)
{
  // index = x + y * num.x + z * num.x * num.y
  // index = x + (y+ z * num.y) * num.x
  // x = index % num.x
  // y  + z * num.y = index / num.x
  // y = (index / num.x) % num.y
  // z = (index / num.x) / num.y
    march(globalThreadId.x);
}
