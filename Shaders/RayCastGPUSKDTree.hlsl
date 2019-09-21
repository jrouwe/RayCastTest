#include "RayCastGPUTree.h"
#include "RayAABox.h"

// Tree properties
static const uint HAS_TRIANGLES = 0x20000000;
static const uint PLANE_AXIS_SHIFT = 30;
static const uint MASK_OUT_HAS_TRIANGLES = 0x1fffffff;

bool GetHitFraction(float3 inOrigin, float3 inDirection, uint inAxis, float inCoordinate, float inSide, inout float outFractionNear, inout float outFractionFar)
{
	float dist_to_plane = inOrigin[inAxis] - inCoordinate;
	float direction = inDirection[inAxis];

	// Check if ray is parallel to plane
	if (direction > -1.0e-12f && direction < 1.0e-12f)
	{
		// Check if ray is on the right side of the plane
		return inSide * dist_to_plane >= 0.0f;
	}
	else
	{
		// Update fraction
		float intersection = -dist_to_plane / direction;
		if (inSide * direction > 0.0f)
			outFractionNear = max(outFractionNear, intersection);
		else
			outFractionFar = min(outFractionFar, intersection);

		// Return if there is still a possibility for a hit
		return outFractionNear <= outFractionFar;
	}
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
	uint node_stack[stack_size];
	float fraction_near_stack[stack_size];
	float fraction_far_stack[stack_size];
	node_stack[0] = -1;
	uint stack_pos = 1;

	uint node = 0;

	// Fetch mesh bounds
	float3 bounds_min = asfloat(tree.Load3(node));
	node += 12;
	float3 bounds_max = asfloat(tree.Load3(node));
	node += 12;

	// Calculate bounds for intersection
	float fraction_near, fraction_far;
	RayAABox(origin, inv_direction, is_parallel, bounds_min, bounds_max, fraction_near, fraction_far);

	while (node != -1)
	{
		if (fraction_near <= fraction_far)
		{
			// Fetch node properties
			uint node_props = tree.Load(node);
			node += 4;

			// Test if node contains triangles
			if ((node_props & HAS_TRIANGLES) == 0)
			{
				uint left_child = node + 8;
				uint right_child = node - 4 + (node_props & MASK_OUT_HAS_TRIANGLES);

				// Get separating plane and left and right value
				uint plane_axis = node_props >> PLANE_AXIS_SHIFT;
				float left_plane = asfloat(tree.Load(node));
				node += 4;
				float right_plane = asfloat(tree.Load(node));
				node += 4;

				float left_fraction_near = fraction_near, left_fraction_far = fraction_far;
				bool left_intersects = GetHitFraction(origin, direction, plane_axis, left_plane, -1.0f, left_fraction_near, left_fraction_far);

				float right_fraction_near = fraction_near, right_fraction_far = fraction_far;
				bool right_intersects = GetHitFraction(origin, direction, plane_axis, right_plane, 1.0f, right_fraction_near, right_fraction_far);
				
				if (left_intersects && right_intersects)
				{
					// Both collide
					if (left_fraction_near < right_fraction_near)
					{
						// Left child before right child
						node_stack[stack_pos] = right_child;
						fraction_near_stack[stack_pos] = right_fraction_near;
						fraction_far_stack[stack_pos++] = right_fraction_far;
						node = left_child;
						fraction_near = left_fraction_near;
						fraction_far = left_fraction_far;
					}
					else
					{
						// Right child before left child
						node_stack[stack_pos] = left_child;
						fraction_near_stack[stack_pos] = left_fraction_near;
						fraction_far_stack[stack_pos++] = left_fraction_far;
						node = right_child;
						fraction_near = right_fraction_near;
						fraction_far = right_fraction_far;
					}
					continue;
				}
				else if (left_intersects)
				{
					// Only left collides
					node = left_child;
					fraction_near = left_fraction_near;
					fraction_far = left_fraction_far;
					continue;
				}
				else if (right_intersects)
				{
					// Only right collides
					node = right_child;
					fraction_near = right_fraction_near;
					fraction_far = right_fraction_far;
					continue;
				}
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
		node = node_stack[--stack_pos];
		fraction_near = fraction_near_stack[stack_pos];
		fraction_far = min(closest, fraction_far_stack[stack_pos]);
	}

	// Store minimum distance
	output[tid.x] = closest;
}
