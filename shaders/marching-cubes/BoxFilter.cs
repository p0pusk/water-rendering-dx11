#include "shaders/Sph.h"

#define KERNEL_SIZE_X 3
#define KERNEL_SIZE_Y 3
#define KERNEL_SIZE_Z 3
#define KERNEL_WEIGHT 1.f / 9.f

RWStructuredBuffer<float> inputMatrix : register(u0);
RWStructuredBuffer<float> outputMatrix : register(u1);

[numthreads(BLOCK_SIZE, 1, 1)]
void cs(uint3 DTid : SV_DispatchThreadID) {
    uint3 cell = GetMCCell(DTid.x);
    if (cell.x >= MC_DIMENSIONS.x || cell.y >= MC_DIMENSIONS.y || cell.z >= MC_DIMENSIONS.z)
    {
      return;
    }

    // Calculate the center of the kernel
    int centerX = KERNEL_SIZE_X / 2;
    int centerY = KERNEL_SIZE_Y / 2;
    int centerZ = KERNEL_SIZE_Z / 2;

    // Accumulator for the filtered value
    float filteredValue = 0.0f;

    // Iterate over the kernel
    for (int dz = -centerZ; dz <= centerZ; ++dz) {
        for (int dy = -centerY; dy <= centerY; ++dy) {
            for (int dx = -centerX; dx <= centerX; ++dx) {

                // Calculate the index in the input matrix
                int3 index = { uint(cell.x) + dx, uint(cell.y) + dy, uint(cell.z) + dz };

                // Check bounds
                if (index.x >= 0 && index.x < MC_DIMENSIONS.x &&
                    index.y >= 0 && index.y < MC_DIMENSIONS.y &&
                    index.z >= 0 && index.z < MC_DIMENSIONS.z) {
                    // Accumulate the value from the input matrix weighted by the kernel
                    filteredValue += inputMatrix[index.z * MC_DIMENSIONS.y * MC_DIMENSIONS.x + index.y * MC_DIMENSIONS.x + index.x] * KERNEL_WEIGHT;
                }
            }
        }
    }

    // Write the filtered value to the output matrix
    outputMatrix[DTid.x] = filteredValue;
}

