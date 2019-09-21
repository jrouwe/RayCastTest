// IWYU pragma: private, include "Math/Mat44.h"
#pragma once

#include <Math/Vec3.h>
#include <Math/Vec4.h>

// Constructor
Mat44::Mat44(const Vec4 &inC1, const Vec4 &inC2, const Vec4 &inC3, const Vec4 &inC4) : mCol { inC1, inC2, inC3, inC4 } 
{ 
}

// Constructor
Mat44::Mat44(__m128 inC1, __m128 inC2, __m128 inC3, __m128 inC4) : mCol { inC1, inC2, inC3, inC4 } 
{
}

// Constructor
Mat44::Mat44(const Mat44 &inM2) : mCol { inM2.mCol[0], inM2.mCol[1], inM2.mCol[2], inM2.mCol[3] }
{ 
}

// Zero matrix
Mat44 Mat44::sZero()
{
	return Mat44(Vec4::sZero(), Vec4::sZero(), Vec4::sZero(), Vec4::sZero());
}

// Identity matrix
Mat44 Mat44::sIdentity()
{
	return Mat44(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1));
}

// Matrix filled with NaN's
Mat44 Mat44::sNaN()
{
	return Mat44(Vec4::sNaN(), Vec4::sNaN(), Vec4::sNaN(), Vec4::sNaN());
}

// Load 16 floats from memory
Mat44 Mat44::sLoadFloat4x4(const Float4 *inV)
{
	Mat44 result;
	for (int c = 0; c < 4; ++c)
		result.mCol[c] = Vec4::sLoadFloat4(inV + c);
	return result;
}

// Load 16 floats from memory, 16 bytes aligned
Mat44 Mat44::sLoadFloat4x4Aligned(const Float4 *inV)
{
	Mat44 result;
	for (int c = 0; c < 4; ++c)
		result.mCol[c] = Vec4::sLoadFloat4Aligned(inV + c);
	return result;
}

// Rotate around X axis (angle in radians)
Mat44 Mat44::sRotationX(float inX)
{
	// TODO: Could be optimized
	float c = cos(inX), s = sin(inX);
	return Mat44(Vec4(1, 0, 0, 0), Vec4(0, c, s, 0), Vec4(0, -s, c, 0), Vec4(0, 0, 0, 1));
}

// Rotate around X axis (angle in radians)
Mat44 Mat44::sRotationY(float inY)
{
	// TODO: Could be optimized
	float c = cos(inY), s = sin(inY);
	return Mat44(Vec4(c, 0, -s, 0), Vec4(0, 1, 0, 0), Vec4(s, 0, c, 0), Vec4(0, 0, 0, 1));
}

// Rotate around X axis (angle in radians)
Mat44 Mat44::sRotationZ(float inZ)
{
	// TODO: Could be optimized
	float c = cos(inZ), s = sin(inZ);
	return Mat44(Vec4(c, s, 0, 0), Vec4(-s, c, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1));
}

// Get matrix that translates
Mat44 Mat44::sTranslation(const Vec3 &inV)
{
	return Mat44(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(inV, 1));
}

// Get matrix that scales uniformly
Mat44 Mat44::sScale(float inScale)
{
	return Mat44(Vec4(inScale, 0, 0, 0), Vec4(0, inScale, 0, 0), Vec4(0, 0, inScale, 0), Vec4(0, 0, 0, 1));
}

// Get matrix that scales
Mat44 Mat44::sScale(const Vec3 &inV)
{
	return Mat44(Vec4(inV.GetX(), 0, 0, 0), Vec4(0, inV.GetY(), 0, 0), Vec4(0, 0, inV.GetZ(), 0), Vec4(0, 0, 0, 1));
}

// Get outer product of inV and inV2 (equivalent to inV1 inV2^T)
Mat44 Mat44::sOuterProduct(const Vec3 &inV1, const Vec3 &inV2)
{
	Vec4 v1(inV1, 0);
	return Mat44(v1 * inV2.SplatX(), v1 * inV2.SplatY(), v1 * inV2.SplatZ(), Vec4(0, 0, 0, 1));
}

