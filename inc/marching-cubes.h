#pragma once

#include <list>
#include <vector>

#include "SimpleMath.h"
#include "lookup-list.h"
#include "particle.h"

using namespace std;

class MarchingCube {
 public:
  MarchingCube() {}
  MarchingCube(const Vector3 _bound, double _l,
               std::vector<Particle>* _particles, float particles_r);
  ~MarchingCube();

  // count the mesh
  void count(vector<Vector3>& vertexs, vector<UINT>& tri_index);

 private:
  // return the closeset particle if there is a particle contain this vertex,
  // otherwise NULL
  Particle* check(Vector3 v) const;

  // if one in one not, return the one in, otherwise, return NULL
  Particle* diff(Particle* a, Particle* b) const;

  // return the intersect point one the edge
  Vector3* countInter(const Vector3& v, const Vector3& u, Particle* tmp) const;

 private:
  // position and size of the container
  Vector3 base, bound;
  // edge length of the cube
  double l;
  // number of edges on each coordinate
  int total_edge[3];
  // total number of vertex and edge
  int sum_v, sum_e;

  // list of particles
  std::vector<Particle>* particles;

  /*	list of all edges for cubes
      point to the cut point if intersect with the mesh, otherwise NULL
      edge are numbered by related vetex towards z, y, x
   */
  vector<Vector3*> intersections;
  // related index offset for twelve edges in a cube
  vector<int> offset_edge;
  // related index offset for two edge that is one step different on each
  // coordinate
  int dx, dy, dz;
  // map edge to index of final mesh vertex
  vector<int> mapping;
  float m_particles_r;

  // list of all vertexs for cubes, point to the closest particle if it is
  // inside, otherwise NULL
  vector<Particle*> inside;
  // related index offset for eight vertex in a cube
  vector<int> offset;
};
