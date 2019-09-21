// IWYU pragma: private, include "Math/UVec8.h"

// Set 256 bit vector from 2 128 bit vectors
UVec8::UVec8(const UVec4 &inLo, const UVec4 &inHi) :
	mValue(_mm256_insertf128_si256(_mm256_castsi128_si256(inLo.mValue), inHi.mValue, 1))
{		
}

// Comparison
bool UVec8::operator == (const UVec8 &inV2) const
{
	return sEquals(*this, inV2).TestAllTrue();
}

// Replicate int across all components
UVec8 UVec8::sReplicate(uint32 inV)
{
	return _mm256_set1_epi32(int(inV));
}

// Replicate the X component of inV to all components
UVec8 UVec8::sSplatX(const UVec4 &inV)
{
	return _mm256_set1_epi32(inV.GetX());
}

// Replicate the Y component of inV to all components
UVec8 UVec8::sSplatY(const UVec4 &inV)
{
	return _mm256_set1_epi32(inV.GetY());
}

// Replicate the Z component of inV to all components
UVec8 UVec8::sSplatZ(const UVec4 &inV)
{
	return _mm256_set1_epi32(inV.GetZ());
}

// Equals (component wise)
UVec8 UVec8::sEquals(const UVec8 &inV1, const UVec8 &inV2)
{
#ifdef USE_AVX2
	return _mm256_cmpeq_epi32(inV1.mValue, inV2.mValue);
#else
	return UVec8(UVec4::sEquals(inV1.LowerVec4(), inV2.LowerVec4()), UVec4::sEquals(inV1.UpperVec4(), inV2.UpperVec4()));
#endif
}

// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
UVec8 UVec8::sSelect(const UVec8 &inV1, const UVec8 &inV2, const UVec8 &inControl)
{
	return _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(inV1.mValue), _mm256_castsi256_ps(inV2.mValue), _mm256_castsi256_ps(inControl.mValue)));
}

// Logical or
UVec8 UVec8::sOr(const UVec8 &inV1, const UVec8 &inV2)
{
	return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(inV1.mValue), _mm256_castsi256_ps(inV2.mValue)));
}

// Logical xor
UVec8 UVec8::sXor(const UVec8 &inV1, const UVec8 &inV2)
{
	return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(inV1.mValue), _mm256_castsi256_ps(inV2.mValue)));
}

// Logical and
UVec8 UVec8::sAnd(const UVec8 &inV1, const UVec8 &inV2)
{
	return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(inV1.mValue), _mm256_castsi256_ps(inV2.mValue)));
}

// 256 bit variant of Vec::Swizzle (no cross 128 bit lane swizzle)
template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
UVec8 UVec8::Swizzle() const
{
	static_assert(SwizzleX <= 3, "SwizzleX template parameter out of range");
	static_assert(SwizzleY <= 3, "SwizzleY template parameter out of range");
	static_assert(SwizzleZ <= 3, "SwizzleZ template parameter out of range");
	static_assert(SwizzleW <= 3, "SwizzleW template parameter out of range");

	return _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(mValue), _mm256_castsi256_ps(mValue), _MM_SHUFFLE(SwizzleW, SwizzleZ, SwizzleY, SwizzleX)));
}

// Test if any of the components are true (true is when highest bit of component is set)
bool UVec8::TestAnyTrue() const
{
	return _mm256_movemask_ps(_mm256_castsi256_ps(mValue)) != 0;
}

// Test if all components are true (true is when highest bit of component is set)
bool UVec8::TestAllTrue() const
{
	return _mm256_movemask_ps(_mm256_castsi256_ps(mValue)) == 0xff;
}

// Fetch the lower 128 bit from a 256 bit variable
UVec4 UVec8::LowerVec4() const
{
	return _mm256_castsi256_si128(mValue);
}

// Fetch the higher 128 bit from a 256 bit variable
UVec4 UVec8::UpperVec4() const
{
	return _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(mValue), 1));
}

// Converts int to float
Vec8 UVec8::ToFloat() const
{
	return _mm256_cvtepi32_ps(mValue);
}

// Shift all components by inCount bits to the left (filling with zeros from the left)
UVec8 UVec8::LogicalShiftLeft(int inCount) const
{
#ifdef USE_AVX2
	return _mm256_slli_epi32(mValue, inCount);
#else
	return UVec8(LowerVec4().LogicalShiftLeft(inCount), UpperVec4().LogicalShiftLeft(inCount));
#endif
}

// Shift all components by inCount bits to the right (filling with zeros from the right)
UVec8 UVec8::LogicalShiftRight(int inCount) const
{
#ifdef USE_AVX2
	return _mm256_srli_epi32(mValue, inCount);
#else
	return UVec8(LowerVec4().LogicalShiftRight(inCount), UpperVec4().LogicalShiftRight(inCount));
#endif
}

// Shift all components by inCount bits to the right (shifting in the value of the highest bit)
UVec8 UVec8::ArithmeticShiftRight(int inCount) const
{
#ifdef USE_AVX2
	return _mm256_srai_epi32(mValue, inCount);
#else
	return UVec8(LowerVec4().ArithmeticShiftRight(inCount), UpperVec4().ArithmeticShiftRight(inCount));
#endif
}
