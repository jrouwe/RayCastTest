// IWYU pragma: private, include "Math/Vec4.h"
#include <Math/Vec3.h>
#include <Math/UVec4.h>

// Constructor
Vec4::Vec4(const Vec3 &inRHS) : 
	mValue(inRHS.mValue) 
{ 
}

// Constructor
Vec4::Vec4(const Vec3 &inRHS, float inW) :
	mValue(_mm_blend_ps(inRHS.mValue, _mm_set1_ps(inW), 8))
{
}

// Create a vector from 4 components
Vec4::Vec4(float inX, float inY, float inZ, float inW) : 
	mValue(_mm_set_ps(inW, inZ, inY, inX))
{
}

// Swizzle the elements in inV
template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
Vec4 Vec4::Swizzle() const
{
	static_assert(SwizzleX <= 3, "SwizzleX template parameter out of range");
	static_assert(SwizzleY <= 3, "SwizzleY template parameter out of range");
	static_assert(SwizzleZ <= 3, "SwizzleZ template parameter out of range");
	static_assert(SwizzleW <= 3, "SwizzleW template parameter out of range");

	return _mm_shuffle_ps(mValue, mValue, _MM_SHUFFLE(SwizzleW, SwizzleZ, SwizzleY, SwizzleX));
}

// Vector with all zeros
Vec4 Vec4::sZero()
{
	return _mm_setzero_ps();
}

// Vector with all NaN's
Vec4 Vec4::sNaN()
{
	return _mm_set1_ps(numeric_limits<float>::quiet_NaN());
}

// Replicate inV across all components
Vec4 Vec4::sReplicate(float inV)
{
	return _mm_set1_ps(inV);
}

// Load 4 floats from memory
Vec4 Vec4::sLoadFloat4(const Float4 *inV)
{
	return _mm_loadu_ps(&inV->x);
}

// Load 4 floats from memory, 16 bytes aligned
Vec4 Vec4::sLoadFloat4Aligned(const Float4 *inV)
{
	assert(IsAligned(inV, 16));
	return _mm_load_ps(&inV->x);
}

// Gather 4 floats from memory at inBase + inOffsets[i] * Scale
template <const int Scale>
Vec4 Vec4::sGatherFloat4(const float *inBase, const UVec4 &inOffsets)
{
#ifdef USE_AVX2
	return _mm_i32gather_ps(inBase, inOffsets.mValue, Scale);
#else
	const uint8 *base = reinterpret_cast<const uint8 *>(inBase);
	__m128 x = _mm_load_ss(reinterpret_cast<const float *>(base + inOffsets.GetX() * Scale));
	__m128 y = _mm_load_ss(reinterpret_cast<const float *>(base + inOffsets.GetY() * Scale));
	__m128 xy = _mm_unpacklo_ps(x, y);
	__m128 z = _mm_load_ss(reinterpret_cast<const float *>(base + inOffsets.GetZ() * Scale));
	__m128 w = _mm_load_ss(reinterpret_cast<const float *>(base + inOffsets.GetW() * Scale));
	__m128 zw = _mm_unpacklo_ps(z, w);
	return _mm_movelh_ps(xy, zw);
#endif
}

// Return the minimum value of each of the components
Vec4 Vec4::sMin(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_min_ps(inV1.mValue, inV2.mValue);
}

// Return the maximum of each of the components
Vec4 Vec4::sMax(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_max_ps(inV1.mValue, inV2.mValue);
}

// Equals (component wise)
UVec4 Vec4::sEquals(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_castps_si128(_mm_cmpeq_ps(inV1.mValue, inV2.mValue));
}

// Less than (component wise)
UVec4 Vec4::sLess(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_castps_si128(_mm_cmplt_ps(inV1.mValue, inV2.mValue));
}

// Less than or equal (component wise)
UVec4 Vec4::sLessOrEqual(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_castps_si128(_mm_cmple_ps(inV1.mValue, inV2.mValue));
}

