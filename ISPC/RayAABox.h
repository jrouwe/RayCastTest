#include "Math.h"

// Intersect AABB with ray, returns minimal distance along ray or FLT_MAX if no hit
inline float RayAABox(float3 inOrigin, float3 inDirection, float3 inBoundsMin, float3 inBoundsMax)
{
	// if (abs(inDirection) <= Epsilon) the ray is nearly parallel to the slab.
	bool3 is_parallel = abs(inDirection) <= epsilon3;

	// Test against all three axii simultaneously.
	float3 inv_direction = 1.0f / inDirection;
	float3 t1 = (inBoundsMin - inOrigin) * inv_direction;
	float3 t2 = (inBoundsMax - inOrigin) * inv_direction;

	// Compute the max of min(t1,t2) and the min of max(t1,t2) ensuring we don't
	// use the results from any directions parallel to the slab.
	float3 t_min = select(is_parallel, flt_min3, min(t1, t2));
	float3 t_max = select(is_parallel, flt_max3, max(t1, t2));

	// Get min and max
	float minimum = max(max(t_min.x, t_min.y), t_min.z);
	float maximum = min(min(t_max.x, t_max.y), t_max.z);

	// Early out
	if (minimum > maximum || maximum < 0.0f)
		return FLT_MAX;

	// if (is_parallel && !(Min <= inOrigin && inOrigin <= Max)) return FLT_MAX; else return t_min.x;
	bool3 no_parallel_overlap = or(inOrigin < inBoundsMin, inOrigin > inBoundsMax);
	return any(and(is_parallel, no_parallel_overlap))? FLT_MAX : minimum;
}
