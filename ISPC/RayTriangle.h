#include "Math.h"

// Taken from: http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
inline float RayTriangle(float3 O, float3 D, float3 V0, float3 V1, float3 V2)
{
	// Find vectors for two edges sharing V0
	float3 e1 = V1 - V0;
	float3 e2 = V2 - V0;

	// Begin calculating determinant - also used to calculate u parameter
	float3 p = cross(D, e2);

	// If determinant is near zero, ray lies in plane of triangle
	float det = dot(e1, p);
	if (abs(det) < EPSILON)
		return FLT_MAX;
	float inv_det = rcp(det);

	// Calculate distance from V0 to ray origin
	float3 s = O - V0;

	// Calculate u parameter 
	float u = dot(s, p) * inv_det;
	if (u < 0)
		return FLT_MAX;

	// Calculate v parameter
	float3 q = cross(s, e1);
	float v = dot(D, q) * inv_det;
	if (v < 0 || u + v > 1.0f)
		return FLT_MAX;

	// Get intersection point 
	float t = dot(e2, q) * inv_det;
	if (t < 0)
		return FLT_MAX;

	// Return collision distance
	return t;
}