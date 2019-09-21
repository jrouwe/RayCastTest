#include "RayTriangle.h"

#define STORE_PER_TRIANGLE 0
#define STORE_PER_VECTOR 1
#define STORE_PER_COMPONENT 2

// These can be supplied as parameter
#ifndef variant
	#define variant STORE_PER_TRIANGLE
#endif
#ifndef num_triangles_per_thread
	#define num_triangles_per_thread 1
#endif
#ifndef group_size
	#define group_size 128
#endif

struct RayCast
{
	float3		Origin;
	float3		Direction;
};

StructuredBuffer<RayCast> rays : register(t1);

// Can't do atomics on float, so using int here (which will be fine as long as Distance > 0)
RWStructuredBuffer<int> output : register(u0);

#if variant == STORE_PER_TRIANGLE

struct Triangle
{
	float3		V0;
	float3		V1;
	float3		V2;
};

StructuredBuffer<Triangle> vertices : register(t0);

#elif variant == STORE_PER_VECTOR

StructuredBuffer<float3> vertices : register(t0);

#elif variant == STORE_PER_COMPONENT

StructuredBuffer<float> vertices : register(t0);

float3 GetVertex(uint idx)
{
	float3 v;
	v.x = vertices[idx];
	v.y = vertices[idx + group_size];
	v.z = vertices[idx + 2 * group_size];
	return v;
}

#endif

[numthreads(group_size,1,1)]
void main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
	// Get ray origin and direction
	float3 origin = rays[gid.y].Origin;
	float3 direction = rays[gid.y].Direction;

#if num_triangles_per_thread == 1

	#if variant == STORE_PER_TRIANGLE

		// Calculate start of triangle data
		uint idx = gid.x * group_size + gtid.x;

		// Fetch triangle vertices
		float3 v0 = vertices[idx].V0;
		float3 v1 = vertices[idx].V1;
		float3 v2 = vertices[idx].V2;

	#elif variant == STORE_PER_VECTOR

		// Calculate start of triangle data
		uint idx = gid.x * 3 * group_size + gtid.x;

		// Fetch triangle vertices
		float3 v0 = vertices[idx];
		float3 v1 = vertices[idx + group_size];
		float3 v2 = vertices[idx + 2 * group_size];

	#elif variant == STORE_PER_COMPONENT

		// Calculate start of triangle data
		uint idx = gid.x * 9 * group_size + gtid.x;

		// Fetch triangle vertices
		float3 v0 = GetVertex(idx);
		float3 v1 = GetVertex(idx + 3 * group_size);
		float3 v2 = GetVertex(idx + 6 * group_size);

	#endif

	// Calculate collision distance
	float min_dist = RayTriangle(origin, direction, v0, v1, v2);

