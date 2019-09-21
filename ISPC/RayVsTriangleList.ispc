#include "RayTriangle.h"

export uniform float RayVsTriangleList(const uniform float origin[3], const uniform float direction[3], const uniform float triangles[], uniform int num_triangles)
{
	uniform float minimum = FLT_MAX;

	uniform const float3 f3_origin = { origin[0], origin[1], origin[2] };
	uniform const float3 f3_direction = { direction[0], direction[1], direction[2] };

	const uniform float * uniform v1x = triangles;
	const uniform float * uniform v1y = v1x + num_triangles;
	const uniform float * uniform v1z = v1y + num_triangles;
	const uniform float * uniform v2x = v1z + num_triangles;
	const uniform float * uniform v2y = v2x + num_triangles;
	const uniform float * uniform v2z = v2y + num_triangles;
	const uniform float * uniform v3x = v2z + num_triangles;
	const uniform float * uniform v3y = v3x + num_triangles;
	const uniform float * uniform v3z = v3y + num_triangles;
	
	foreach (index = 0 ... num_triangles) 
	{
		varying float3 v1 = { v1x[index], v1y[index], v1z[index] };
		varying float3 v2 = { v2x[index], v2y[index], v2z[index] };
		varying float3 v3 = { v3x[index], v3y[index], v3z[index] };

		minimum = min(minimum, reduce_min(RayTriangle(f3_origin, f3_direction, v1, v2, v3)));
	}

	return minimum;
}
