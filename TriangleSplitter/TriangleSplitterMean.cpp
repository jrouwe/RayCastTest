#include <pch.h> // IWYU pragma: keep

#include <TriangleSplitter/TriangleSplitterMean.h>

TriangleSplitterMean::TriangleSplitterMean(const VertexList &inVertices, const IndexedTriangleList &inTriangles) :
	TriangleSplitter(inVertices, inTriangles)
{
}

bool TriangleSplitterMean::Split(const Range &inTriangles, Range &outLeft, Range &outRight, uint &outDimension, float &outSplit)
{	
	// Calculate mean value for these triangles
	Vec3 mean = Vec3::sZero();
	for (uint t = inTriangles.mBegin; t < inTriangles.mEnd; ++t)
		mean += Vec3(mCentroids[mSortedTriangleIdx[t]]);
	mean *= 1.0f / inTriangles.Count();

	// Calculate deviation
	Vec3 deviation = Vec3::sZero();
	for (uint t = inTriangles.mBegin; t < inTriangles.mEnd; ++t)
	{
		Vec3 delta = Vec3(mCentroids[mSortedTriangleIdx[t]]) - mean;
		deviation += delta * delta;
	}
	deviation *= 1.0f / inTriangles.Count();

	// Calculate split plane
	outDimension = deviation.GetHighestComponentIndex();
	outSplit = mean[outDimension];

	return SplitInternal(inTriangles, outDimension, outSplit, outLeft, outRight);
}
