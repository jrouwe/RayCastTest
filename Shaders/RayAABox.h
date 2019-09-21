#include "Constants.h"

// Test if ray is parallel to the coordinate axis
inline bool3 RayIsParallel(float3 inDirection)
{
	// if (abs(inDirection) <= Epsilon) the ray is nearly parallel to the slab.
	return abs(inDirection) <= epsilon3;
}

// Intersect AABB with ray, returns minimal distance along ray or FLT_MAX if no hit
inline float RayAABox(float3 inOrigin, float3 inInvDirection, bool3 inIsParallel, float3 inBoundsMin, float3 inBoundsMax)
{
	// Test against all three axii simultaneously.
	float3 t1 = (inBoundsMin - inOrigin) * inInvDirection;
	float3 t2 = (inBoundsMax - inOrigin) * inInvDirection;

	// Compute the max of min(t1,t2) and the min of max(t1,t2) ensuring we don't
	// use the results from any directions parallel to the slab.
	float3 t_min = inIsParallel? flt_min3 : min(t1, t2);
	float3 t_max = inIsParallel? flt_max3 : max(t1, t2);

	// t_min.xyz = maximum(t_min.x, t_min.y, t_min.z);
	t_min = max(t_min, t_min.yyy);
	t_min = max(t_min, t_min.zzz);
	t_min = t_min.xxx;

	// t_max.xyz = minimum(t_max.x, t_max.y, t_max.z);
	t_max = min(t_max, t_max.yyy);
	t_max = min(t_max, t_max.zzz);
	t_max = t_max.xxx;

	// if (t_min > t_max) return FLT_MAX;
	bool3 no_intersection = t_min > t_max;

	// if (t_max < 0.0f) return FLT_MAX;
	no_intersection = no_intersection || (t_max < zero3);

	// if (inIsParallel && !(Min <= inOrigin && inOrigin <= Max)) return FLT_MAX; else return t_min.x;
	bool3 no_parallel_overlap = (inOrigin < inBoundsMin) || (inOrigin > inBoundsMax);
	no_intersection = no_intersection || (inIsParallel && no_parallel_overlap);
	return any(no_intersection)? FLT_MAX : t_min.x;
}

inline void RayAABox(float3 inOrigin, float3 inInvDirection, bool3 inIsParallel, float3 inBoundsMin, float3 inBoundsMax, out float outMin, out float outMax)
{
	// Test against all three axii simultaneously.
	float3 t1 = (inBoundsMin - inOrigin) * inInvDirection;
	float3 t2 = (inBoundsMax - inOrigin) * inInvDirection;

	// Compute the max of min(t1,t2) and the min of max(t1,t2) ensuring we don't
	// use the results from any directions parallel to the slab.
	float3 t_min = inIsParallel? flt_min3 : min(t1, t2);
	float3 t_max = inIsParallel? flt_max3 : max(t1, t2);

	// t_min.xyz = maximum(t_min.x, t_min.y, t_min.z);
	t_min = max(t_min, t_min.yyy);
	t_min = max(t_min, t_min.zzz);
	t_min = t_min.xxx;

	// t_max.xyz = minimum(t_max.x, t_max.y, t_max.z);
	t_max = min(t_max, t_max.yyy);
	t_max = min(t_max, t_max.zzz);
	t_max = t_max.xxx;

	// if (t_min > t_max) return FLT_MAX;
	bool3 no_intersection = t_min > t_max;

	// if (t_max < 0.0f) return FLT_MAX;
	no_intersection = no_intersection || (t_max < zero3);

	// if (inIsParallel && !(Min <= inOrigin && inOrigin <= Max)) return FLT_MAX; else return t_min.x;
	bool3 no_parallel_overlap = (inOrigin < inBoundsMin) || (inOrigin > inBoundsMax);
	no_intersection = no_intersection || (inIsParallel && no_parallel_overlap);
	outMin = any(no_intersection)? FLT_MAX : t_min.x;
	outMax = any(no_intersection)? -FLT_MAX : t_max.x;
}
