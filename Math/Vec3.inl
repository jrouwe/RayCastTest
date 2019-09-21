// IWYU pragma: private, include "Math/Vec3.h"
#include <Math/Vec4.h>
#include <Math/UVec4.h>
#include <Core/HashCombine.h>
#include <random>

// Create a std::hash for Vec3
MAKE_HASHABLE(Vec3, t.GetX(), t.GetY(), t.GetZ())

// Constructor
Vec3::Vec3(const Vec4 &inRHS) : mValue(inRHS.mValue)
{ 
}

// Load 3 floats from memory
Vec3::Vec3(const Float3 &inV)
{
	__m128 x = _mm_load_ss(&inV.x);
	__m128 y = _mm_load_ss(&inV.y);
	__m128 z = _mm_load_ss(&inV.z);
	__m128 xy = _mm_unpacklo_ps(x, y);
	mValue = _mm_movelh_ps(xy, z);
}

// Create a vector from 4 components
Vec3::Vec3(float inX, float inY, float inZ) : 
	mValue(_mm_set_ps(0, inZ, inY, inX))
{
}

// Swizzle the elements in inV
template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
Vec3 Vec3::Swizzle() const
{
	static_assert(SwizzleX <= 3, "SwizzleX template parameter out of range");
	static_assert(SwizzleY <= 3, "SwizzleY template parameter out of range");
	static_assert(SwizzleZ <= 3, "SwizzleZ template parameter out of range");
	static_assert(SwizzleW <= 3, "SwizzleW template parameter out of range");

	return _mm_shuffle_ps(mValue, mValue, _MM_SHUFFLE(SwizzleW, SwizzleZ, SwizzleY, SwizzleX));
}

// Vector with all zeros
Vec3 Vec3::sZero()
{
	return _mm_setzero_ps();
}

// Vector with all NaN's
Vec3 Vec3::sNaN()
{
	return _mm_set1_ps(numeric_limits<float>::quiet_NaN());
}

// Replicate inV across all components
Vec3 Vec3::sReplicate(float inV)
{
	return _mm_set1_ps(inV);
}

// Load 3 floats from memory (reads 32 bits extra which it doesn't use)
Vec3 Vec3::sLoadFloat3Unsafe(const Float3 &inV)
{
	return _mm_loadu_ps(&inV.x);
}

// Return the minimum value of each of the components
Vec3 Vec3::sMin(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_min_ps(inV1.mValue, inV2.mValue);
}

// Return the maximum of each of the components
Vec3 Vec3::sMax(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_max_ps(inV1.mValue, inV2.mValue);
}

// Clamp a vector between min and max (component wise)
Vec3 Vec3::sClamp(const Vec3 &inV, const Vec3 &inMin, const Vec3 &inMax)
{
	return sMax(sMin(inV, inMax), inMin);
}

// Equals (component wise)
UVec4 Vec3::sEquals(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_castps_si128(_mm_cmpeq_ps(inV1.mValue, inV2.mValue));
}

// Less than (component wise)
UVec4 Vec3::sLess(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_castps_si128(_mm_cmplt_ps(inV1.mValue, inV2.mValue));
}

// Less than or equal (component wise)
UVec4 Vec3::sLessOrEqual(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_castps_si128(_mm_cmple_ps(inV1.mValue, inV2.mValue));
}

// Greater than (component wise)
UVec4 Vec3::sGreater(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_castps_si128(_mm_cmpgt_ps(inV1.mValue, inV2.mValue));
}

// Greater than or equal (component wise)
UVec4 Vec3::sGreaterOrEqual(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_castps_si128(_mm_cmpge_ps(inV1.mValue, inV2.mValue));
}

// Calculates inMul1 * inMul2 + inAdd
Vec3 Vec3::sFusedMultiplyAdd(const Vec3 &inMul1, const Vec3 &inMul2, const Vec3 &inAdd)
{
#ifdef USE_FMADD
	return _mm_fmadd_ps(inMul1.mValue, inMul2.mValue, inAdd.mValue);
#else
	return _mm_add_ps(_mm_mul_ps(inMul1.mValue, inMul2.mValue), inAdd.mValue);
#endif
}

// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
Vec3 Vec3::sSelect(const Vec3 &inV1, const Vec3 &inV2, const UVec4 &inControl)
{
	return _mm_blendv_ps(inV1.mValue, inV2.mValue, _mm_castsi128_ps(inControl.mValue));
}

// Logical or (component wise)
Vec3 Vec3::sOr(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_or_ps(inV1.mValue, inV2.mValue);
}

// Logical xor (component wise)
Vec3 Vec3::sXor(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_xor_ps(inV1.mValue, inV2.mValue);
}

// Logical and (component wise)
Vec3 Vec3::sAnd(const Vec3 &inV1, const Vec3 &inV2)
{
	return _mm_and_ps(inV1.mValue, inV2.mValue);
}

