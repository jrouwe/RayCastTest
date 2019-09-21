#pragma once

#include <Geometry/IndexedTriangle.h>
#include <Stripify/StripifyFlags.h>
#include <unordered_map>

// Non-conventional triangle stripping algorithm, instead of using the last 2 vertices to construct a new triangle any one of the three vertices from the last triangle can be used
class Stripify
{
public:
	struct Vertex
	{
		// Next vertex index
		uint32					mIndex;

		// Material for the next triangle triangle
		uint32					mMaterialIndex;

		// Flags from StripifyFlags.h that indicate how to make the next triangle
		uint32					mFlags;
	};

	using TriangleStrip = vector<Vertex>;

	// Convert indexed triangle list to stripped triangle list
	static void					sStripify(const IndexedTriangleList &inTriangles, TriangleStrip &outStrips);

	// Convert stripped triangle list back to indexed triangle list (example of how to decode the list)
	static void					sDeStripify(const TriangleStrip &inStrips, IndexedTriangleList &outTriangles);

private:
	// Edge structure
	struct Edge
	{
		// Constructor, make sure that lowest index is always mIdx[0]
								Edge()												{ } 
								Edge(uint32 inIdx1, uint32 inIdx2)					{ mIdx[0] = min(inIdx1, inIdx2); mIdx[1] = max(inIdx1, inIdx2); }

		bool					operator == (const Edge &inRHS) const				{ return mIdx[0] == inRHS.mIdx[0] && mIdx[1] == inRHS.mIdx[1]; }

		// Vertex indexes of the edge
		uint32					mIdx[2];
	};

	MAKE_HASH_STRUCT(Edge, EdgeHash, t.mIdx[0], t.mIdx[1])

	using EdgeToTriangles = unordered_map<Edge, vector<uint32>, EdgeHash>;

	// Extra data that we need to keep per triangle
	struct TriangleEx
	{
		// How many shared edges the triangle has
		uint					mSharedEdgeCount;

		// If this triangle has already been used in a strip
		bool					mUsedInStrip;
	};

	using TriangleExList = vector<TriangleEx>;
};
