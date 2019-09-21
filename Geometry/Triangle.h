#pragma once

#include <Geometry/RayTriangle.h>

// Triangle 
class Triangle
{
public:
	// Constructor
					Triangle() { }
					Triangle(const Float3 &inV1, const Float3 &inV2, const Float3 &inV3) : mV { inV1, inV2, inV3 } { }
					Triangle(const Float3 &inV1, const Float3 &inV2, const Float3 &inV3, uint32 inMaterialIndex) : Triangle(inV1, inV2, inV3) { mMaterialIndex = inMaterialIndex; }
					Triangle(const Vec3 &inV1, const Vec3 &inV2, const Vec3 &inV3) { inV1.StoreFloat3(&mV[0]); inV2.StoreFloat3(&mV[1]); inV3.StoreFloat3(&mV[2]); }

	// Get center of triangle
	Vec3			GetCentroid() const
	{
		return (Vec3::sLoadFloat3Unsafe(mV[0]) + Vec3::sLoadFloat3Unsafe(mV[1]) + Vec3::sLoadFloat3Unsafe(mV[2])) * (1.0f / 3.0f);
	}

	// Intersect ray with triangle, returns closest point or FLT_MAX if no hit
	float			IntersectsRay(const Vec3 &inOrigin, const Vec3 &inDirection) const
	{
		return RayTriangle(inOrigin, inDirection, Vec3::sLoadFloat3Unsafe(mV[0]), Vec3::sLoadFloat3Unsafe(mV[1]), Vec3::sLoadFloat3Unsafe(mV[2]));
	}

	// Vertices
	Float3			mV[3];
	uint32			mMaterialIndex = 0;			// Follows mV[3] so that we can read mV as 4 vectors
};

using TriangleList = vector<Triangle>;