// Get unit vector given spherical coordinates
// inTheta e [0, pi] is angle between vector and z-axis
// inPhi e [0, 2 pi] is the angle in the xy-plane starting from the x axis and rotating counter clockwise around the z-axis
Vec3 Vec3::sUnitSpherical(float inTheta, float inPhi)
{
	float sint = sin(inTheta);
	return Vec3(sint * cos(inPhi), sint * sin(inPhi), cos(inTheta));
}

// Get random unit vector
template <class Random>
Vec3 Vec3::sRandom(Random &inRandom)
{
	uniform_real_distribution<float> zero_to_one(0.0f, 1.0f);
	float theta = F_PI * zero_to_one(inRandom);
	float phi = 2.0f * F_PI * zero_to_one(inRandom);
	return sUnitSpherical(theta, phi);
}

// Assignment
Vec3 &Vec3::operator = (const Vec3 &inV2)
{
	mValue = inV2.mValue;
	return *this;
}

// Comparison
bool Vec3::operator == (const Vec3 &inV2) const 
{ 
	return sEquals(*this, inV2).TestAllXYZTrue();
}

// Test if two vectors are close
bool Vec3::IsClose(const Vec3 &inV2, float inMaxDistSq) const
{
	return (inV2 - *this).LengthSq() <= inMaxDistSq;
}

// Test if vector is near zero
bool Vec3::IsNearZero(float inMaxDistSq) const
{
	return LengthSq() <= inMaxDistSq;
}

// Multiply two float vectors (component wise)
Vec3 Vec3::operator * (const Vec3 &inV2) const
{
	return _mm_mul_ps(mValue, inV2.mValue);
}

// Multiply vector with float
Vec3 Vec3::operator * (float inV2) const
{
	return _mm_mul_ps(mValue, _mm_set1_ps(inV2));
}

// Multiply vector with float
Vec3 operator * (float inV1, const Vec3 &inV2)
{
	return _mm_mul_ps(_mm_set1_ps(inV1), inV2.mValue);
}

// Divide vector by float
Vec3 Vec3::operator / (float inV2) const
{
	return _mm_div_ps(mValue, _mm_set1_ps(inV2));
}

// Multiply vector with float
Vec3 &Vec3::operator *= (float inV2)
{
	mValue = _mm_mul_ps(mValue, _mm_set1_ps(inV2));
	return *this;
}

// Multiply vector with vector
Vec3 &Vec3::operator *= (const Vec3 &inV2)
{
	mValue = _mm_mul_ps(mValue, inV2.mValue);
	return *this;
}

// Divide vector by float
Vec3 &Vec3::operator /= (float inV2)
{
	mValue = _mm_div_ps(mValue, _mm_set1_ps(inV2));
	return *this;
}

// Add two float vectors (component wise)
Vec3 Vec3::operator + (const Vec3 &inV2) const
{
	return _mm_add_ps(mValue, inV2.mValue);
}

// Add two float vectors (component wise)
Vec3 &Vec3::operator += (const Vec3 &inV2)
{
	mValue = _mm_add_ps(mValue, inV2.mValue);
	return *this;
}

// Negate
Vec3 Vec3::operator - () const
{
	return _mm_sub_ps(_mm_setzero_ps(), mValue);
}

// Subtract two float vectors (component wise)
Vec3 Vec3::operator - (const Vec3 &inV2) const
{
	return _mm_sub_ps(mValue, inV2.mValue);
}

// Add two float vectors (component wise)
Vec3 &Vec3::operator -= (const Vec3 &inV2)
{
	mValue = _mm_sub_ps(mValue, inV2.mValue);
	return *this;
}

// Divide (component wise)
Vec3 Vec3::operator / (const Vec3 &inV2) const
{
	return _mm_div_ps(mValue, inV2.mValue);
}

// Replicate the X component to all components
Vec4 Vec3::SplatX() const
{
	return Vec4(mValue).Swizzle<SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X>();
}

// Replicate the Y component to all components
Vec4 Vec3::SplatY() const
{
	return Vec4(mValue).Swizzle<SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y>();
}

// Replicate the Z component to all components
Vec4 Vec3::SplatZ() const
{
	return Vec4(mValue).Swizzle<SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z>();
}

// Get index of component with lowest value
int Vec3::GetLowestComponentIndex() const
{
	return GetX() < GetY() ? (GetZ() < GetX() ? 2 : 0) : (GetZ() < GetY() ? 2 : 1);
}

// Get index of component with highest value
int Vec3::GetHighestComponentIndex() const
{
	return GetX() > GetY() ? (GetZ() > GetX() ? 2 : 0) : (GetZ() > GetY() ? 2 : 1);
}

