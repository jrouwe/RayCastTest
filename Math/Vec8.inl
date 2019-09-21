// IWYU pragma: private, include "Math/Vec8.h"
#include <Math/UVec8.h>

// Set 256 bit vector from 2 128 bit vectors
Vec8::Vec8(const Vec4 &inLo, const Vec4 &inHi) :
	mValue(_mm256_insertf128_ps(_mm256_castps128_ps256(inLo.mValue), inHi.mValue, 1))
{		
}
	
// Vector with all zeros
Vec8 Vec8::sZero()
{
	return _mm256_setzero_ps();
}

// Replicate across all components
Vec8 Vec8::sReplicate(float inV)
{
	return _mm256_set1_ps(inV);
}

// Replicate the X component of inV to all components
Vec8 Vec8::sSplatX(const Vec4 &inV)
{
	return _mm256_set1_ps(inV.GetX());
}

// Replicate the Y component of inV to all components
Vec8 Vec8::sSplatY(const Vec4 &inV)
{
	return _mm256_set1_ps(inV.GetY());
}

// Replicate the Z component of inV to all components
Vec8 Vec8::sSplatZ(const Vec4 &inV)
{
	return _mm256_set1_ps(inV.GetZ());
}

// Calculates inMul1 * inMul2 + inAdd
Vec8 Vec8::sFusedMultiplyAdd(const Vec8 &inMul1, const Vec8 &inMul2, const Vec8 &inAdd)
{
#ifdef USE_FMADD
	return _mm256_fmadd_ps(inMul1.mValue, inMul2.mValue, inAdd.mValue);
#else
	return _mm256_add_ps(_mm256_mul_ps(inMul1.mValue, inMul2.mValue), inAdd.mValue);
#endif
}

// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
Vec8 Vec8::sSelect(const Vec8 &inV1, const Vec8 &inV2, const UVec8 &inControl)
{
	return _mm256_blendv_ps(inV1.mValue, inV2.mValue, _mm256_castsi256_ps(inControl.mValue));
}

// Component wise min
Vec8 Vec8::sMin(const Vec8 &inV1, const Vec8 &inV2)
{
	return _mm256_min_ps(inV1.mValue, inV2.mValue);
}

// Component wise max
Vec8 Vec8::sMax(const Vec8 &inV1, const Vec8 &inV2)
{
	return _mm256_max_ps(inV1.mValue, inV2.mValue);
}

// Less than
UVec8 Vec8::sLess(const Vec8 &inV1, const Vec8 &inV2)
{
	return _mm256_castps_si256(_mm256_cmp_ps(inV1.mValue, inV2.mValue, _CMP_LT_OQ));
}

// Less than
UVec8 Vec8::sGreater(const Vec8 &inV1, const Vec8 &inV2)
{
	return _mm256_castps_si256(_mm256_cmp_ps(inV1.mValue, inV2.mValue, _CMP_GT_OQ));
}

// Load from memory
Vec8 Vec8::sLoadFloat8(const float *inV)
{
	return _mm256_loadu_ps(inV);
}

// Load 8 floats from memory, 32 bytes aligned
Vec8 Vec8::sLoadFloat8Aligned(const float *inV)
{
	assert(IsAligned(inV, 32));
	return _mm256_load_ps(inV);
}

// Multiply two float vectors
Vec8 Vec8::operator * (const Vec8 &inV2) const
{
	return _mm256_mul_ps(mValue, inV2.mValue);
}

// Multiply vector by float
Vec8 Vec8::operator * (float inV2) const
{
	return _mm256_mul_ps(mValue, _mm256_set1_ps(inV2));
}

// Add two float vectors
Vec8 Vec8::operator + (const Vec8 &inV2) const
{
	return _mm256_add_ps(mValue, inV2.mValue);
}

// Subtract two float vectors
Vec8 Vec8::operator - (const Vec8 &inV2) const
{
	return _mm256_sub_ps(mValue, inV2.mValue);
}

// Divide
Vec8 Vec8::operator / (const Vec8 &inV2) const
{
	return _mm256_div_ps(mValue, inV2.mValue);
}

// Reciprocal vector
Vec8 Vec8::Reciprocal() const
{
	return Vec8::sReplicate(1.0f) / mValue;
}

// 256 bit variant of Vec::Swizzle (no cross 128 bit lane swizzle)
template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
Vec8 Vec8::Swizzle() const
{
	static_assert(SwizzleX <= 3, "SwizzleX template parameter out of range");
	static_assert(SwizzleY <= 3, "SwizzleY template parameter out of range");
	static_assert(SwizzleZ <= 3, "SwizzleZ template parameter out of range");
	static_assert(SwizzleW <= 3, "SwizzleW template parameter out of range");

	return _mm256_shuffle_ps(mValue, mValue, _MM_SHUFFLE(SwizzleW, SwizzleZ, SwizzleY, SwizzleX));
}

// Get absolute value of all components
Vec8 Vec8::Abs() const
{
	return _mm256_max_ps(_mm256_sub_ps(_mm256_setzero_ps(), mValue), mValue);
}
	
// Fetch the lower 128 bit from a 256 bit variable
Vec4 Vec8::LowerVec4() const
{
	return _mm256_castps256_ps128(mValue);
}

// Fetch the higher 128 bit from a 256 bit variable
Vec4 Vec8::UpperVec4() const
{
	return _mm256_extractf128_ps(mValue, 1);
}

// Get the minimum value of the 8 floats
float Vec8::ReduceMin() const
{
	return Vec4::sMin(LowerVec4(), UpperVec4()).ReduceMin();
}
