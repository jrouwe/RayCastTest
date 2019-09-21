#pragma once

#include <Geometry/IndexedTriangle.h>

// A class that groups triangles in batches of N (according to closeness)
class TriangleGrouper
{
public:
	// Virtual destructor
	virtual					~TriangleGrouper() { }

	// Group a batch of triangles
	virtual void			Group(const VertexList &inVertices, const IndexedTriangleList &inTriangles, int inGroupSize, vector<uint> &outGroupedTriangleIndices) = 0;
};
