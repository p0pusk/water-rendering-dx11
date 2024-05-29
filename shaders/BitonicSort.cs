//--------------------------------------------------------------------------------------
// File: ComputeShaderSort11.hlsl
//
// This file contains the compute shaders to perform GPU sorting using DirectX 11.
// 
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------

#define BITONIC_BLOCK_SIZE 1024

#define TRANSPOSE_BLOCK_SIZE 16

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer CB : register( b0 )
{
    uint g_iLevel;
    uint g_iLevelMask;
    uint g_iWidth;
    uint g_iHeight;
};

//--------------------------------------------------------------------------------------
// Structured Buffers
//--------------------------------------------------------------------------------------

struct DiffuseParticle {
  float3 position;
  float3 velocity;
  // 0 - spray, 1 - foam, 2 - bubbles
  uint type;
  float lifetime;
};

StructuredBuffer<DiffuseParticle> Input : register( t0 );
RWStructuredBuffer<DiffuseParticle> Data : register( u0 );

//--------------------------------------------------------------------------------------
// Bitonic Sort Compute Shader
//--------------------------------------------------------------------------------------
groupshared DiffuseParticle shared_data[BITONIC_BLOCK_SIZE];

[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void BitonicSort( uint3 Gid : SV_GroupID, 
                  uint3 DTid : SV_DispatchThreadID, 
                  uint3 GTid : SV_GroupThreadID, 
                  uint GI : SV_GroupIndex )
{
    // Load shared data
    shared_data[GI] = Data[DTid.x];
    GroupMemoryBarrierWithGroupSync();
    
    // Sort the shared data
    for (uint j = g_iLevel >> 1 ; j > 0 ; j >>= 1)
    {
        DiffuseParticle result;
        if ((shared_data[GI & ~j].lifetime <= shared_data[GI | j].lifetime) == (bool)(g_iLevelMask & DTid.x)) {
           result = shared_data[GI ^ j];
        } else {
           result = shared_data[GI];
        }
        GroupMemoryBarrierWithGroupSync();
        shared_data[GI] = result;
        GroupMemoryBarrierWithGroupSync();
    }
    
    // Store shared data
    Data[DTid.x] = shared_data[GI];
}

//--------------------------------------------------------------------------------------
// Matrix Transpose Compute Shader
//--------------------------------------------------------------------------------------
groupshared DiffuseParticle transpose_shared_data[TRANSPOSE_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE];

[numthreads(TRANSPOSE_BLOCK_SIZE, TRANSPOSE_BLOCK_SIZE, 1)]
void MatrixTranspose( uint3 Gid : SV_GroupID, 
                      uint3 DTid : SV_DispatchThreadID, 
                      uint3 GTid : SV_GroupThreadID, 
                      uint GI : SV_GroupIndex )
{
    transpose_shared_data[GI] = Input[DTid.y * g_iWidth + DTid.x];
    GroupMemoryBarrierWithGroupSync();
    uint2 XY = DTid.yx - GTid.yx + GTid.xy;
    Data[XY.y * g_iHeight + XY.x] = transpose_shared_data[GTid.x * TRANSPOSE_BLOCK_SIZE + GTid.y];
}
