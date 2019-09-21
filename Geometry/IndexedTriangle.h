#pragma once

// Triangle with 32-bit indices
class IndexedTriangleNoMaterial
{
public:
	// Constructor
					IndexedTriangleNoMaterial()											{ }
					IndexedTriangleNoMaterial(uint32 inI1, uint32 inI2, uint32 inI3)	{ mIdx[0] = inI1; mIdx[1] = inI2; mIdx[2] = inI3; }

	// Check if two triangles are identical
	bool			operator == (const IndexedTriangleNoMaterial &inRHS) const
	{
		return mIdx[0] == inRHS.mIdx[0] && mIdx[1] == inRHS.mIdx[1] && mIdx[2] == inRHS.mIdx[2];
	}

	// Check if two triangles are equivalent (using the same vertices)
	bool			IsEquivalent(const IndexedTriangleNoMaterial &inRHS) const
	{
		return (mIdx[0] == inRHS.mIdx[0] && mIdx[1] == inRHS.mIdx[1] && mIdx[2] == inRHS.mIdx[2])
			|| (mIdx[0] == inRHS.mIdx[1] && mIdx[1] == inRHS.mIdx[2] && mIdx[2] == inRHS.mIdx[0])
			|| (mIdx[0] == inRHS.mIdx[2] && mIdx[1] == inRHS.mIdx[0] && mIdx[2] == inRHS.mIdx[1]);
	}

	// Check if two triangles are opposite (using the same vertices but in opposing order)
	bool			IsOpposite(const IndexedTriangleNoMaterial &inRHS) const
	{
		return (mIdx[0] == inRHS.mIdx[0] && mIdx[1] == inRHS.mIdx[2] && mIdx[2] == inRHS.mIdx[1])
			|| (mIdx[0] == inRHS.mIdx[1] && mIdx[1] == inRHS.mIdx[0] && mIdx[2] == inRHS.mIdx[2])
			|| (mIdx[0] == inRHS.mIdx[2] && mIdx[1] == inRHS.mIdx[1] && mIdx[2] == inRHS.mIdx[0]);
	}

	// Check if triangle is degenerate
	bool			IsDegenerate() const
	{
		return mIdx[0] == mIdx[1] || mIdx[1] == mIdx[2] || mIdx[2] == mIdx[0];
	}

	// Rotate the vertices so that the second vertex becomes first etc. This does not change the represented triangle
	void			Rotate()
	{
		uint32 tmp = mIdx[0];
		mIdx[0] = mIdx[1];
		mIdx[1] = mIdx[2];
		mIdx[2] = tmp;
	}

	// Get center of triangle
	Vec3			GetCentroid(const VertexList &inVertices) const
	{
		return (Vec3(inVertices[mIdx[0]]) + Vec3(inVertices[mIdx[1]]) + Vec3(inVertices[mIdx[2]])) / 3.0f;
	}

	uint32			mIdx[3];
};

// Triangle with 32-bit indices and material index
class IndexedTriangle : public IndexedTriangleNoMaterial
{
public:
	// Constructor
					IndexedTriangle() = default;
					IndexedTriangle(uint32 inI1, uint32 inI2, uint32 inI3) : IndexedTriangleNoMaterial(inI1, inI2, inI3) { }
					IndexedTriangle(uint32 inI1, uint32 inI2, uint32 inI3, uint32 inMaterialIndex) : IndexedTriangleNoMaterial(inI1, inI2, inI3), mMaterialIndex(inMaterialIndex) { }

	// Check if two triangles are identical
	bool			operator == (const IndexedTriangle &inRHS) const
	{
		return mMaterialIndex == inRHS.mMaterialIndex && IndexedTriangleNoMaterial::operator==(inRHS);
	}

	uint32			mMaterialIndex = 0;
};

using IndexedTriangleNoMaterialList = vector<IndexedTriangleNoMaterial>;
using IndexedTriangleList = vector<IndexedTriangle>;
