#pragma once

#include <Math/Float4.h>
#include <Math/Swizzle.h>

class Vec3;
class UVec4;

class Vec4
{
public:
	// Constructor
	f_inline					Vec4()											{ }
	f_inline					Vec4(const Vec4 &inRHS) : mValue(inRHS.mValue)	{ }
	explicit f_inline			Vec4(const Vec3 &inRHS);						// WARNING: W component undefined!
	f_inline					Vec4(const Vec3 &inRHS, float inW);
	f_inline					Vec4(__m128 inRHS) : mValue(inRHS)				{ }

	// Create a vector from 4 components
	f_inline					Vec4(float inX, float inY, float inZ, float inW);

	// Vector with all zeros
	static f_inline Vec4		sZero();

	// Vector with all NaN's
	static f_inline Vec4		sNaN();

	// Replicate inV across all components
	static f_inline Vec4		sReplicate(float inV);

	// Load 4 floats from memory
	static f_inline Vec4		sLoadFloat4(const Float4 *inV);

	// Load 4 floats from memory, 16 bytes aligned
	static f_inline Vec4		sLoadFloat4Aligned(const Float4 *inV);

	// Gather 4 floats from memory at inBase + inOffsets[i] * Scale
	template <const int Scale>
	static f_inline Vec4		sGatherFloat4(const float *inBase, const UVec4 &inOffsets);

	// Return the minimum value of each of the components
	static f_inline Vec4		sMin(const Vec4 &inV1, const Vec4 &inV2);

	// Return the maximum of each of the components
	static f_inline Vec4		sMax(const Vec4 &inV1, const Vec4 &inV2);

	// Equals (component wise)
	static f_inline UVec4		sEquals(const Vec4 &inV1, const Vec4 &inV2);

	// Less than (component wise)
	static f_inline UVec4		sLess(const Vec4 &inV1, const Vec4 &inV2);

	// Less than or equal (component wise)
	static f_inline UVec4		sLessOrEqual(const Vec4 &inV1, const Vec4 &inV2);

	// Greater than (component wise)
	static f_inline UVec4		sGreater(const Vec4 &inV1, const Vec4 &inV2);

	// Greater than or equal (component wise)
	static f_inline UVec4		sGreaterOrEqual(const Vec4 &inV1, const Vec4 &inV2);

	// Calculates inMul1 * inMul2 + inAdd
	static f_inline Vec4		sFusedMultiplyAdd(const Vec4 &inMul1, const Vec4 &inMul2, const Vec4 &inAdd);

	// Calculates inMul1 * inMul2 - inSub
	static f_inline Vec4		sFusedMultiplySub(const Vec4 &inMul1, const Vec4 &inMul2, const Vec4 &inSub);

	// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
	static f_inline Vec4		sSelect(const Vec4 &inV1, const Vec4 &inV2, const UVec4 &inControl);

	// Logical or (component wise)
	static f_inline Vec4		sOr(const Vec4 &inV1, const Vec4 &inV2);

	// Logical xor (component wise)
	static f_inline Vec4		sXor(const Vec4 &inV1, const Vec4 &inV2);

	// Logical and (component wise)
	static f_inline Vec4		sAnd(const Vec4 &inV1, const Vec4 &inV2);
	
	// Sort the four elements of ioValue and sort ioIndex at the same time
	// Based on a sorting network: http://en.wikipedia.org/wiki/Sorting_network
	static f_inline void		sSort4(Vec4 &ioValue, UVec4 &ioIndex);

	// Reverse sort the four elements of ioValue (highest first) and sort ioIndex at the same time
	// Based on a sorting network: http://en.wikipedia.org/wiki/Sorting_network
	static f_inline void		sSort4Reverse(Vec4 &ioValue, UVec4 &ioIndex);

	// Get individual components
	f_inline float				GetX() const									{ return _mm_cvtss_f32(mValue); }
	f_inline float				GetY() const									{ return mF32[1]; }
	f_inline float				GetZ() const									{ return mF32[2]; }
	f_inline float				GetW() const									{ return mF32[3]; }

	// Set individual components
	f_inline void				SetX(float inX)									{ mF32[0] = inX; }
	f_inline void				SetY(float inY)									{ mF32[1] = inY; }
	f_inline void				SetZ(float inZ)									{ mF32[2] = inZ; }
	f_inline void				SetW(float inW)									{ mF32[3] = inW; }

