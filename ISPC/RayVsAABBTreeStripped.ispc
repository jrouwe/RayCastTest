#include "RayTriangle.h"
#include "RayAABox.h"
#include "../Shaders/StripifyFlags.h"

// Tree properties
static const uint HAS_TRIANGLES = 0x80000000;
static const uint MASK_OUT_HAS_TRIANGLES = 0x7fffffff;

struct RayCast
{
	float		Origin[3];
	float		Direction[3];
};

inline uint get_flags(float3 inVertex)
{
	uint3 u3 = asuint(inVertex) & uint3_one;
	return u3.x + (u3.y << 1) + (u3.z << 2);
}

inline float3 remove_flags(float3 inVertex)
{
	return asfloat(asuint(inVertex) & uint3_all_but_lowest_bit_set);
}

inline float3 load3(const uniform uint tree[], varying uint node)
{
	float3 ret = { floatbits(tree[node + 0]), floatbits(tree[node + 1]), floatbits(tree[node + 2]) };
	return ret;	
}

export void RayVsAABBTreeStripped(const uniform RayCast raycasts[], const uniform uint num_rays, const uniform uint tree[], uniform float distances[])
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
			// Skip node bounds
			node += 6;

			// Fetch node properties
			varying uint node_props = tree[node];
			++node;
		
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
				varying float3 v[3];
				for (varying uint triangles_left = (node_props & MASK_OUT_HAS_TRIANGLES); triangles_left > 0; --triangles_left)
				{
					varying float3 next = load3(tree, node);
					node += 3;

					varying uint flags = get_flags(next);
					next = remove_flags(next);

					if (flags == STRIPIFY_FLAG_START_STRIP_V1)
					{
						// Start of new strip, load first 2 vertices
						v[0] = next;
						v[1] = remove_flags(load3(tree, node));
						node += 3;
						next = remove_flags(load3(tree, node));
						node += 3;
					}
					else
					{
						// Continuation of strip, determine first 2 vertices
						varying uint a = flags & STRIPIFY_FLAG_A_MASK;
						varying uint b = a + ((flags & STRIPIFY_FLAG_B_MASK) >> STRIPIFY_FLAG_B_SHIFT) + 1;
						varying float3 va = v[a];
						varying float3 vb = v[b > 2? b - 3 : b];
						v[0] = va;
						v[1] = vb;
					}

					// Load third vertex
					v[2] = next;

					closest = min(closest, RayTriangle(origin, direction, v[0], v[1], v[2]));
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
