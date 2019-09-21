#pragma once

class UVec8;

class Vec8
{
public:
	// Constructor
	f_inline					Vec8()											{ }
	f_inline					Vec8(const Vec8 &inRHS) : mValue(inRHS.mValue)	{ }
	f_inline					Vec8(__m256 inRHS) : mValue(inRHS)				{ }

	// Set 256 bit vector from 2 128 bit vectors
	f_inline					Vec8(const Vec4 &inLo, const Vec4 &inHi);
	
	// Vector with all zeros
	static f_inline Vec8		sZero();

	// Replicate across all components
	static f_inline Vec8		sReplicate(float inV);

	// Replicate the X component of inV to all components
	static f_inline Vec8		sSplatX(const Vec4 &inV);

	// Replicate the Y component of inV to all components
	static f_inline Vec8		sSplatY(const Vec4 &inV);

	// Replicate the Z component of inV to all components
	static f_inline Vec8		sSplatZ(const Vec4 &inV);

	// Calculates inMul1 * inMul2 + inAdd
	static f_inline Vec8		sFusedMultiplyAdd(const Vec8 &inMul1, const Vec8 &inMul2, const Vec8 &inAdd);

	// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
	static f_inline Vec8		sSelect(const Vec8 &inV1, const Vec8 &inV2, const UVec8 &inControl);

	// Component wise min
	static f_inline Vec8		sMin(const Vec8 &inV1, const Vec8 &inV2);

	// Component wise max
	static f_inline Vec8		sMax(const Vec8 &inV1, const Vec8 &inV2);

	// Less than
	static f_inline UVec8		sLess(const Vec8 &inV1, const Vec8 &inV2);

	// Less than
	static f_inline UVec8		sGreater(const Vec8 &inV1, const Vec8 &inV2);
	
	// Load from memory
	static f_inline Vec8		sLoadFloat8(const float *inV);

	// Load 8 floats from memory, 32 bytes aligned
	static f_inline Vec8		sLoadFloat8Aligned(const float *inV);

	// Get float component by index
	f_inline float				operator [] (uint inCoordinate) const			{ assert(inCoordinate < 8); return mF32[inCoordinate]; }
	f_inline float &			operator [] (uint inCoordinate)					{ assert(inCoordinate < 8); return mF32[inCoordinate]; }
	
	// Multiply two float vectors
	f_inline Vec8				operator * (const Vec8 &inV2) const;

	// Multiply vector by float
	f_inline Vec8				operator * (float inV2) const;

	// Add two float vectors
	f_inline Vec8				operator + (const Vec8 &inV2) const;

	// Subtract two float vectors
	f_inline Vec8				operator - (const Vec8 &inV2) const;

	// Divide
	f_inline Vec8				operator / (const Vec8 &inV2) const;

	// Reciprocal vector
	f_inline Vec8				Reciprocal() const;

	// 256 bit variant of Vec::Swizzle (no cross 128 bit lane swizzle)
	template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
	f_inline Vec8				Swizzle() const;

	// Get absolute value of all components
	f_inline Vec8				Abs() const;
	
	// Fetch the lower 128 bit from a 256 bit variable
	f_inline Vec4				LowerVec4() const;

	// Fetch the higher 128 bit from a 256 bit variable
	f_inline Vec4				UpperVec4() const;

	// Get the minimum value of the 8 floats
	f_inline float				ReduceMin() const;

	union
	{
		__m256					mValue;
		float					mF32[8];
	};
};

// Load 8 floats from memory conditionally aligned to 32 bytes
template <bool Aligned>
static f_inline Vec8 Vec8LoadFloat8ConditionallyAligned(const float *inV)
{
	return Vec8::sLoadFloat8(inV);
}

template <>
f_inline Vec8 Vec8LoadFloat8ConditionallyAligned<true>(const float *inV)
{
	return Vec8::sLoadFloat8Aligned(inV);
}
	
#include "Vec8.inl"