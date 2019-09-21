#include "RayCastGPUAABBList.h"
#include "RayTriangle.h"

ConsumeStructuredBuffer<JobItem> jobs : register(u1);

groupshared JobItem item;

float3 GetVertex(uint idx)
{
	float3 v;
	v.x = vertices[idx];
	v.y = vertices[idx + group_size];
	v.z = vertices[idx + 2 * group_size];
	return v;
}

[numthreads(group_size, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID)
{
	// Get work item
	if (gtid.x == 0)
		item = jobs.Consume();

	// Wait until item is available to all threads
	GroupMemoryBarrierWithGroupSync();

	// Calculate ray and triangle to test
	uint ray_idx = item.RayIdx;
	uint tri_idx = item.GroupIdx * 36 * group_size + gtid.x;

	// Get ray
	float3 origin = rays[ray_idx].Origin;
	float3 direction = rays[ray_idx].Direction;

	// 1st triangle, coalesced read
	float3 v00 = GetVertex(tri_idx + (9 * 0 + 0) * group_size);
	float3 v01 = GetVertex(tri_idx + (9 * 0 + 3) * group_size);
	float3 v02 = GetVertex(tri_idx + (9 * 0 + 6) * group_size);
	float r1 = RayTriangle(origin, direction, v00, v01, v02);

	// 2nd triangle, coalesced read
	float3 v10 = GetVertex(tri_idx + (9 * 1 + 0) * group_size);
	float3 v11 = GetVertex(tri_idx + (9 * 1 + 3) * group_size);
	float3 v12 = GetVertex(tri_idx + (9 * 1 + 6) * group_size);
	float r2 = RayTriangle(origin, direction, v10, v11, v12);

	// 3rd triangle, coalesced read
	float3 v20 = GetVertex(tri_idx + (9 * 2 + 0) * group_size);
	float3 v21 = GetVertex(tri_idx + (9 * 2 + 3) * group_size);
	float3 v22 = GetVertex(tri_idx + (9 * 2 + 6) * group_size);
	float r3 = RayTriangle(origin, direction, v20, v21, v22);

	// 4th triangle, coalesced read
	float3 v30 = GetVertex(tri_idx + (9 * 3 + 0) * group_size);
	float3 v31 = GetVertex(tri_idx + (9 * 3 + 3) * group_size);
	float3 v32 = GetVertex(tri_idx + (9 * 3 + 6) * group_size);
	float r4 = RayTriangle(origin, direction, v30, v31, v32);
	// Store minimum
	float min_dist = min(min(r1, r2), min(r3, r4));
	if (min_dist < FLT_MAX)
		InterlockedMin(output[ray_idx], asint(min_dist));
}
