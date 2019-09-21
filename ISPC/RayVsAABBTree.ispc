#include "RayTriangle.h"
#include "RayAABox.h"

// Tree properties
static const uint HAS_TRIANGLES = 0x80000000;
static const uint MASK_OUT_HAS_TRIANGLES = 0x7fffffff;

struct RayCast
{
	float		Origin[3];
	float		Direction[3];
};

inline float3 load3(const uniform uint tree[], varying uint node)
{
	float3 ret = { floatbits(tree[node + 0]), floatbits(tree[node + 1]), floatbits(tree[node + 2]) };
	return ret;	
}

export void RayVsAABBTree(const uniform RayCast raycasts[], const uniform uint num_rays, const uniform uint tree[], uniform float distances[])
{
	foreach (index = 0 ... num_rays)
	{
		// Get ray
		varying float3 origin = { raycasts[index].Origin[0], raycasts[index].Origin[1], raycasts[index].Origin[2] };
		varying float3 direction = { raycasts[index].Direction[0], raycasts[index].Direction[1], raycasts[index].Direction[2] };

		varying float closest = FLT_MAX;

		// Stack with 0 as terminator
		varying uint stack[64];
		stack[0] = 0;
		varying uint stack_pos = 1;

		varying uint node = 0;
		for (;;)
		{
			// Fetch node bounds
			varying float3 bounds_min = load3(tree, node);
			node += 3;
			varying float3 bounds_max = load3(tree, node);
			node += 3;

			// Fetch node properties
			varying uint node_props = tree[node];
			++node;
		
			// Test if node is closer than closest hit result
			if (RayAABox(origin, direction, bounds_min, bounds_max) < closest)
			{
				// Test if node contains triangles
				if ((node_props & HAS_TRIANGLES) == 0)
				{
					varying uint left_child = node;
					varying uint right_child = node - 7 + ((node_props & MASK_OUT_HAS_TRIANGLES) >> 2); // Shift by 2 since offset is in bytes and we're calculating in uints

					// Test bounds of left child
					varying float3 left_bounds_min = load3(tree, left_child);
					varying float3 left_bounds_max = load3(tree, left_child + 3);
					varying float left_distance = RayAABox(origin, direction, left_bounds_min, left_bounds_max);
					varying bool left_intersects = left_distance < closest;

					// Test bounds of right child
					varying float3 right_bounds_min = load3(tree, right_child);
					varying float3 right_bounds_max = load3(tree, right_child + 3);
					varying float right_distance = RayAABox(origin, direction, right_bounds_min, right_bounds_max);
					varying bool right_intersects = right_distance < closest;

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
					// Loop all triangles in node
					for (varying uint triangles_left = (node_props & MASK_OUT_HAS_TRIANGLES); triangles_left > 0; --triangles_left)
					{
						varying float3 v0 = load3(tree, node);
						node += 3;
						varying float3 v1 = load3(tree, node);
						node += 3;
						varying float3 v2 = load3(tree, node);
						node += 3;
						
						closest = min(closest, RayTriangle(origin, direction, v0, v1, v2));
					}				
				}
			}

			// Fetch next node
			node = stack[--stack_pos];
			if (node == 0)
				break;
		}

		// Store minimum distance
		distances[index] = closest;
	}
}
