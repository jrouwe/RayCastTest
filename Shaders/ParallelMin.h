groupshared float parallel_min[group_size]; 

inline float ParallelMin(uint inThreadID, float inValue)
{
	parallel_min[inThreadID] = inValue;

	// Wait until all parallel_min have been calculated
	GroupMemoryBarrierWithGroupSync();

	// Do reduction in shared memory (see: http://www.nvidia.com/content/GTC-2010/pdfs/2260_GTC2010.pdf)
	[unroll(10)]
	for (uint i = group_size / 2; i > 1; i >>= 1) 
	{
		if (inThreadID < i)
			parallel_min[inThreadID] = min(parallel_min[inThreadID], parallel_min[inThreadID + i]);

		GroupMemoryBarrierWithGroupSync();
	}

	// Return closest distance of group
	return min(parallel_min[0], parallel_min[1]);
}