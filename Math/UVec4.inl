// IWYU pragma: private, include "Math/UVec4.h"

// Create a vector from 4 integer components
UVec4::UVec4(uint32 inX, uint32 inY, uint32 inZ, uint32 inW)
{
	mValue = _mm_set_epi32(int(inW), int(inZ), int(inY), int(inX));
}

// Comparison
bool UVec4::operator == (const UVec4 &inV2) const
{
	return sEquals(*this, inV2).TestAllTrue();
}

// Swizzle the elements in inV
template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
UVec4 UVec4::Swizzle() const
{
	static_assert(SwizzleX <= 3, "SwizzleX template parameter out of range");
	static_assert(SwizzleY <= 3, "SwizzleY template parameter out of range");
	static_assert(SwizzleZ <= 3, "SwizzleZ template parameter out of range");
	static_assert(SwizzleW <= 3, "SwizzleW template parameter out of range");

	return _mm_shuffle_epi32(mValue, _MM_SHUFFLE(SwizzleW, SwizzleZ, SwizzleY, SwizzleX));
}

// Vector with all zeros
UVec4 UVec4::sZero()
{
	return _mm_setzero_si128();
}

// Replicate int inV across all components
UVec4 UVec4::sReplicate(uint32 inV)
{
	return _mm_set1_epi32(int(inV));
}

// Load 1 int from memory and place it in the X component, zeros Y, Z and W
UVec4 UVec4::sLoadInt(const uint32 *inV)
{
	return _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(inV)));
}

// Load 4 ints from memory
UVec4 UVec4::sLoadInt4(const uint32 *inV)
{
	return _mm_loadu_si128(reinterpret_cast<const __m128i *>(inV));
}

// Load 4 ints from memory, aligned to 16 bytes
UVec4 UVec4::sLoadInt4Aligned(const uint32 *inV)
{
	assert(IsAligned(inV, 16));
	return _mm_load_si128(reinterpret_cast<const __m128i *>(inV));
}

// Load 3 bytes of inData into X, Y, Z (as ints)
UVec4 UVec4::sLoadBytes3(const uint32 *inData)
{
	UVec4 shuffle(0x80808000, 0x80808001, 0x80808002, 0x80808080);
	return _mm_shuffle_epi8(_mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(inData))), shuffle.mValue);
}

// Gather 4 ints from memory at inBase + inOffsets[i] * Scale
template <const int Scale>
UVec4 UVec4::sGatherInt4(const uint32 *inBase, const UVec4 &inOffsets)
{
#ifdef USE_AVX2
	return _mm_i32gather_epi32(reinterpret_cast<const int *>(inBase), inOffsets.mValue, Scale);
#else
	return Vec4::sGatherFloat4<Scale>(reinterpret_cast<const float *>(inBase), inOffsets).ReinterpretAsInt();
#endif
}

// Return the minimum value of each of the components
UVec4 UVec4::sMin(const UVec4 &inV1, const UVec4 &inV2)
{
	return _mm_min_epu32(inV1.mValue, inV2.mValue);
}

// Return the maximum of each of the components
UVec4 UVec4::sMax(const UVec4 &inV1, const UVec4 &inV2)
{
	return _mm_min_epu32(inV1.mValue, inV2.mValue);
}

// Equals (component wise)
UVec4 UVec4::sEquals(const UVec4 &inV1, const UVec4 &inV2)
{
	return _mm_cmpeq_epi32(inV1.mValue, inV2.mValue);
}

// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
UVec4 UVec4::sSelect(const UVec4 &inV1, const UVec4 &inV2, const UVec4 &inControl)
{
	return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(inV1.mValue), _mm_castsi128_ps(inV2.mValue), _mm_castsi128_ps(inControl.mValue)));
}

// Logical or (component wise)
UVec4 UVec4::sOr(const UVec4 &inV1, const UVec4 &inV2)
{
	return _mm_or_si128(inV1.mValue, inV2.mValue);
}

