#pragma once

#include <TriangleGrouper/TriangleGrouper.h>

// A class that groups triangles in batches of N 
// According to morton code of centroid
// Time complexity: O(N log(N))
class TriangleGrouperMorton : public TriangleGrouper
{
public:
	// Group a batch of triangles
	virtual void			Group(const VertexList &inVertices, const IndexedTriangleList &inTriangles, int inGroupSize, vector<uint> &outGroupedTriangleIndices);
};