// Get matrix that represents a cross product A x B = sCrossProduct(A) * B
Mat44 Mat44::sCrossProduct(const Vec3 &inV)
{
	// Zero out the W component
	__m128 zero = _mm_setzero_ps();
	__m128 v = _mm_blend_ps(inV.mValue, zero, 0b1000);

	// Negate
	__m128 min_v = _mm_sub_ps(zero, v);

	return Mat44(
		_mm_shuffle_ps(v, min_v, _MM_SHUFFLE(3, 1, 2, 3)), // [0, z, -y, 0]
		_mm_shuffle_ps(min_v, v, _MM_SHUFFLE(3, 0, 3, 2)), // [-z, 0, x, 0]
		_mm_blend_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 1)), _mm_shuffle_ps(min_v, min_v, _MM_SHUFFLE(3, 3, 0, 3)), 0b0010), // [y, -x, 0, 0]
		Vec4(0, 0, 0, 1));
}

// Assignment
Mat44 &Mat44::operator = (const Mat44 &inM2)
{
	for (int c = 0; c < 4; ++c)
		mCol[c] = inM2.mCol[c];

	return *this;
}

// Comparsion
bool Mat44::operator == (const Mat44 &inM2) const
{
	return UVec4::sAnd(
		UVec4::sAnd(Vec4::sEquals(mCol[0], inM2.mCol[0]), Vec4::sEquals(mCol[1], inM2.mCol[1])),
		UVec4::sAnd(Vec4::sEquals(mCol[2], inM2.mCol[2]), Vec4::sEquals(mCol[3], inM2.mCol[3]))
	).TestAllTrue();
}

// Test if two matrices are close
bool Mat44::IsClose(const Mat44 &inM2, float inMaxDistSq) const
{
	for (int i = 0; i < 4; ++i)
		if (!mCol[i].IsClose(inM2.mCol[i], inMaxDistSq))
			return false;
	return true;
}

// Multiply matrix by matrix
Mat44 Mat44::operator * (const Mat44 &inM) const
{
	Mat44 result;
	for (int i = 0; i < 4; ++i)
	{
		__m128 c = inM.mCol[i].mValue;
		__m128 t = _mm_mul_ps(mCol[0].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 0)));
		t = _mm_add_ps(t, _mm_mul_ps(mCol[1].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1))));
		t = _mm_add_ps(t, _mm_mul_ps(mCol[2].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 2, 2, 2))));
		t = _mm_add_ps(t, _mm_mul_ps(mCol[3].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 3, 3, 3))));
		result.mCol[i].mValue = t;
	}
	return result;
}

// Multiply vector by matrix
Vec3 Mat44::operator * (const Vec3 &inV) const
{
	__m128 t = _mm_mul_ps(mCol[0].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(0, 0, 0, 0)));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[1].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(1, 1, 1, 1))));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[2].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(2, 2, 2, 2))));
	t = _mm_add_ps(t, mCol[3].mValue);
	return t;
}

// Multiply vector by matrix
Vec4 Mat44::operator * (const Vec4 &inV) const
{
	__m128 t = _mm_mul_ps(mCol[0].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(0, 0, 0, 0)));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[1].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(1, 1, 1, 1))));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[2].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(2, 2, 2, 2))));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[3].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(3, 3, 3, 3))));
	return t;
}

// Multiply vector by only 3x3 part of the matrix
Vec3 Mat44::Multiply3x3(const Vec3 &inV) const
{
	__m128 t = _mm_mul_ps(mCol[0].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(0, 0, 0, 0)));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[1].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(1, 1, 1, 1))));
	t = _mm_add_ps(t, _mm_mul_ps(mCol[2].mValue, _mm_shuffle_ps(inV.mValue, inV.mValue, _MM_SHUFFLE(2, 2, 2, 2))));
	return t;
}