// Greater than (component wise)
UVec4 Vec4::sGreater(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_castps_si128(_mm_cmpgt_ps(inV1.mValue, inV2.mValue));
}

// Greater than or equal (component wise)
UVec4 Vec4::sGreaterOrEqual(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_castps_si128(_mm_cmpge_ps(inV1.mValue, inV2.mValue));
}

// Calculates inMul1 * inMul2 + inAdd
Vec4 Vec4::sFusedMultiplyAdd(const Vec4 &inMul1, const Vec4 &inMul2, const Vec4 &inAdd)
{
#ifdef USE_FMADD
	return _mm_fmadd_ps(inMul1.mValue, inMul2.mValue, inAdd.mValue);
#else
	return _mm_add_ps(_mm_mul_ps(inMul1.mValue, inMul2.mValue), inAdd.mValue);
#endif
}

// Calculates inMul1 * inMul2 - inSub
Vec4 Vec4::sFusedMultiplySub(const Vec4 &inMul1, const Vec4 &inMul2, const Vec4 &inSub)
{
#ifdef USE_FMADD
	return _mm_fmsub_ps(inMul1.mValue, inMul2.mValue, inSub.mValue);
#else
	return _mm_sub_ps(_mm_mul_ps(inMul1.mValue, inMul2.mValue), inSub.mValue);
#endif
}

// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
Vec4 Vec4::sSelect(const Vec4 &inV1, const Vec4 &inV2, const UVec4 &inControl)
{
	return _mm_blendv_ps(inV1.mValue, inV2.mValue, _mm_castsi128_ps(inControl.mValue));
}

// Logical or (component wise)
Vec4 Vec4::sOr(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_or_ps(inV1.mValue, inV2.mValue);
}

// Logical xor (component wise)
Vec4 Vec4::sXor(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_xor_ps(inV1.mValue, inV2.mValue);
}

// Logical and (component wise)
Vec4 Vec4::sAnd(const Vec4 &inV1, const Vec4 &inV2)
{
	return _mm_and_ps(inV1.mValue, inV2.mValue);
}

// Sort the four elements of ioValue and sort ioIndex at the same time
// Based on a sorting network: http://en.wikipedia.org/wiki/Sorting_network
void Vec4::sSort4(Vec4 &ioValue, UVec4 &ioIndex)
{
	// Pass 1, test 1st vs 3rd, 2nd vs 4th
	Vec4 v1 = ioValue.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y>();
	UVec4 i1 = ioIndex.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y>();
	UVec4 c1 = sLess(ioValue, v1).Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_Z, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v1, c1);
	ioIndex = UVec4::sSelect(ioIndex, i1, c1);

	// Pass 2, test 1st vs 2nd, 3rd vs 4th
	Vec4 v2 = ioValue.Swizzle<SWIZZLE_Y, SWIZZLE_X, SWIZZLE_W, SWIZZLE_Z>();
	UVec4 i2 = ioIndex.Swizzle<SWIZZLE_Y, SWIZZLE_X, SWIZZLE_W, SWIZZLE_Z>();
	UVec4 c2 = sLess(ioValue, v2).Swizzle<SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_W, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v2, c2);
	ioIndex = UVec4::sSelect(ioIndex, i2, c2);

	// Pass 3, test 2nd vs 3rd component
	Vec4 v3 = ioValue.Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Y, SWIZZLE_W>();
	UVec4 i3 = ioIndex.Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Y, SWIZZLE_W>();
	UVec4 c3 = sLess(ioValue, v3).Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v3, c3);
	ioIndex = UVec4::sSelect(ioIndex, i3, c3);
}