#elif num_triangles_per_thread == -4

	#if variant == STORE_PER_TRIANGLE
	
		// Calculate start of triangle data
		uint idx = gid.x * -num_triangles_per_thread * group_size + gtid.x;

		// Fetch triangle vertices
		float3 v01 = vertices[idx].V0;
		float3 v11 = vertices[idx].V1;
		float3 v21 = vertices[idx].V2;

		float3 v02 = vertices[idx + group_size].V0;
		float3 v12 = vertices[idx + group_size].V1;
		float3 v22 = vertices[idx + group_size].V2;

		float3 v03 = vertices[idx + 2 * group_size].V0;
		float3 v13 = vertices[idx + 2 * group_size].V1;
		float3 v23 = vertices[idx + 2 * group_size].V2;

		float3 v04 = vertices[idx + 3 * group_size].V0;
		float3 v14 = vertices[idx + 3 * group_size].V1;
		float3 v24 = vertices[idx + 3 * group_size].V2;

	#elif variant == STORE_PER_VECTOR

		// Calculate start of triangle data
		uint idx = gid.x * 3 * -num_triangles_per_thread * group_size + gtid.x;

		// Fetch triangle vertices
		float3 v01 = vertices[idx + (3 * 0 + 0) * group_size];
		float3 v11 = vertices[idx + (3 * 0 + 1) * group_size];
		float3 v21 = vertices[idx + (3 * 0 + 2) * group_size];

		float3 v02 = vertices[idx + (3 * 1 + 0) * group_size];
		float3 v12 = vertices[idx + (3 * 1 + 1) * group_size];
		float3 v22 = vertices[idx + (3 * 1 + 2) * group_size];

		float3 v03 = vertices[idx + (3 * 2 + 0) * group_size];
		float3 v13 = vertices[idx + (3 * 2 + 1) * group_size];
		float3 v23 = vertices[idx + (3 * 2 + 2) * group_size];

		float3 v04 = vertices[idx + (3 * 3 + 0) * group_size];
		float3 v14 = vertices[idx + (3 * 3 + 1) * group_size];
		float3 v24 = vertices[idx + (3 * 3 + 2) * group_size];

	#elif variant == STORE_PER_COMPONENT

		// Calculate start of triangle data
		uint idx = gid.x * 9 * -num_triangles_per_thread * group_size + gtid.x;

		// Fetch triangle vertices
		float3 v01 = GetVertex(idx + (9 * 0 + 0) * group_size);
		float3 v11 = GetVertex(idx + (9 * 0 + 3) * group_size);
		float3 v21 = GetVertex(idx + (9 * 0 + 6) * group_size);

		float3 v02 = GetVertex(idx + (9 * 1 + 0) * group_size);
		float3 v12 = GetVertex(idx + (9 * 1 + 3) * group_size);
		float3 v22 = GetVertex(idx + (9 * 1 + 6) * group_size);

		float3 v03 = GetVertex(idx + (9 * 2 + 0) * group_size);
		float3 v13 = GetVertex(idx + (9 * 2 + 3) * group_size);
		float3 v23 = GetVertex(idx + (9 * 2 + 6) * group_size);

		float3 v04 = GetVertex(idx + (9 * 3 + 0) * group_size);
		float3 v14 = GetVertex(idx + (9 * 3 + 3) * group_size);
		float3 v24 = GetVertex(idx + (9 * 3 + 6) * group_size);

	#endif
	
	// Do 4 ray triangle tests
	float min_dist = RayTriangle4(origin, direction, v01, v11, v21, v02, v12, v22, v03, v13, v23, v04, v14, v24);

#else

	#if variant == STORE_PER_TRIANGLE

		// Calculate start of triangle data
		uint idx = gid.x * num_triangles_per_thread * group_size + gtid.x;

	#elif variant == STORE_PER_VECTOR

		// Calculate start of triangle data
		uint idx = gid.x * 3 * num_triangles_per_thread * group_size + gtid.x;

	#elif variant == STORE_PER_COMPONENT

		// Calculate start of triangle data
		uint idx = gid.x * 9 * num_triangles_per_thread * group_size + gtid.x;

	#endif

	float min_dist = FLT_MAX;

	[unroll(16)]
	for (int i = 0; i < num_triangles_per_thread; ++i)
	{
	#if variant == STORE_PER_TRIANGLE

		// Fetch triangle vertices
		float3 v0 = vertices[idx + i * group_size].V0;
		float3 v1 = vertices[idx + i * group_size].V1;
		float3 v2 = vertices[idx + i * group_size].V2;

	#elif variant == STORE_PER_VECTOR

		// Fetch triangle vertices
		float3 v0 = vertices[idx + (3 * i) * group_size];
		float3 v1 = vertices[idx + (3 * i + 1) * group_size];
		float3 v2 = vertices[idx + (3 * i + 2) * group_size];

	#elif variant == STORE_PER_COMPONENT

		// Fetch triangle vertices
		float3 v0 = GetVertex(idx + (9 * i) * group_size);
		float3 v1 = GetVertex(idx + (9 * i + 3) * group_size);
		float3 v2 = GetVertex(idx + (9 * i + 6) * group_size);

	#endif

		// Calculate collision distance
		float distance = RayTriangle(origin, direction, v0, v1, v2);

		// Calculate minimum
		min_dist = min(distance, min_dist);
	}

#endif

	// Store minimum
	if (min_dist < FLT_MAX)
		InterlockedMin(output[gid.y], asint(min_dist));
}
