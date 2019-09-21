#pragma once

#include <TriangleSplitter/TriangleSplitter.h>
#include <Geometry/AABox.h>

// Same as TriangleSplitterBinning, but ensuring that leaves have a fixed amount of triangles
// The resulting tree should be suitable for processing on GPU where we want all threads to process an equal amount of triangles
class TriangleSplitterFixedLeafSize : public TriangleSplitter
{
public:
	// Constructor
							TriangleSplitterFixedLeafSize(const VertexList &inVertices, const IndexedTriangleList &inTriangles, uint inLeafSize, uint inMinNumBins = 8, uint inMaxNumBins = 128, uint inNumTrianglesPerBin = 6);

	// Get stats of splitter
	virtual void			GetStats(Stats &outStats) const override
	{
		outStats.mSplitterName = "TriangleSplitterFixedLeafSize";
		outStats.mLeafSize = mLeafSize;
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

private:
	// Get centroid for group
	Vec3					GetCentroidForGroup(uint inFirstTriangleInGroup);
	
	// Configuration
	const uint				mLeafSize;
	const uint				mMinNumBins;
	const uint				mMaxNumBins;
	const uint				mNumTrianglesPerBin;
	
	struct Bin
	{
		// Properties of this bin
		AABox				mBounds;
		float				mMinCentroid;
		uint				mNumTriangles;

		// Accumulated data from left most / right most bin to current (including this bin)
		AABox				mBoundsAccumulatedLeft;				
		AABox				mBoundsAccumulatedRight;			
		uint				mNumTrianglesAccumulatedLeft;		
		uint				mNumTrianglesAccumulatedRight;		
	};
};
