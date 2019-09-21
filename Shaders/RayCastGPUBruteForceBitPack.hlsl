#include "RayTriangle.h"

#ifndef group_size
	#define group_size 128
#endif

#define component_bits 21
#define component_max ((1 << component_bits) - 1)

cbuffer DecompressionInfo : register(b0)
{
	float3		Offset;
	float		Pad;
	float3		Scale;
}

struct RayCast
{
	float3		Origin;
	float3		Direction;
};

StructuredBuffer<uint> vertices : register(t0);

StructuredBuffer<RayCast> rays : register(t1);

// Can't do atomics on float, so using int here (which will be fine as long as Distance > 0)
RWStructuredBuffer<int> output : register(u0);

float3 DecompressVertex(uint v1, uint v2)
{
	float3 rv;
	rv.x = Offset.x + Scale.x * (v1 & component_max);
	rv.y = Offset.y + Scale.y * ((v1 >> component_bits) | ((v2 >> component_bits) << (32 - component_bits)));
	rv.z = Offset.z + Scale.z * (v2 & component_max);
	return rv;
}

[numthreads(group_size,1,1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
	// Get ray origin and direction
	float3 origin = rays[gid.y].Origin;
	float3 direction = rays[gid.y].Direction;

	// Calculate real triangle to take
	uint idx = gid.x * 6 * group_size + gtid.x;

	// Fetch triangle vertices
	uint v01 = vertices[idx + 0 * group_size];
	uint v02 = vertices[idx + 1 * group_size];
	float3 v0 = DecompressVertex(v01, v02);
	uint v11 = vertices[idx + 2 * group_size];
	uint v12 = vertices[idx + 3 * group_size];
	float3 v1 = DecompressVertex(v11, v12);
	uint v21 = vertices[idx + 4 * group_size];
	uint v22 = vertices[idx + 5 * group_size];
	float3 v2 = DecompressVertex(v21, v22);

	// Calculate collision distance
	float distance = RayTriangle(origin, direction, v0, v1, v2);

	// Store minimum
	if (distance < FLT_MAX)
		InterlockedMin(output[gid.y], asint(distance));
}
