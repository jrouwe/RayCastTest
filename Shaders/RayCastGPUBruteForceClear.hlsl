#include "Constants.h"

#ifndef group_size
	#define group_size 128
#endif

RWStructuredBuffer<float> output : register(u0);

[numthreads(group_size,1,1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	// Initialize memory
	output[dtid.x] = FLT_MAX;
}
