#pragma once

#include <Geometry/Triangle.h>
#include <Geometry/IndexedTriangle.h>
#include <Math/Mat44.h>

// Axis aligned box
class AABox
{
public:
	// Constructor
					AABox()												: mMin(Vec3::sReplicate(FLT_MAX)), mMax(Vec3::sReplicate(-FLT_MAX)) { }
					AABox(const Vec3 &inMin, const Vec3 &inMax)			: mMin(inMin), mMax(inMax) { }
					AABox(const Vec3 &inCenter, float inRadius)			: mMin(inCenter - Vec3::sReplicate(inRadius)), mMax(inCenter + Vec3::sReplicate(inRadius)) { }

	// Create box from 2 points
	static AABox	sFromTwoPoints(const Vec3 &inP1, const Vec3 &inP2)	{ return AABox(Vec3::sMin(inP1, inP2), Vec3::sMax(inP1, inP2)); }

	// Get bounding box of size 2 * FLT_MAX
	static AABox	sBiggest()
	{
		return AABox(Vec3::sReplicate(-FLT_MAX), Vec3::sReplicate(FLT_MAX));
	}

	// Reset the bounding box to an empty bounding box
	void			SetEmpty()
	{
		mMin = Vec3::sReplicate(FLT_MAX);
		mMax = Vec3::sReplicate(-FLT_MAX);
	}

	// Check if the bounding box is valid (max >= min)
	bool			IsValid() const
	{
		return mMin.GetX() <= mMax.GetX() && mMin.GetY() <= mMax.GetY() && mMin.GetZ() <= mMax.GetZ();
	}

	// Encapsulate point in bounding box
	void			Encapsulate(const Vec3 &inPos)						
	{ 
		mMin = Vec3::sMin(mMin, inPos); 
		mMax = Vec3::sMax(mMax, inPos); 
	}

	// Encapsulate bounding box in bounding box
	void			Encapsulate(const AABox &inRHS)			
	{ 
		mMin = Vec3::sMin(mMin, inRHS.mMin);
		mMax = Vec3::sMax(mMax, inRHS.mMax);
	}

	// Encapsulate triangle in bounding box
	void			Encapsulate(const Triangle &inRHS)
	{
		Vec3 v = Vec3::sLoadFloat3Unsafe(inRHS.mV[0]);
		Encapsulate(v);
		v = Vec3::sLoadFloat3Unsafe(inRHS.mV[1]);
		Encapsulate(v);
		v = Vec3::sLoadFloat3Unsafe(inRHS.mV[2]);
		Encapsulate(v);
	}

	// Encapsulate triangle in bounding box
	void			Encapsulate(const VertexList &inVertices, const IndexedTriangle &inTriangle)
	{
		for (uint32 idx : inTriangle.mIdx)
			Encapsulate(Vec3(inVertices[idx]));
	}

	// Intersect this bounding box with inOther, returns the intersection
	const AABox		Intersect(const AABox &inOther) const
	{
		return AABox(Vec3::sMax(mMin, inOther.mMin), Vec3::sMin(mMax, inOther.mMax));
	}

	// Make sure that each edge of the bounding box has a minimal length
	void			EnsureMinimalEdgeLength(float inMinEdgeLength)
	{
		Vec3 min_length = Vec3::sReplicate(inMinEdgeLength);
		mMax = Vec3::sSelect(mMax, mMin + min_length, Vec3::sLess(mMax - mMin, min_length));
	}
	
	// Widen the box by inFactor
	void			WidenByFactor(float inFactor)
	{
		Vec3 delta = (0.5f * inFactor) * (mMax - mMin);
		mMin -= delta;
		mMax += delta;
	}

	// Widen the box by with an constant vector
	void			WidenByConstant(const Vec3 &inVector)
	{
		Vec3 delta = 0.5f * inVector;
		mMin -= delta;
		mMax += delta;
	}

	// Get center of bounding box
	const Vec3		GetCenter() const
	{
		return 0.5f * (mMin + mMax);
	}

	// Get extent of bounding box (half of the size)
	const Vec3		GetExtent() const
	{
		return 0.5f * (mMax - mMin);
	}

	// Get size of bounding box
	const Vec3		GetSize() const
	{
		return mMax - mMin;
	}

	// Get surface area of bounding box
	float			GetSurfaceArea() const							
	{ 
		Vec3 extent = mMax - mMin;
		return 2.0f * (extent.GetX() * extent.GetY() + extent.GetX() * extent.GetZ() + extent.GetY() * extent.GetZ());
	}

	// Get volume of bounding box
	float			GetVolume() const
	{
		Vec3 extent = mMax - mMin;
		return extent.GetX() * extent.GetY() * extent.GetZ();
	}

	// Check if this box contains another box
	bool			Contains(const AABox &inOther) const
	{
		return UVec4::sAnd(Vec3::sLessOrEqual(mMin, inOther.mMin), Vec3::sGreaterOrEqual(mMax, inOther.mMax)).TestAllXYZTrue();
	}

	// Check if this box contains a point
	bool			Contains(const Vec3 &inOther) const
	{
		return UVec4::sAnd(Vec3::sLessOrEqual(mMin, inOther), Vec3::sGreaterOrEqual(mMax, inOther)).TestAllXYZTrue();
	}

	// Check if this box overlaps with another box
	bool			Overlaps(const AABox &inOther) const
	{
		return !UVec4::sOr(Vec3::sGreater(mMin, inOther.mMax), Vec3::sLess(mMax, inOther.mMin)).TestAnyXYZTrue();
	}

	// Translate bounding box
	void			Translate(const Vec3 &inTranslation)
	{
		mMin += inTranslation;
		mMax += inTranslation;
	}

	// Transform bounding box
	AABox			Transformed(const Mat44 &inMatrix) const
	{
		// Start with the translation of the matrix
		Vec3 new_min, new_max;
		new_min = new_max = inMatrix.GetTranslation();
		
		// Now find the extreme points by considering the product of the min and max with each column of inMatrix
		for (int c = 0; c < 3; ++c)
		{
			Vec3 col = inMatrix.GetColumn3(c);

			Vec3 a = col * mMin[c];
			Vec3 b = col * mMax[c];

			new_min += Vec3::sMin(a, b);
			new_max += Vec3::sMax(a, b);
		}

		// Return the new bounding box
		return AABox(new_min, new_max);
	}

	// Scale this bounding box, can handle non-uniform and negative scaling
	AABox			Scaled(const Vec3 &inScale) const
	{
		return AABox::sFromTwoPoints(mMin * inScale, mMax * inScale);
	}

	// Bounding box min and max
	Vec3			mMin;
	Vec3			mMax;
};
