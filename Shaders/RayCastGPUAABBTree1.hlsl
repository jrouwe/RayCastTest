// Variant 1: Tests bounds at each level of the tree and recurses to left and right child if it intersects
#include "RayCastGPUAABBTree.h"

[numthreads(group_size,1,1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	// Get ray
	float3 origin = raycasts[tid.x].Origin;
	float3 direction = raycasts[tid.x].Direction;
	float3 inv_direction = rcp(direction);
	bool3 is_parallel = RayIsParallel(direction);

	float closest = FLT_MAX;

	// Stack with -1 as terminator
	uint stack[stack_size];
	stack[0] = -1;
	uint stack_pos = 1;

	for (uint node = 0; node != -1; )
	{
		// Fetch node bounds
		float3 bounds_min = asfloat(tree.Load3(node));
		node += 12;
		float3 bounds_max = asfloat(tree.Load3(node));
		node += 12;

		// Fetch node properties
		uint node_props = tree.Load(node);
		node += 4;

		// Test if node is closer than closest hit result
		if (RayAABox(origin, inv_direction, is_parallel, bounds_min, bounds_max) < closest)
		{
			// Test if node contains triangles
			if ((node_props & HAS_TRIANGLES) == 0)
			{
				// No triangles, visit both child nodes

				// Push right node on stack
				uint right_child = node - 28 + (node_props & MASK_OUT_HAS_TRIANGLES);
				stack[stack_pos++] = right_child;

				// Node now implicitly points to the left node
				continue;
			}
			else
			{	
				// Loop all triangles in node
				for (uint triangles_left = (node_props & MASK_OUT_HAS_TRIANGLES); triangles_left > 0; --triangles_left)
				{
					float3 v1 = asfloat(tree.Load3(node));
					node += 12;
					float3 v2 = asfloat(tree.Load3(node));
					node += 12;
					float3 v3 = asfloat(tree.Load3(node));
					node += 12;

					closest = min(closest, RayTriangle(origin, direction, v1, v2, v3));
				}				
			}
		}

		// Fetch next node
		node = stack[--stack_pos];
	}

	// Store minimum distance
	output[tid.x] = closest;
}