// Multiply vector by only 3x3 part of the transpose of the matrix (result = this^T * inV)
Vec3 Mat44::Multiply3x3Transposed(const Vec3 &inV) const
{
	return Transposed3x3().Multiply3x3(inV);
}

// Multiply 3x3 matrix by 3x3 matrix
Mat44 Mat44::Multiply3x3(const Mat44 &inM) const
{
	assert(mCol[0][3] == 0.0f);
	assert(mCol[1][3] == 0.0f);
	assert(mCol[2][3] == 0.0f);

	Mat44 result;
	for (int i = 0; i < 3; ++i)
	{
		__m128 c = inM.mCol[i].mValue;
		__m128 t = _mm_mul_ps(mCol[0].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(0, 0, 0, 0)));
		t = _mm_add_ps(t, _mm_mul_ps(mCol[1].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1))));
		t = _mm_add_ps(t, _mm_mul_ps(mCol[2].mValue, _mm_shuffle_ps(c, c, _MM_SHUFFLE(2, 2, 2, 2))));
		result.mCol[i].mValue = t;
	}
	result.mCol[3].mValue = _mm_set_ps(1, 0, 0, 0);
	return result;
}

// Multiply transpose of 3x3 matrix by 3x3 matrix (result = this^T * inM)
Mat44 Mat44::Multiply3x3LeftTransposed(const Mat44 &inM) const
{
	// Transpose left hand side
	__m128 tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 tmp3 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(3, 2, 3, 2));
	__m128 zero = _mm_setzero_ps();
	__m128 tmp2 = _mm_shuffle_ps(mCol[2].mValue, zero, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 tmp4 = _mm_shuffle_ps(mCol[2].mValue, zero, _MM_SHUFFLE(3, 2, 3, 2));
	Vec4 col0 = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(2, 0, 2, 0));
	Vec4 col1 = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(3, 1, 3, 1));
	Vec4 col2 = _mm_shuffle_ps(tmp3, tmp4, _MM_SHUFFLE(2, 0, 2, 0));

	// Do 3x3 matrix multiply
	Mat44 result;
	result.mCol[0] = col0 * inM.mCol[0].SplatX() + col1 * inM.mCol[0].SplatY() + col2 * inM.mCol[0].SplatZ();
	result.mCol[1] = col0 * inM.mCol[1].SplatX() + col1 * inM.mCol[1].SplatY() + col2 * inM.mCol[1].SplatZ();
	result.mCol[2] = col0 * inM.mCol[2].SplatX() + col1 * inM.mCol[2].SplatY() + col2 * inM.mCol[2].SplatZ();
	result.mCol[3] = Vec4(0, 0, 0, 1);
	return result;
}

// Multiply 3x3 matrix by the transpose of a 3x3 matrix (result = this * inM^T)
Mat44 Mat44::Multiply3x3RightTransposed(const Mat44 &inM) const
{
	assert(mCol[0][3] == 0.0f);
	assert(mCol[1][3] == 0.0f);
	assert(mCol[2][3] == 0.0f);

	Mat44 result;
	result.mCol[0] = mCol[0] * inM.mCol[0].SplatX() + mCol[1] * inM.mCol[1].SplatX() + mCol[2] * inM.mCol[2].SplatX();
	result.mCol[1] = mCol[0] * inM.mCol[0].SplatY() + mCol[1] * inM.mCol[1].SplatY() + mCol[2] * inM.mCol[2].SplatY();
	result.mCol[2] = mCol[0] * inM.mCol[0].SplatZ() + mCol[1] * inM.mCol[1].SplatZ() + mCol[2] * inM.mCol[2].SplatZ();
	result.mCol[3] = Vec4(0, 0, 0, 1);
	return result;
}

// Multiply matrix with float
Mat44 Mat44::operator * (float inV) const
{
	Vec4 multiplier = Vec4::sReplicate(inV);

	Mat44 result;
	for (int c = 0; c < 4; ++c)
		result.mCol[c] = mCol[c] * multiplier;
	return result;
}

