#include "RayTriangle.h"

#ifndef group_size
	#define group_size 64
#endif
#ifndef stack_size
	#define stack_size 64
#endif

struct RayCastTestIn
{
	float3		Origin;
	float3		Direction;
};

StructuredBuffer<RayCastTestIn> raycasts : register(t0);

ByteAddressBuffer tree : register(t1);

RWStructuredBuffer<float> output : register(u0);