// Logical xor (component wise)
UVec4 UVec4::sXor(const UVec4 &inV1, const UVec4 &inV2)
{
	return _mm_xor_si128(inV1.mValue, inV2.mValue);
}

// Logical and (component wise)
UVec4 UVec4::sAnd(const UVec4 &inV1, const UVec4 &inV2)
{
	return _mm_and_si128(inV1.mValue, inV2.mValue);
}

// Logical not (component wise)
UVec4 UVec4::sNot(const UVec4 &inV1)
{
	return sXor(inV1, sReplicate(0xffffffff));
}

// Sorts 4 elements so that the True values go first (highest bit set), sorts ioIndex at the same time
// Based on a sorting network: http://en.wikipedia.org/wiki/Sorting_network
void UVec4::sSort4True(UVec4 &ioValue, UVec4 &ioIndex)
{
	// Pass 1, test 1st vs 3rd, 2nd vs 4th
	UVec4 v1 = ioValue.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y>();
	UVec4 i1 = ioIndex.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y>();
	UVec4 c1 = UVec4(_mm_cmplt_epi32(ioValue.mValue, v1.mValue)).Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_Z, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v1, c1);
	ioIndex = sSelect(ioIndex, i1, c1);

	// Pass 2, test 1st vs 2nd, 3rd vs 4th
	UVec4 v2 = ioValue.Swizzle<SWIZZLE_Y, SWIZZLE_X, SWIZZLE_W, SWIZZLE_Z>();
	UVec4 i2 = ioIndex.Swizzle<SWIZZLE_Y, SWIZZLE_X, SWIZZLE_W, SWIZZLE_Z>();
	UVec4 c2 = UVec4(_mm_cmplt_epi32(ioValue.mValue, v2.mValue)).Swizzle<SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_W, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v2, c2);
	ioIndex = sSelect(ioIndex, i2, c2);

	// Pass 3, test 2nd vs 3rd component
	UVec4 v3 = ioValue.Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Y, SWIZZLE_W>();
	UVec4 i3 = ioIndex.Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Y, SWIZZLE_W>();
	UVec4 c3 = UVec4(_mm_cmplt_epi32(ioValue.mValue, v3.mValue)).Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v3, c3);
	ioIndex = sSelect(ioIndex, i3, c3);
}

// Multiplies each of the 4 integer components with an integer (discards any overflow)
UVec4 UVec4::operator * (const UVec4 &inV2) const
{
	return _mm_mullo_epi32(mValue, inV2.mValue);
}

// Adds an integer value to all integer components (discards any overflow)
UVec4 UVec4::operator + (const UVec4 &inV2)
{
	return _mm_add_epi32(mValue, inV2.mValue);
}

// Add two integer vectors (component wise)
UVec4 &UVec4::operator += (const UVec4 &inV2)
{
	mValue = _mm_add_epi32(mValue, inV2.mValue);
	return *this;
}

// Replicate the X component to all components
UVec4 UVec4::SplatX() const
{
	return Swizzle<SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X>();
}

// Replicate the Y component to all components
UVec4 UVec4::SplatY() const
{
	return Swizzle<SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y>();
}

// Replicate the Z component to all components
UVec4 UVec4::SplatZ() const
{
	return Swizzle<SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z>();
}

// Replicate the W component to all components
UVec4 UVec4::SplatW() const
{
	return Swizzle<SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W>();
}

// Convert each component from an int to a float
Vec4 UVec4::ToFloat() const
{
	return _mm_cvtepi32_ps(mValue);
}

// Reinterpret UVec4 as a Vec4 (doesn't change the bits)
Vec4 UVec4::ReinterpretAsFloat() const
{
	return Vec4(_mm_castsi128_ps(mValue));
}

// Store 4 ints to memory
void UVec4::StoreInt4(uint32 *outV) const
{
	_mm_storeu_si128(reinterpret_cast<__m128i *>(outV), mValue);
}

// Test if any of the components are true (true is when highest bit of component is set)
bool UVec4::TestAnyTrue() const
{
	return _mm_movemask_ps(_mm_castsi128_ps(mValue)) != 0;
}

