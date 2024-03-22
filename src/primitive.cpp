#include "primitive.h"

void Primitive::GetSphereDataSize(size_t latCells, size_t lonCells,
                                  size_t& indexCount, size_t& vertexCount) {
  vertexCount = (latCells + 1) * (lonCells + 1);
  indexCount = latCells * lonCells * 6;
}

void Primitive::CreateSphere(size_t latCells, size_t lonCells, UINT16* pIndices,
                             Vector3* pPos) {
  for (size_t lat = 0; lat < latCells + 1; lat++) {
    for (size_t lon = 0; lon < lonCells + 1; lon++) {
      int index = (int)(lat * (lonCells + 1) + lon);
      float lonAngle = 2.0f * (float)M_PI * lon / lonCells + (float)M_PI;
      float latAngle = -(float)M_PI / 2 + (float)M_PI * lat / latCells;

      Vector3 r = Vector3{sinf(lonAngle) * cosf(latAngle), sinf(latAngle),
                          cosf(lonAngle) * cosf(latAngle)};

      pPos[index] = r * 0.5f;
    }
  }

  for (size_t lat = 0; lat < latCells; lat++) {
    for (size_t lon = 0; lon < lonCells; lon++) {
      size_t index = lat * lonCells * 6 + lon * 6;
      pIndices[index + 0] = (UINT16)(lat * (latCells + 1) + lon + 0);
      pIndices[index + 2] = (UINT16)(lat * (latCells + 1) + lon + 1);
      pIndices[index + 1] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
      pIndices[index + 3] = (UINT16)(lat * (latCells + 1) + lon + 1);
      pIndices[index + 5] =
          (UINT16)(lat * (latCells + 1) + latCells + 1 + lon + 1);
      pIndices[index + 4] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
    }
  }
}
