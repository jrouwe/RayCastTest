#include "RayCastGPUAABBTree.h"
#include "StripifyFlags.h"

inline uint get_flags(float3 inVertex)
{
	uint3 u3 = asuint(inVertex) & uint3_one;
	return u3.x + (u3.y << 1) + (u3.z << 2);
}

inline float3 remove_flags(float3 inVertex)
{
	return asfloat(asuint(inVertex) & uint3_all_but_lowest_bit_set);
}

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
				uint left_child = node;
				uint right_child = node - 28 + (node_props & MASK_OUT_HAS_TRIANGLES);

				// Test bounds of left child
				float3 left_bounds_min = asfloat(tree.Load3(left_child));
				float3 left_bounds_max = asfloat(tree.Load3(left_child + 12));
				float left_distance = RayAABox(origin, inv_direction, is_parallel, left_bounds_min, left_bounds_max);
				bool left_intersects = left_distance < closest;

				// Test bounds of right child
				float3 right_bounds_min = asfloat(tree.Load3(right_child));
				float3 right_bounds_max = asfloat(tree.Load3(right_child + 12));
				float right_distance = RayAABox(origin, inv_direction, is_parallel, right_bounds_min, right_bounds_max);
				bool right_intersects = right_distance < closest;

				if (left_intersects && right_intersects)
				{
					// Both collide
					if (left_distance < right_distance)
					{
						// Left child before right child
						stack[stack_pos++] = right_child;
						node = left_child;
					}
					else
					{
						// Right child before left child
						stack[stack_pos++] = left_child;
						node = right_child;
					}
					continue;
				}
				else if (left_intersects)
				{
					// Only left collides
					node = left_child;
					continue;
				}
				else if (right_intersects)
				{
					// Only right collides
					node = right_child;
					continue;
				}
			}
			else
			{	
				// Avoid compiler warning 'possibly unused variable' and expand the first triangle
				float3 v[3];
				v[0] = remove_flags(asfloat(tree.Load3(node)));
				node += 12;
				v[1] = remove_flags(asfloat(tree.Load3(node)));
				node += 12;
				v[2] = remove_flags(asfloat(tree.Load3(node)));
				node += 12;
				closest = min(closest, RayTriangle(origin, direction, v[0], v[1], v[2]));

				// Loop all triangles in node
				for (uint triangles_left = (node_props & MASK_OUT_HAS_TRIANGLES) - 1; triangles_left > 0; --triangles_left)
				{
					float3 next = asfloat(tree.Load3(node));
					node += 12;

					uint flags = get_flags(next);
					next = remove_flags(next);

					if (flags == STRIPIFY_FLAG_START_STRIP_V1)
					{
						// Start of new strip, load first 2 vertices
						v[0] = next;
						v[1] = remove_flags(asfloat(tree.Load3(node)));
						node += 12;
						next = remove_flags(asfloat(tree.Load3(node)));
						node += 12;
					}
					else
					{
						// Continuation of strip, determine first 2 vertices
						uint a = flags & STRIPIFY_FLAG_A_MASK;
						uint b = (a + ((flags & STRIPIFY_FLAG_B_MASK) >> STRIPIFY_FLAG_B_SHIFT) + 1) % 3;
						float3 va = v[a];
						float3 vb = v[b];
						v[0] = va;
						v[1] = vb;
					}

					// Load third vertex
					v[2] = next;

					closest = min(closest, RayTriangle(origin, direction, v[0], v[1], v[2]));
				}				
			}
		}

		// Fetch next node
		node = stack[--stack_pos];
	}

	// Store minimum distance
	output[tid.x] = closest;
}
