#pragma once

#include <TriangleGrouper/TriangleGrouper.h>

// A class that groups triangles in batches of N
// Starts with centroid with lowest X coordinate and finds N closest centroids, repeat
// Time complexity: O(N^2)
class TriangleGrouperClosestCentroid : public TriangleGrouper
{
public:
	// Group a batch of triangles
	virtual void			Group(const VertexList &inVertices, const IndexedTriangleList &inTriangles, int inGroupSize, vector<uint> &outGroupedTriangleIndices);
};