// Test if any of X, Y or Z components are true (true is when highest bit of component is set)
bool UVec4::TestAnyXYZTrue() const
{
	return (_mm_movemask_ps(_mm_castsi128_ps(mValue)) & 0x7) != 0;
}

// Test if all components are true (true is when highest bit of component is set)
bool UVec4::TestAllTrue() const
{
	return _mm_movemask_ps(_mm_castsi128_ps(mValue)) == 0xf;
}

// Test if X, Y and Z components are true (true is when highest bit of component is set)
bool UVec4::TestAllXYZTrue() const
{
	return (_mm_movemask_ps(_mm_castsi128_ps(mValue)) & 0x7) == 0x7;
}

// Count the number of components that are true (true is when highest bit of component is set)
int UVec4::CountTrues() const
{
	return CountBits(_mm_movemask_ps(_mm_castsi128_ps(mValue)));
}

// Store if X is true in bit 0, Y in bit 1, Z in bit 2 and W in bit 3 (true is when highest bit of component is set)
int UVec4::GetTrues() const
{
	return _mm_movemask_ps(_mm_castsi128_ps(mValue));
}

// Shift all components by inCount bits to the left (filling with zeros from the left)
UVec4 UVec4::LogicalShiftLeft(int inCount) const
{
	return _mm_slli_epi32(mValue, inCount);
}

// Shift all components by inCount bits to the right (filling with zeros from the right)
UVec4 UVec4::LogicalShiftRight(int inCount) const
{
	return _mm_srli_epi32(mValue, inCount);
}

// Shift all components by inCount bits to the right (shifting in the value of the highest bit)
UVec4 UVec4::ArithmeticShiftRight(int inCount) const
{
	return _mm_srai_epi32(mValue, inCount);
}

// Takes the lower 4 16 bits and expands them to X, Y, Z and W
UVec4 UVec4::Expand4Uint16Lo() const
{
	return _mm_unpacklo_epi16(mValue, _mm_castps_si128(_mm_setzero_ps()));
}

// Takes the upper 4 16 bits and expands them to X, Y, Z and W
UVec4 UVec4::Expand4Uint16Hi() const
{
	return _mm_unpackhi_epi16(mValue, _mm_castps_si128(_mm_setzero_ps()));
}

// Takes byte 0 .. 3 and expands them to X, Y, Z and W
UVec4 UVec4::Expand4Byte0() const
{
	return _mm_shuffle_epi8(mValue, _mm_set_epi32(0xffffff03, 0xffffff02, 0xffffff01, 0xffffff00));
}

// Takes byte 4 .. 7 and expands them to X, Y, Z and W
UVec4 UVec4::Expand4Byte4() const
{
	return _mm_shuffle_epi8(mValue, _mm_set_epi32(0xffffff07, 0xffffff06, 0xffffff05, 0xffffff04));
}

// Takes byte 8 .. 11 and expands them to X, Y, Z and W
UVec4 UVec4::Expand4Byte8() const
{
	return _mm_shuffle_epi8(mValue, _mm_set_epi32(0xffffff0b, 0xffffff0a, 0xffffff09, 0xffffff08));
}

// Takes byte 12 .. 15 and expands them to X, Y, Z and W
UVec4 UVec4::Expand4Byte12() const
{
	return _mm_shuffle_epi8(mValue, _mm_set_epi32(0xffffff0f, 0xffffff0e, 0xffffff0d, 0xffffff0c));
}

// Shift vector components by 4 - Count floats to the left, so if Count = 1 the resulting vector is (Y, Z, W, 0)
UVec4 UVec4::ShiftComponents4Minus(int inCount) const
{
	return _mm_shuffle_epi8(mValue, sFourMinusXShuffle[inCount].mValue);
}

// Convert 4 half floats (lower 64 bits) to floats
Vec4 UVec4::HalfFloatToFloat() const
{
	return _mm_cvtph_ps(mValue);
}