// Multiply matrix with float
Mat44 &Mat44::operator *= (float inV)
{
	for (int c = 0; c < 4; ++c)
		mCol[c] *= inV;

	return *this;
}

// Per element addition of matrix
Mat44 Mat44::operator + (const Mat44 &inM) const
{
	Mat44 result;
	for (int i = 0; i < 4; ++i)
		result.mCol[i] = mCol[i] + inM.mCol[i];
	return result;
}

// Negate
Mat44 Mat44::operator - () const
{
	Mat44 result;
	for (int i = 0; i < 4; ++i)
		result.mCol[i] = -mCol[i];
	return result;
}

// Per element subtraction of matrix
Mat44 Mat44::operator - (const Mat44 &inM) const
{
	Mat44 result;
	for (int i = 0; i < 4; ++i)
		result.mCol[i] = mCol[i] - inM.mCol[i];
	return result;
}

// Per element addition of matrix
Mat44 &Mat44::operator += (const Mat44 &inM)
{
	for (int c = 0; c < 4; ++c)
		mCol[c] += inM.mCol[c];

	return *this;
}

// Store matrix to memory
void Mat44::StoreFloat4x4(Float4 *outV) const
{
	for (int c = 0; c < 4; ++c)
		mCol[c].StoreFloat4(outV + c);
}

// Transpose matrix
Mat44 Mat44::Transposed() const
{
	__m128 tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 tmp3 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(3, 2, 3, 2));
	__m128 tmp2 = _mm_shuffle_ps(mCol[2].mValue, mCol[3].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 tmp4 = _mm_shuffle_ps(mCol[2].mValue, mCol[3].mValue, _MM_SHUFFLE(3, 2, 3, 2));

	Mat44 result;
	result.mCol[0].mValue = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(2, 0, 2, 0));
	result.mCol[1].mValue = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(3, 1, 3, 1));
	result.mCol[2].mValue = _mm_shuffle_ps(tmp3, tmp4, _MM_SHUFFLE(2, 0, 2, 0));
	result.mCol[3].mValue = _mm_shuffle_ps(tmp3, tmp4, _MM_SHUFFLE(3, 1, 3, 1));
	return result;
}

// Transpose 3x3 subpart of matrix
Mat44 Mat44::Transposed3x3() const
{
	__m128 zero = _mm_setzero_ps();
	__m128 tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 tmp3 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(3, 2, 3, 2));
	__m128 tmp2 = _mm_shuffle_ps(mCol[2].mValue, zero, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 tmp4 = _mm_shuffle_ps(mCol[2].mValue, zero, _MM_SHUFFLE(3, 2, 3, 2));

	Mat44 result;
	result.mCol[0].mValue = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(2, 0, 2, 0));
	result.mCol[1].mValue = _mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(3, 1, 3, 1));
	result.mCol[2].mValue = _mm_shuffle_ps(tmp3, tmp4, _MM_SHUFFLE(2, 0, 2, 0));
	result.mCol[3] = Vec4(0, 0, 0, 1);
	return result;
}