// Return the absolute value of each of the components
Vec3 Vec3::Abs() const
{
	return _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), mValue), mValue);
}

// Reciprocal vector (1 / value) for each of the components
Vec3 Vec3::Reciprocal() const
{
	return sReplicate(1.0f) / mValue;
}

// Cross product
Vec3 Vec3::Cross(const Vec3 &inV2) const
{
	__m128  t1 = _mm_shuffle_ps(mValue, mValue, _MM_SHUFFLE(3, 0, 2, 1));
	__m128  t2 = _mm_shuffle_ps(inV2.mValue, inV2.mValue, _MM_SHUFFLE(3, 1, 0, 2));
	__m128  result = _mm_mul_ps(t1, t2);

	t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(3, 0, 2, 1));
	t2 = _mm_shuffle_ps(t2, t2, _MM_SHUFFLE(3, 1, 0, 2));

	result = _mm_sub_ps(result, _mm_mul_ps(t1, t2));
	return result;
}

// Dot product, returns the dot product in X, Y and Z components
Vec3 Vec3::DotV(const Vec3 &inV2) const
{
	return _mm_dp_ps(mValue, inV2.mValue, 0x7f);
}

// Dot product
float Vec3::Dot(const Vec3 &inV2) const
{
	return _mm_cvtss_f32(_mm_dp_ps(mValue, inV2.mValue, 0x7f));
}

// Squared length of vector
float Vec3::LengthSq() const
{
	return _mm_cvtss_f32(_mm_dp_ps(mValue, mValue, 0x7f));
}

// Length of vector
float Vec3::Length() const
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(mValue, mValue, 0x7f)));
}

// Component wise square root
Vec3 Vec3::Sqrt() const
{
	return _mm_sqrt_ps(mValue);
}

// Normalize 3 vector
Vec3 Vec3::Normalized() const
{
	return _mm_div_ps(mValue, _mm_sqrt_ps(_mm_dp_ps(mValue, mValue, 0x7f)));
}

// Test if vector is normalized
bool Vec3::IsNormalized(float inTolerance) const 
{ 
	return abs(LengthSq() - 1.0f) <= inTolerance; 
}

// Test if vector contains NaN elements
bool Vec3::IsNaN() const
{
	return (_mm_movemask_ps(_mm_cmpunord_ps(mValue, mValue)) & 0x7) != 0;
}

// Store 3 floats to memory
void Vec3::StoreFloat3(Float3 *outV) const
{
	_mm_store_ss(&outV->x, mValue);
	Vec3 t = Swizzle<SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_UNUSED>();
	_mm_store_ss(&outV->y, t.mValue);
	t = t.Swizzle<SWIZZLE_Y, SWIZZLE_UNUSED, SWIZZLE_UNUSED>();
	_mm_store_ss(&outV->z, t.mValue);
}

// Convert each component from a float to an int
UVec4 Vec3::ToInt() const
{
	return _mm_cvttps_epi32(mValue);
}

// Reinterpret Vec3 as a UVec4 (doesn't change the bits)
UVec4 Vec3::ReinterpretAsInt() const
{
	return UVec4(_mm_castps_si128(mValue));
}

// Get the minimum of X, Y and Z
float Vec3::ReduceMin() const
{
	Vec3 v = sMin(mValue, Swizzle<SWIZZLE_Y, SWIZZLE_UNUSED, SWIZZLE_Z>());
	v = sMin(v, v.Swizzle<SWIZZLE_Z, SWIZZLE_UNUSED, SWIZZLE_UNUSED>());
	return v.GetX();
}

// Get the maximum of X, Y and Z
float Vec3::ReduceMax() const
{
	Vec3 v = sMax(mValue, Swizzle<SWIZZLE_Y, SWIZZLE_UNUSED, SWIZZLE_Z>());
	v = sMax(v, v.Swizzle<SWIZZLE_Z, SWIZZLE_UNUSED, SWIZZLE_UNUSED>());
	return v.GetX();
}

// Get vector that is perpendicular to this vector
Vec3 Vec3::GetPerpendicular() const
{
	if (abs(mF32[0]) > abs(mF32[1]))
	{
		float len = sqrt(mF32[0] * mF32[0] + mF32[2] * mF32[2]);
		return Vec3(mF32[2], 0.0f, -mF32[0]) / len;
	}
	else
	{
		float len = sqrt(mF32[1] * mF32[1] + mF32[2] * mF32[2]);
		return Vec3(0.0f, mF32[2], -mF32[1]) / len;
	}
}

// Get vector that contains the sign of each element (returns 1.0f if positive, -1.0f if negative)
Vec3 Vec3::GetSign() const
{
	__m128 minus_one = _mm_set1_ps(-1.0f);
	__m128 one = _mm_set1_ps(1.0f);
	return _mm_or_ps(_mm_and_ps(mValue, minus_one), one);
}
