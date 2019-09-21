#pragma once

#include <Geometry/IndexedTriangle.h>

// A class that splits a triangle list into two parts for building a tree
class TriangleSplitter
{
public:
	// Constructor
								TriangleSplitter(const VertexList &inVertices, const IndexedTriangleList &inTriangles);

	// Virtual destructor
	virtual						~TriangleSplitter() { }

	struct Stats
	{
		const char *			mSplitterName = nullptr;
		int						mLeafSize = 0;
	};

	// Get stats of splitter
	virtual void				GetStats(Stats &outStats) const = 0;

	// Helper struct to indicate triangle range before and after the split
	struct Range
	{
		// Constructor
								Range() { }
								Range(uint inBegin, uint inEnd) : mBegin(inBegin), mEnd(inEnd) { }

		// Get number of triangles in range
		uint					Count() const 
		{ 
			return mEnd - mBegin;
		}

		// Start and end index (end = 1 beyond end)
		uint					mBegin;
		uint					mEnd;
	};

	// Range of triangles to start with
	Range						GetInitialRange() const 
	{ 
		return Range(0, (uint)mSortedTriangleIdx.size()); 
	}

	// If the Split function returns a proper outDimension and outSplit
	virtual bool				CalculatesSplitDimension() const = 0;

	// Split triangles into two groups left and right, returns false if no split could be made
	// outDimension is the axis along which the split was made (0 = x, 1 = y, 2 = z) and outSplit is the value at which the split was made
	// Note that this is only available when CalculatesSplitDimension() returns true
	virtual bool				Split(const Range &inTriangles, Range &outLeft, Range &outRight, uint &outDimension, float &outSplit) = 0;

	// Get the list of vertices
	const VertexList &			GetVertices() const
	{
		return mVertices;
	}

	// Get triangle by index
	const IndexedTriangle &		GetTriangle(uint inIdx) const
	{
		return mTriangles[mSortedTriangleIdx[inIdx]];
	}

protected:
	// Helper function to split triangles based on dimension and split value
	bool						SplitInternal(const Range &inTriangles, uint inDimension, float inSplit, Range &outLeft, Range &outRight);

	const VertexList &			mVertices;				// Vertices of the indexed triangles
	const IndexedTriangleList &	mTriangles;				// Unsorted triangles
	vector<Float3>				mCentroids;				// Unsorted centroids of triangles
	vector<uint>				mSortedTriangleIdx;		// Indices to sort triangles
};
