#include "RayCastGPUAABBList.h"
#include "RayAABox.h"

AppendStructuredBuffer<JobItem> jobs : register(u1);

[numthreads(group_size, 1, 1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
	// Get parameters
	uint box_idx = gid.x * group_size + gtid.x;
	uint ray_idx = gid.y;

	// Get ray
	float3 origin = rays[ray_idx].Origin;
	float3 direction = rays[ray_idx].Direction;
	float3 inv_direction = rcp(direction);
	bool3 is_parallel = RayIsParallel(direction);

	// Get bounding box, coalesced read
	uint bounds_offset = gid.x * 2 * group_size + gtid.x;
	float3 bounds_min = bounds[bounds_offset];
	float3 bounds_max = bounds[bounds_offset + group_size];

	// Test bounding box
	if (bounds_min.x <= bounds_max.x // Last batch contains invalid bounding boxes which we need to ignore here
		&& RayAABox(origin, inv_direction, is_parallel, bounds_min, bounds_max) < FLT_MAX)
	{
		JobItem item;
		item.RayIdx = ray_idx;
		item.GroupIdx = box_idx;
		jobs.Append(item);
	}

	// Initialize output distance
	if (box_idx == 0)
		output[ray_idx] = asint(FLT_MAX);
}