	// Get float component by index
	f_inline float				operator [] (uint inCoordinate) const			{ assert(inCoordinate < 4); return mF32[inCoordinate]; }
	f_inline float &			operator [] (uint inCoordinate)					{ assert(inCoordinate < 4); return mF32[inCoordinate]; }

	// Assignment
	f_inline Vec4 &				operator = (const Vec4 &inV2);

	// Comparison
	f_inline bool				operator == (const Vec4 &inV2) const;
	f_inline bool				operator != (const Vec4 &inV2) const			{ return !(*this == inV2); }

	// Test if two vectors are close
	f_inline bool				IsClose(const Vec4 &inV2, float inMaxDistSq = 1.0e-12f) const;

	// Test if vector is normalized
	f_inline bool				IsNormalized(float inTolerance = 1.0e-6f) const;

	// Test if vector contains NaN elements
	f_inline bool				IsNaN() const;

	// Multiply two float vectors (component wise)
	f_inline Vec4				operator * (const Vec4 &inV2) const;

	// Multiply vector with float
	f_inline Vec4				operator * (float inV2) const;

	// Multiply vector with float
	friend f_inline Vec4		operator * (float inV1, const Vec4 &inV2);

	// Divide vector by float
	f_inline Vec4				operator / (float inV2) const;

	// Multiply vector with float
	f_inline Vec4 &				operator *= (float inV2);

	// Divide vector by float
	f_inline Vec4 &				operator /= (float inV2);

	// Add two float vectors (component wise)
	f_inline Vec4				operator + (const Vec4 &inV2) const;

	// Add two float vectors (component wise)
	f_inline Vec4 &				operator += (const Vec4 &inV2);

	// Negate
	f_inline Vec4				operator - () const;

	// Subtract two float vectors (component wise)
	f_inline Vec4				operator - (const Vec4 &inV2) const;

	// Add two float vectors (component wise)
	f_inline Vec4 &				operator -= (const Vec4 &inV2);

	// Divide (component wise)
	f_inline Vec4				operator / (const Vec4 &inV2) const;

	// Swizzle the elements in inV
	template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
	f_inline Vec4				Swizzle() const;

	// Replicate the X component to all components
	f_inline Vec4				SplatX() const;

	// Replicate the Y component to all components
	f_inline Vec4				SplatY() const;

	// Replicate the Z component to all components
	f_inline Vec4				SplatZ() const;

	// Replicate the W component to all components
	f_inline Vec4				SplatW() const;

	// Return the absolute value of each of the components
	f_inline Vec4				Abs() const;

	// Reciprocal vector (1 / value) for each of the components
	f_inline Vec4				Reciprocal() const;
	
	// Dot product, returns the dot product in X, Y and Z components
	f_inline Vec4				DotV(const Vec4 &inV2) const;
	
	// Dot product
	f_inline float				Dot(const Vec4 &inV2) const;

	// Squared length of vector
	f_inline float				LengthSq() const;

	// Length of vector
	f_inline float				Length() const;

	// Normalize vector
	f_inline Vec4				Normalized() const;

	// Store 4 floats to memory
	f_inline void				StoreFloat4(Float4 *outV) const;

	// Convert each component from a float to an int
	f_inline UVec4				ToInt() const;

	// Reinterpret Vec4 as a UVec4 (doesn't change the bits)
	f_inline UVec4				ReinterpretAsInt() const;

	// Store if X is negative in bit 0, Y in bit 1, Z in bit 2 and W in bit 3
	f_inline int				GetSignBits() const;

	// Get the minimum of X, Y, Z and W
	f_inline float				ReduceMin() const;

	// Get the maximum of X, Y, Z and W
	f_inline float				ReduceMax() const;

	// Component wise square root
	f_inline Vec4				Sqrt() const;

	// To String
	friend ostream &			operator << (ostream &inStream, const Vec4 &inV)
	{
		inStream << inV.mF32[0] << ", " << inV.mF32[1] << ", " << inV.mF32[2] << ", " << inV.mF32[3];
		return inStream;
	}

	union
	{
		__m128					mValue;
		float					mF32[4];
	};
};

// Load 4 floats from memory conditionally aligned to 16 bytes
template <bool Aligned>
static f_inline Vec4 Vec4LoadFloat4ConditionallyAligned(const Float4 *inV)
{
	return Vec4::sLoadFloat4(inV);
}

template <>
f_inline Vec4 Vec4LoadFloat4ConditionallyAligned<true>(const Float4 *inV)
{
	return Vec4::sLoadFloat4Aligned(inV);
}

#include "Vec4.inl"