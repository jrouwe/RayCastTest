#pragma once

#include <TriangleSplitter/TriangleSplitter.h>

// Splitter using center of bounding box with longest axis
class TriangleSplitterLongestAxis : public TriangleSplitter
{
public:
	// Constructor
							TriangleSplitterLongestAxis(const VertexList &inVertices, const IndexedTriangleList &inTriangles);

	// Get stats of splitter
	virtual void			GetStats(Stats &outStats) const override
	{
		outStats.mSplitterName = "TriangleSplitterLongestAxis";
	}

	// If the Split function returns a proper outDimension and outSplit
	virtual bool			CalculatesSplitDimension() const override
	{ 
		return true; 
	}

	// Split triangles into two groups left and right, returns false if no split could be made
	// outDimension is the axis along which the split was made (0 = x, 1 = y, 2 = z) and outSplit is the value at which the split was made
	// Note that this is only available when CalculatesSplitDimension() returns true
	virtual bool			Split(const Range &inTriangles, Range &outLeft, Range &outRight, uint &outDimension, float &outSplit) override;
};
