#include "heightfield.h"

HeightField::HeightField(const Props& props) {
  m_props = props;

  m_grid.resize(props.gridSize.x);
  m_objects.resize(props.gridSize.x);
  m_numX = props.gridSize.x;
  m_numZ = props.gridSize.y;
  for (int i = 0; i < props.gridSize.x; i++) {
    m_grid[i].resize(props.gridSize.y);
    m_objects[i].resize(props.gridSize.y, ObjectCell{});
  }

  for (int i = 0; i < m_props.gridSize.x; i++) {
    for (int j = 0; j < m_props.gridSize.y; j++) {
      m_grid[i][j].height = m_props.height;
      m_grid[i][j].velocity = 0;
    }
  }
}

void HeightField::Update(float dt) {
  float c = min(m_props.waveSpeed, 0.5 * m_props.w / dt);
  float k = pow(c, 2) / pow(m_props.w, 2);
  float velDamping = 0.9f;
  float posDamping = 0.1f;

  int numX = m_props.gridSize.x;
  int numY = m_props.gridSize.y;

  // update objects volume
  for (int i = 0; i < numX; i++) {
    for (int j = 0; j < numY; j++) {
      // coeff of strenght of effect
      auto& obj = m_objects[i][j];
      if (obj.height != 0) {
        float alpha = 0.5;
        m_grid[i][j].height += alpha * (obj.height - obj.prev_height);

        float object_acceleration = 0;
        float force_to_obj =
            m_props.density * obj.height * pow(m_props.w, 2) * m_props.g;

        if (obj.mass != 0) {
          object_acceleration = 9.8f - force_to_obj / obj.mass;
        } else {
          object_acceleration = -force_to_obj / 0.1f;
        }

        obj.velocity += object_acceleration * dt;
        obj.prev_height = obj.height;
        obj.height = max(obj.height + obj.velocity * dt, 0);
        if (obj.height == 0) {
          obj.velocity = 0;
          obj.prev_height = 0;
        }
      }
    }
  }

  // update velocity
  for (int i = 0; i < numX; i++) {
    for (int j = 0; j < numY; j++) {
      float h = m_grid[i][j].height;
      float sumH = 0.0f;
      sumH += i > 0 ? m_grid[i - 1][j].height : h;
      sumH += i < numX - 1 ? m_grid[i + 1][j].height : h;
      sumH += j > 0 ? m_grid[i][j - 1].height : h;
      sumH += j < numY - 1 ? m_grid[i][j + 1].height : h;
      m_grid[i][j].velocity += dt * k * (sumH - 4.0f * h);

      m_grid[i][j].height +=
          (0.25 * sumH - h) * posDamping;  // positional damping
    }
  }

  // update height
  for (int i = 0; i < numX; i++) {
    for (int j = 0; j < numY; j++) {
      m_grid[i][j].velocity *= velDamping;                // velocity damping
      m_grid[i][j].height += m_grid[i][j].velocity * dt;  // velocity damping
    }
  }
}

void HeightField::GetRenderData(std::vector<Vector3>& vertecies,
                                std::vector<UINT16>& indecies) {
  auto& offset = m_props.pos;
  auto& w = m_props.w;
  vertecies.clear();
  vertecies.clear();
  for (int i = 0; i < m_numX; i++) {
    for (int j = 0; j < m_numZ; j++) {
      Vector3 pos =
          w * Vector3(i, m_grid[i][j].height - m_objects[i][j].height, j) +
          offset;
      vertecies.emplace_back(pos);
    }
  }

  for (int i = 0; i < m_numX - 1; i++) {
    for (int j = 0; j < m_numZ - 1; j++) {
      /* top view
        .         .
        ul        ur


        .         .
        bl        br */

      UINT bl = i * m_numZ + j;
      UINT ul = bl + 1;
      UINT br = bl + m_numZ;
      UINT ur = br + 1;
      indecies.push_back(bl);
      indecies.push_back(br);
      indecies.push_back(ul);
      indecies.push_back(br);
      indecies.push_back(ur);
      indecies.push_back(ul);
    }
  }
}

void HeightField::StartImpulse(int x, int z, float value, float radius) {
  float maxX = min(x + radius, m_numX - 1);
  float maxZ = min(z + radius, m_numZ - 1);
  float minX = max(x - radius, 0);
  float minZ = max(z - radius, 0);
  for (int i = minX; i <= maxX; i++) {
    for (int j = minZ; j <= maxZ; j++) {
      m_objects[i][j].height += value;
    }
  }
}

void HeightField::StopImpulse(int x, int z, float radius) {
  float maxX = min(x + radius, m_numX - 1);
  float maxZ = min(z + radius, m_numZ - 1);
  float minX = max(x - radius, 0);
  float minZ = max(z - radius, 0);
  for (int i = minX; i <= maxX; i++) {
    for (int j = minZ; j <= maxZ; j++) {
      m_objects[i][j].height = 0;
    }
  }
}