// Inverse 4x4 matrix
Mat44 Mat44::Inversed() const
{
	// Algorithm from: http://download.intel.com/design/PentiumIII/sml/24504301.pdf
	// Streaming SIMD Extensions - Inverse of 4x4 Matrix
	// Adapted to load data using _mm_shuffle_ps instead of loading from memory
	// Replaced _mm_rcp_ps with _mm_div_ps for better accuracy

	__m128 tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 row1 = _mm_shuffle_ps(mCol[2].mValue, mCol[3].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
	row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
	tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(3, 2, 3, 2));
	__m128 row3 = _mm_shuffle_ps(mCol[2].mValue, mCol[3].mValue, _MM_SHUFFLE(3, 2, 3, 2));
	__m128 row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
	row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	__m128 minor0 = _mm_mul_ps(row1, tmp1);
	__m128 minor1 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
	minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
	minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	__m128 minor3 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
	minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);

	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4E);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	__m128 minor2 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
	minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

	tmp1 = _mm_mul_ps(row0, row1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

	tmp1 = _mm_mul_ps(row0, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
	minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

	__m128 det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
	det = _mm_div_ss(_mm_set_ss(1.0f), det);
	det = _mm_shuffle_ps(det, det, 0x00);
	
	Mat44 result;
	result.mCol[0].mValue = _mm_mul_ps(det, minor0);
	result.mCol[1].mValue = _mm_mul_ps(det, minor1);
	result.mCol[2].mValue = _mm_mul_ps(det, minor2);
	result.mCol[3].mValue = _mm_mul_ps(det, minor3);
	return result;
}

// Inverse 4x4 matrix when it only contains rotation and translation
Mat44 Mat44::InversedRotationTranslation() const
{
	Mat44 m = Transposed3x3();
	m.SetTranslation(-m.Multiply3x3(GetTranslation()));
	return m;
}

// Get the determinant of a 3x3 matrix
float Mat44::GetDeterminant3x3() const
{
	return GetAxisX().Dot(GetAxisY().Cross(GetAxisZ()));
}

// Inverse 3x3 matrix
Mat44 Mat44::Inversed3x3() const
{
	// Adapted from Inversed() to remove 4th column
	// Note: This can be optimized.

	assert(mCol[0][3] == 0.0f);
	assert(mCol[1][3] == 0.0f);
	assert(mCol[2][3] == 0.0f);

	__m128 tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 row1 = _mm_shuffle_ps(mCol[2].mValue, _mm_setzero_ps(), _MM_SHUFFLE(1, 0, 1, 0));
	__m128 row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
	row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
	tmp1 = _mm_shuffle_ps(mCol[0].mValue, mCol[1].mValue, _MM_SHUFFLE(3, 2, 3, 2));
	__m128 row3 = _mm_shuffle_ps(mCol[2].mValue, _mm_set_ps(1, 0, 0, 0), _MM_SHUFFLE(3, 2, 3, 2));
	__m128 row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
	row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	__m128 minor0 = _mm_mul_ps(row1, tmp1);
	__m128 minor1 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
	minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
	minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));

	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4E);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	__m128 minor2 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
	minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

	tmp1 = _mm_mul_ps(row0, row1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);

	tmp1 = _mm_mul_ps(row0, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
	minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));

	__m128 det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
	det = _mm_div_ss(_mm_set_ss(1.0f), det);
	det = _mm_shuffle_ps(det, det, 0x00);
	
	Mat44 result;
	result.mCol[0].mValue = _mm_mul_ps(det, minor0);
	result.mCol[1].mValue = _mm_mul_ps(det, minor1);
	result.mCol[2].mValue = _mm_mul_ps(det, minor2);
	result.mCol[3] = Vec4(0, 0, 0, 1);
	return result;
}

// Get rotation part only (note: retains the first 3 values from the bottom row)
Mat44 Mat44::GetRotation() const
{ 
	assert(mCol[0][3] == 0.0f);
	assert(mCol[1][3] == 0.0f);
	assert(mCol[2][3] == 0.0f);

	return Mat44(mCol[0], mCol[1], mCol[2], Vec4(0, 0, 0, 1)); 
}

// Get rotation part only (note: also clears the bottom row)
Mat44 Mat44::GetRotationSafe() const
{ 
	__m128 zero = _mm_setzero_ps(); 
	return Mat44(_mm_blend_ps(mCol[0].mValue, zero, 8), 
				 _mm_blend_ps(mCol[1].mValue, zero, 8), 
				 _mm_blend_ps(mCol[2].mValue, zero, 8), 
				 Vec4(0, 0, 0, 1)); 
}

// Scale a matrix: result = Mat44::sScale(inScale) * this
Mat44 Mat44::Scaled(const Vec3 &inScale) const
{
	Vec4 scale(inScale, 1);
	return Mat44(scale * mCol[0], scale * mCol[1], scale * mCol[2], scale * mCol[3]);
}
