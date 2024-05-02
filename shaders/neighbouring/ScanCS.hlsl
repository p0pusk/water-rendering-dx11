//--------------------------------------------------------------------------------------
// File: ScanCS.hlsl
//
// A simple inclusive prefix sum(scan) implemented in CS4.0, 
// using a typical up sweep and down sweep scheme
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
StructuredBuffer<uint> Input : register( t0 );     // Change uint2 here if scan other types, and
RWStructuredBuffer<uint> Result : register( u0 );  // also here

#define groupthreads 1024
groupshared uint2 bucket[groupthreads];             // Change uint4 to the "type x2" if scan other types, e.g.
                                                    // if scan uint2, then put uint4 here,
                                                    // if scan float, then put float2 here

void CSScan( uint3 DTid, uint GI, uint x )         // Change the type of x here if scan other types
{
    // since CS40 can only support one shared memory for one shader, we use .xy and .zw as ping-ponging buffers
    // if scan a single element type like int, search and replace all .xy to .x and .zw to .y below
    bucket[GI].x = x; 
    bucket[GI].y = 0;

    // Up sweep    
    [unroll]
    for ( uint stride = 2; stride <= groupthreads; stride <<= 1 )
    {
        GroupMemoryBarrierWithGroupSync();
        
        if ( (GI & (stride - 1)) == (stride - 1) )
        {
            bucket[GI].x += bucket[GI - stride/2].x;
        }
    }

    if ( GI == (groupthreads - 1) ) 
    {
        bucket[GI].x = 0;
    }

    // Down sweep
    bool n = true;
    [unroll]
    for ( stride = groupthreads / 2; stride >= 1; stride >>= 1 )
    {
        GroupMemoryBarrierWithGroupSync();

        uint a = stride - 1;
        uint b = stride | a;

        if ( n )        // ping-pong between passes
        {
            if ( ( GI & b) == b )
            {
                bucket[GI].y = bucket[GI-stride].x + bucket[GI].x;
            } else
            if ( (GI & a) == a )
            {
                bucket[GI].y = bucket[GI+stride].x;
            } else        
            {
                bucket[GI].y = bucket[GI].x;
            }
        } else
        {
            if ( ( GI & b) == b )
            {
                bucket[GI].x = bucket[GI-stride].y + bucket[GI].y;
            } else
            if ( (GI & a) == a )
            {
                bucket[GI].x = bucket[GI+stride].y;
            } else        
            {
                bucket[GI].x = bucket[GI].y;
            }
        }
        
        n = !n;
    }    

    Result[DTid.x] = bucket[GI].y + x;
}

// scan in each bucket
[numthreads( groupthreads, 1, 1 )]
void CSScanInBucket( uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI: SV_GroupIndex )
{
    uint x = Input[DTid.x];                    // Change the type of x here if scan other types 
    CSScan( DTid, GI, x );
}

// record and scan the sum of each bucket
[numthreads( groupthreads, 1, 1 )]
void CSScanBucketResult( uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI: SV_GroupIndex )
{
    uint x = Input[DTid.x*groupthreads - 1];   // Change the type of x here if scan other types
    CSScan( DTid, GI, x );
}

StructuredBuffer<uint> Input1 : register( t1 );

// add the bucket scanned result to each bucket to get the final result
[numthreads( groupthreads, 1, 1 )]
void CSScanAddBucketResult( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI: SV_GroupIndex )
{
    Result[DTid.x] = Input[DTid.x] + Input1[Gid.x];
}
