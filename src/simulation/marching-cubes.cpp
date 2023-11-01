#include "marching-cubes.h"

#include <vector>

#include "SimpleMath.h"

using namespace std;

/*
 * cube vetex order
 *  -
 *  + x
 *  + x + z
 *  + z
 *  + y
 *  + y + x
 *  + y + x + z
 *  + y + z
 */

MarchingCube::MarchingCube(const Vector3 _bound, double _l,
                           std::vector<Particle>* _particles,
                           float particle_radius)
    : bound(_bound),
      l(_l),
      sum_v((1 + (int)(_bound.x * 2 / _l)) * (1 + (int)(_bound.y * 2 / _l)) *
            (1 + (int)(_bound.z * 2 / _l))),
      sum_e((1 + (int)(_bound.x * 2 / _l)) * (1 + (int)(_bound.y * 2 / _l)) *
            (1 + (int)(_bound.z * 2 / _l)) * 3),
      particles(_particles),
      m_particles_r(particle_radius),
      offset(8),
      offset_edge(12),
      intersections(sum_e, NULL),
      inside(sum_v, NULL),
      mapping(sum_e) {
  base = -bound;
  total_edge[0] = (int)(_bound.x * 2 / l);
  total_edge[1] = (int)(_bound.y * 2 / l);
  total_edge[2] = (int)(_bound.z * 2 / l);

  dx = (total_edge[2] + 1) * (total_edge[1] + 1);
  dy = total_edge[2] + 1;
  dz = 1;

  offset[0] = 0;
  offset[1] = dx;
  offset[2] = dx + dz;
  offset[3] = dz;
  offset[4] = dy;
  offset[5] = dy + dx;
  offset[6] = dx + dy + dz;
  offset[7] = dy + dz;

  offset_edge[0] = 0;
  offset_edge[1] = dx * 3 + 2;
  offset_edge[2] = dz * 3;
  offset_edge[3] = 2;
  offset_edge[4] = dy * 3;
  offset_edge[5] = (dy + dx) * 3 + 2;
  offset_edge[6] = (dy + dz) * 3;
  offset_edge[7] = dy * 3 + 2;
  offset_edge[8] = 1;
  offset_edge[9] = dx * 3 + 1;
  offset_edge[10] = (dx + dz) * 3 + 1;
  offset_edge[11] = dz * 3 + 1;
}

MarchingCube::~MarchingCube() {
  for (int i = 0; i < intersections.size(); ++i) {
    if (intersections[i]) {
      delete (intersections[i]);
    }
  }
}

void MarchingCube::count(vector<Vector3>& vertexs, vector<UINT>& tri_index) {
  vertexs.clear();
  tri_index.clear();

  // count inside
  int p = 0;
  for (int i = 0; i <= total_edge[0]; ++i) {
    for (int j = 0; j <= total_edge[1]; ++j) {
      for (int k = 0; k <= total_edge[2]; ++k, ++p) {
        Vector3 vetex(base.x + i * l, base.y + j * l, base.z + k * l);
        inside[p] = check(vetex);
      }
    }
  }

  // cout intersection
  p = 0;
  Particle* tmp = NULL;
  for (int i = 0; i <= total_edge[0]; ++i) {
    for (int j = 0; j <= total_edge[1]; ++j) {
      for (int k = 0; k <= total_edge[2]; ++k, p += 3) {
        Vector3 v0(base.x + i * l, base.y + j * l, base.z + k * l);
        Vector3 v1(base.x + i * l + l, base.y + j * l, base.z + k * l);
        Vector3 v2(base.x + i * l, base.y + j * l + l, base.z + k * l);
        Vector3 v3(base.x + i * l, base.y + j * l, base.z + k * l + l);

        if ((tmp = diff(inside[p / 3], inside[p / 3 + dx]))) {
          intersections[p] = countInter(v0, v1, tmp);
        }
        if ((tmp = diff(inside[p / 3], inside[p / 3 + dy]))) {
          intersections[p + 1] = countInter(v0, v2, tmp);
        }
        if ((tmp = diff(inside[p / 3], inside[p / 3 + dz]))) {
          intersections[p + 2] = countInter(v0, v3, tmp);
        }
      }
    }
  }

  // intersection mapping
  for (int i = 0; i < sum_e; ++i) {
    if (intersections[i]) {
      mapping[i] = vertexs.size();
      vertexs.push_back(*intersections[i]);
    }
  }
  // printf("edge num: %d\n", (int)vertexs.size());

  // lookup
  p = 0;
  // printf("rang: %d, %d, %d\n", total_edge[0], total_edge[1], total_edge[2]);
  for (int i = 0; i < total_edge[0]; ++i) {
    for (int j = 0; j < total_edge[1]; ++j) {
      for (int k = 0; k < total_edge[2]; ++k, ++p) {
        // count status
        int status = 0;
        for (int sta_i = 0, tw = 1; sta_i < 8; ++sta_i, tw = tw << 1) {
          status |= inside[p + offset[sta_i]] ? tw : 0;
        }
        if (status == 0 || status == 255) continue;

        // count triangle
        for (int tri_p = 0; tri_p < 16 && TRI_TABLE[status][tri_p] >= 0;
             ++tri_p) {
          tri_index.push_back(
              mapping[offset_edge[TRI_TABLE[status][tri_p]] + p * 3]);
        }
      }
      p += dz;
    }
    p += dy;
  }
}

// return the closeset particle if there is a particle contain this vertex,
// otherwise NULL
Particle* MarchingCube::check(Vector3 v) const {
  double min_dis2 = l * l + 0.1;
  Particle* result = NULL;
  for (auto particle : *particles) {
    double dis2 = Vector3::DistanceSquared(particle.position, v);
    if (dis2 < min_dis2) {
      min_dis2 = dis2;
      result = &particle;
    }
  }
  if (min_dis2 < m_particles_r * m_particles_r) return result;
  return NULL;
}

// if one in one not, return the one in, otherwise, return NULL
Particle* MarchingCube::diff(Particle* a, Particle* b) const {
  if (a == NULL && b != NULL) return b;
  if (a != NULL && b == NULL) return a;
  return NULL;
}

Vector3* MarchingCube::countInter(const Vector3& v, const Vector3& u,
                                  Particle* tmp) const {
  return new Vector3((v + u) * 0.5f);

  // TODO: count intersection between v-u and particle tmp
}
