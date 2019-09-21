#include <pch.h> // IWYU pragma: keep

#include <TriangleSplitter/TriangleSplitterMorton.h>
#include <Geometry/MortonCode.h>

TriangleSplitterMorton::TriangleSplitterMorton(const VertexList &inVertices, const IndexedTriangleList &inTriangles) :
	TriangleSplitter(inVertices, inTriangles)
{
	// Calculate bounds of centroids
	AABox bounds;
	for (uint t = 0; t < inTriangles.size(); ++t)
		bounds.Encapsulate(Vec3(mCentroids[t]));

	// Make sure box is not degenerate
	bounds.EnsureMinimalEdgeLength(1.0e-5f);

	// Calculate morton codes
	mMortonCodes.resize(inTriangles.size());
	for (uint t = 0; t < inTriangles.size(); ++t)
		mMortonCodes[t] = MortonCode::sGetMortonCode(Vec3(mCentroids[t]), bounds);

	// Sort triangles on morton code
	const vector<uint32> &morton_codes = mMortonCodes;
	sort(mSortedTriangleIdx.begin(), mSortedTriangleIdx.end(), [&morton_codes](uint inLHS, uint inRHS) -> bool { return morton_codes[inLHS] < morton_codes[inRHS]; });
}

bool TriangleSplitterMorton::Split(const Range &inTriangles, Range &outLeft, Range &outRight, uint &outDimension, float &outSplit)
{	
	uint32 first_code = mMortonCodes[mSortedTriangleIdx[inTriangles.mBegin]];
	uint32 last_code = mMortonCodes[mSortedTriangleIdx[inTriangles.mEnd - 1]];

	uint common_prefix = CountLeadingZeros(first_code ^ last_code);

	// Use binary search to find where the next bit differs
	uint split = inTriangles.mBegin; // Initial guess    
	uint step = inTriangles.Count();
	do    
	{
		step = (step + 1) >> 1; // Exponential decrease        
		uint new_split = split + step; // Proposed new position
		if (new_split < inTriangles.mEnd)
		{
			uint32 split_code = mMortonCodes[mSortedTriangleIdx[new_split]];
			uint split_prefix = CountLeadingZeros(first_code ^ split_code);
			if (split_prefix > common_prefix)
				split = new_split; // Accept proposal        
		}
	}
	while (step > 1);

	// Cannot calculate this
	outDimension = (uint)-1;
	outSplit = 0.0f;

	outLeft = Range(inTriangles.mBegin, split + 1);
	outRight = Range(split + 1, inTriangles.mEnd);
	return outLeft.Count() > 0 && outRight.Count() > 0;
}
