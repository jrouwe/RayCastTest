#include <pch.h> // IWYU pragma: keep

#include <TriangleSplitter/TriangleSplitterLongestAxis.h>
#include <Geometry/AABox.h>

TriangleSplitterLongestAxis::TriangleSplitterLongestAxis(const VertexList &inVertices, const IndexedTriangleList &inTriangles) :
	TriangleSplitter(inVertices, inTriangles)
{
}

bool TriangleSplitterLongestAxis::Split(const Range &inTriangles, Range &outLeft, Range &outRight, uint &outDimension, float &outSplit)
{	
	// Calculate bounding box for triangles
	AABox bounds;
	for (uint t = inTriangles.mBegin; t < inTriangles.mEnd; ++t)
		bounds.Encapsulate(mVertices, GetTriangle(t));

	// Calculate split plane
	outDimension = bounds.GetExtent().GetHighestComponentIndex();
	outSplit = bounds.GetCenter()[outDimension];

	return SplitInternal(inTriangles, outDimension, outSplit, outLeft, outRight);
}