// Reverse sort the four elements of ioValue (highest first) and sort ioIndex at the same time
// Based on a sorting network: http://en.wikipedia.org/wiki/Sorting_network
void Vec4::sSort4Reverse(Vec4 &ioValue, UVec4 &ioIndex)
{
	// Pass 1, test 1st vs 3rd, 2nd vs 4th
	Vec4 v1 = ioValue.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y>();
	UVec4 i1 = ioIndex.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_X, SWIZZLE_Y>();
	UVec4 c1 = sGreater(ioValue, v1).Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_Z, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v1, c1);
	ioIndex = UVec4::sSelect(ioIndex, i1, c1);

	// Pass 2, test 1st vs 2nd, 3rd vs 4th
	Vec4 v2 = ioValue.Swizzle<SWIZZLE_Y, SWIZZLE_X, SWIZZLE_W, SWIZZLE_Z>();
	UVec4 i2 = ioIndex.Swizzle<SWIZZLE_Y, SWIZZLE_X, SWIZZLE_W, SWIZZLE_Z>();
	UVec4 c2 = sGreater(ioValue, v2).Swizzle<SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_W, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v2, c2);
	ioIndex = UVec4::sSelect(ioIndex, i2, c2);

	// Pass 3, test 2nd vs 3rd component
	Vec4 v3 = ioValue.Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Y, SWIZZLE_W>();
	UVec4 i3 = ioIndex.Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Y, SWIZZLE_W>();
	UVec4 c3 = sGreater(ioValue, v3).Swizzle<SWIZZLE_X, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_W>();
	ioValue = sSelect(ioValue, v3, c3);
	ioIndex = UVec4::sSelect(ioIndex, i3, c3);
}

// Assignment
Vec4 &Vec4::operator = (const Vec4 &inV2)
{
	mValue = inV2.mValue;
	return *this;
}

// Comparison
bool Vec4::operator == (const Vec4 &inV2) const 
{ 
	return sEquals(*this, inV2).TestAllTrue();
}

// Test if two vectors are close
bool Vec4::IsClose(const Vec4 &inV2, float inMaxDistSq) const
{
	return (inV2 - *this).LengthSq() <= inMaxDistSq;
}

// Test if vector is normalized
bool Vec4::IsNormalized(float inTolerance) const 
{ 
	return abs(LengthSq() - 1.0f) <= inTolerance; 
}

// Test if vector contains NaN elements
bool Vec4::IsNaN() const
{
	return _mm_movemask_ps(_mm_cmpunord_ps(mValue, mValue)) != 0;
}

// Multiply two float vectors (component wise)
Vec4 Vec4::operator * (const Vec4 &inV2) const
{
	return _mm_mul_ps(mValue, inV2.mValue);
}

// Multiply vector with float
Vec4 Vec4::operator * (float inV2) const
{
	return _mm_mul_ps(mValue, _mm_set1_ps(inV2));
}

// Multiply vector with float
Vec4 operator * (float inV1, const Vec4 &inV2)
{
	return _mm_mul_ps(_mm_set1_ps(inV1), inV2.mValue);
}

// Divide vector by float
Vec4 Vec4::operator / (float inV2) const
{
	return _mm_div_ps(mValue, _mm_set1_ps(inV2));
}

// Multiply vector with float
Vec4 &Vec4::operator *= (float inV2)
{
	mValue = _mm_mul_ps(mValue, _mm_set1_ps(inV2));
	return *this;
}

// Divide vector by float
Vec4 &Vec4::operator /= (float inV2)
{
	mValue = _mm_div_ps(mValue, _mm_set1_ps(inV2));
	return *this;
}

// Add two float vectors (component wise)
Vec4 Vec4::operator + (const Vec4 &inV2) const
{
	return _mm_add_ps(mValue, inV2.mValue);
}

// Add two float vectors (component wise)
Vec4 &Vec4::operator += (const Vec4 &inV2)
{
	mValue = _mm_add_ps(mValue, inV2.mValue);
	return *this;
}

// Negate
Vec4 Vec4::operator - () const
{
	return _mm_sub_ps(_mm_setzero_ps(), mValue);
}

