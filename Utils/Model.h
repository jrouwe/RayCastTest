#pragma once

#include <Geometry/AABox.h>
#include <Geometry/Triangle.h>
#include <Geometry/IndexedTriangle.h>

// Simple model
class Model
{
public:
	struct ModelHeaderV1
	{
		static inline const uint32	sVersion = uint32('M') + (uint32('D') << 8) + (uint32('V') << 16) + (uint32('1') << 24);

									ModelHeaderV1() : mVersion(sVersion), mNumVertices(0), mNumTriangles(0) { }

		uint32						mVersion;
		uint32						mNumVertices;
		uint32						mNumTriangles;
	};

	// Read model file data
	void							ReadFromFile(const char *inFileName);

	// Get number of triangles in this model
	uint							GetTriangleCount() const
	{
		return (uint)mIndexedTriangles.size();
	}

	// Access a triangle
	const Triangle &				GetTriangle(uint inIdx) const
	{
		return mTriangles[inIdx];
	}

	// Triangles
	VertexList						mTriangleVertices;
	IndexedTriangleNoMaterialList	mIndexedTrianglesNoMaterial;
	IndexedTriangleList				mIndexedTriangles;
	TriangleList					mTriangles;

	// Bounding box
	AABox							mBounds;
};