// Subtract two float vectors (component wise)
Vec4 Vec4::operator - (const Vec4 &inV2) const
{
	return _mm_sub_ps(mValue, inV2.mValue);
}

// Add two float vectors (component wise)
Vec4 &Vec4::operator -= (const Vec4 &inV2)
{
	mValue = _mm_sub_ps(mValue, inV2.mValue);
	return *this;
}

// Divide (component wise)
Vec4 Vec4::operator / (const Vec4 &inV2) const
{
	return _mm_div_ps(mValue, inV2.mValue);
}

// Replicate the X component to all components
Vec4 Vec4::SplatX() const
{
	return Swizzle<SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X>();
}

// Replicate the Y component to all components
Vec4 Vec4::SplatY() const
{
	return Swizzle<SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y>();
}

// Replicate the Z component to all components
Vec4 Vec4::SplatZ() const
{
	return Swizzle<SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z>();
}

// Replicate the W component to all components
Vec4 Vec4::SplatW() const
{
	return Swizzle<SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W>();
}

// Return the absolute value of each of the components
Vec4 Vec4::Abs() const
{
	return _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), mValue), mValue);
}

// Reciprocal vector (1 / value) for each of the components
Vec4 Vec4::Reciprocal() const
{
	return sReplicate(1.0f) / mValue;
}

// Dot product, returns the dot product in X, Y and Z components
Vec4 Vec4::DotV(const Vec4 &inV2) const
{
	return _mm_dp_ps(mValue, inV2.mValue, 0xff);
}

// Dot product
float Vec4::Dot(const Vec4 &inV2) const
{
	return _mm_cvtss_f32(_mm_dp_ps(mValue, inV2.mValue, 0xff));
}

// Squared length of vector
float Vec4::LengthSq() const
{
	return _mm_cvtss_f32(_mm_dp_ps(mValue, mValue, 0xff));
}

// Length of vector
float Vec4::Length() const
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(mValue, mValue, 0xff)));
}

// Component wise square root
Vec4 Vec4::Sqrt() const
{
	return _mm_sqrt_ps(mValue);
}

// Normalize vector
Vec4 Vec4::Normalized() const
{
	return _mm_div_ps(mValue, _mm_sqrt_ps(_mm_dp_ps(mValue, mValue, 0xff)));
}

// Store 4 floats to memory
void Vec4::StoreFloat4(Float4 *outV) const
{
	_mm_storeu_ps(&outV->x, mValue);
}

// Convert each component from a float to an int
UVec4 Vec4::ToInt() const
{
	return _mm_cvttps_epi32(mValue);
}

// Reinterpret Vec4 as a UVec4 (doesn't change the bits)
UVec4 Vec4::ReinterpretAsInt() const
{
	return UVec4(_mm_castps_si128(mValue));
}

// Store if X is negative in bit 0, Y in bit 1, Z in bit 2 and W in bit 3
int Vec4::GetSignBits() const
{
	return _mm_movemask_ps(mValue);
}

// Get the minimum of X, Y, Z and W
float Vec4::ReduceMin() const
{
	Vec4 v = sMin(mValue, Swizzle<SWIZZLE_Y, SWIZZLE_UNUSED, SWIZZLE_W, SWIZZLE_UNUSED>());
	v = sMin(v, v.Swizzle<SWIZZLE_Z, SWIZZLE_UNUSED, SWIZZLE_UNUSED, SWIZZLE_UNUSED>());
	return v.GetX();
}

// Get the maximum of X, Y, Z and W
float Vec4::ReduceMax() const
{
	Vec4 v = sMax(mValue, Swizzle<SWIZZLE_Y, SWIZZLE_UNUSED, SWIZZLE_W, SWIZZLE_UNUSED>());
	v = sMax(v, v.Swizzle<SWIZZLE_Z, SWIZZLE_UNUSED, SWIZZLE_UNUSED, SWIZZLE_UNUSED>());
	return v.GetX();
}
