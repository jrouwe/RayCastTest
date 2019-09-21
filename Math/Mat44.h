#pragma once

class Vec3;
class Vec4;

class Mat44
{
public:
	// Constructor
	f_inline					Mat44()													{ }
	f_inline					Mat44(const Vec4 &inC1, const Vec4 &inC2, const Vec4 &inC3, const Vec4 &inC4);
	f_inline					Mat44(const Mat44 &inM2);
	f_inline					Mat44(__m128 inC1, __m128 inC2, __m128 inC3, __m128 inC4);

	// Zero matrix
	static f_inline Mat44		sZero();

	// Identity matrix
	static f_inline Mat44		sIdentity();

	// Matrix filled with NaN's
	static f_inline Mat44		sNaN();

	// Load 16 floats from memory
	static f_inline Mat44		sLoadFloat4x4(const Float4 *inV);

	// Load 16 floats from memory, 16 bytes aligned
	static f_inline Mat44		sLoadFloat4x4Aligned(const Float4 *inV);

	// Rotate around X, Y or Z axis (angle in radians)
	static f_inline Mat44		sRotationX(float inX);
	static f_inline Mat44		sRotationY(float inY);
	static f_inline Mat44		sRotationZ(float inZ);

	// Get matrix that translates
	static f_inline Mat44		sTranslation(const Vec3 &inV);

	// Get matrix that scales uniformly
	static f_inline Mat44		sScale(float inScale);

	// Get matrix that scales (produces a matrix with (inV, 1) on its diagonal)
	static f_inline Mat44		sScale(const Vec3 &inV);

	// Get outer product of inV and inV2 (equivalent to inV1 inV2^T)
	static f_inline Mat44		sOuterProduct(const Vec3 &inV1, const Vec3 &inV2);

	// Get matrix that represents a cross product A x B = sCrossProduct(A) * B
	static f_inline Mat44		sCrossProduct(const Vec3 &inV);

	// Get float component by element index
	f_inline float				operator () (uint inRow, uint inColumn) const			{ assert(inRow < 4); assert(inColumn < 4); return mCol[inColumn].mF32[inRow]; }
	f_inline float &			operator () (uint inRow, uint inColumn)					{ assert(inRow < 4); assert(inColumn < 4); return mCol[inColumn].mF32[inRow]; }
	
	// Assignment
	f_inline Mat44 &			operator = (const Mat44 &inM2);

	// Comparsion
	f_inline bool				operator == (const Mat44 &inM2) const;
	f_inline bool				operator != (const Mat44 &inM2) const					{ return !(*this == inM2); }

	// Test if two matrices are close
	f_inline bool				IsClose(const Mat44 &inM2, float inMaxDistSq = 1.0e-12f) const;

	// Multiply matrix by matrix
	f_inline Mat44				operator * (const Mat44 &inM) const;

	// Multiply vector by matrix
	f_inline Vec3				operator * (const Vec3 &inV) const;
	f_inline Vec4				operator * (const Vec4 &inV) const;

	// Multiply vector by only 3x3 part of the matrix
	f_inline Vec3				Multiply3x3(const Vec3 &inV) const;

	// Multiply vector by only 3x3 part of the transpose of the matrix (result = this^T * inV)
	f_inline Vec3				Multiply3x3Transposed(const Vec3 &inV) const;

	// Multiply 3x3 matrix by 3x3 matrix
	f_inline Mat44				Multiply3x3(const Mat44 &inM) const;

	// Multiply transpose of 3x3 matrix by 3x3 matrix (result = this^T * inM)
	f_inline Mat44				Multiply3x3LeftTransposed(const Mat44 &inM) const;

	// Multiply 3x3 matrix by the transpose of a 3x3 matrix (result = this * inM^T)
	f_inline Mat44				Multiply3x3RightTransposed(const Mat44 &inM) const;

	// Multiply matrix with float
	f_inline Mat44				operator * (float inV) const;
	friend f_inline Mat44		operator * (float inV, const Mat44 &inM)				{ return inM * inV; }

	// Multiply matrix with float
	f_inline Mat44 &			operator *= (float inV);

	// Per element addition of matrix
	f_inline Mat44				operator + (const Mat44 &inM) const;

	// Negate
	f_inline Mat44				operator - () const;

	// Per element subtraction of matrix
	f_inline Mat44				operator - (const Mat44 &inM) const;

	// Per element addition of matrix
	f_inline Mat44 &			operator += (const Mat44 &inM);

	// Access to the columns
	f_inline const Vec3			GetAxisX() const										{ return Vec3(mCol[0]); }
	f_inline void				SetAxisX(const Vec3 &inV)								{ mCol[0] = Vec4(inV, 0.0f); }
	f_inline const Vec3			GetAxisY() const										{ return Vec3(mCol[1]); }
	f_inline void				SetAxisY(const Vec3 &inV)								{ mCol[1] = Vec4(inV, 0.0f); }
	f_inline const Vec3			GetAxisZ() const										{ return Vec3(mCol[2]); }
	f_inline void				SetAxisZ(const Vec3 &inV)								{ mCol[2] = Vec4(inV, 0.0f); }
	f_inline const Vec3			GetTranslation() const									{ return Vec3(mCol[3]); }
	f_inline void				SetTranslation(const Vec3 &inV)							{ mCol[3] = Vec4(inV, 1.0f); }
	f_inline const Vec3			GetDiagonal3() const									{ return Vec3(mCol[0][0], mCol[1][1], mCol[2][2]); }
	f_inline void				SetDiagonal3(const Vec3 &inV)							{ mCol[0][0] = inV.GetX(); mCol[1][1] = inV.GetY(); mCol[2][2] = inV.GetZ(); }
	f_inline const Vec4			GetDiagonal4() const									{ return Vec4(mCol[0][0], mCol[1][1], mCol[2][2], mCol[3][3]); }
	f_inline void				SetDiagonal4(const Vec4 &inV)							{ mCol[0][0] = inV.GetX(); mCol[1][1] = inV.GetY(); mCol[2][2] = inV.GetZ(); mCol[3][3] = inV.GetW(); }
	f_inline const Vec3			GetColumn3(uint inCol) const							{ assert(inCol < 4); return Vec3(mCol[inCol]); }
	f_inline void				SetColumn3(uint inCol, const Vec3 &inV)					{ assert(inCol < 4); mCol[inCol] = Vec4(inV, inCol == 3? 1.0f : 0.0f); }
	f_inline const Vec4			GetColumn4(uint inCol) const							{ assert(inCol < 4); return mCol[inCol]; }
	f_inline void				SetColumn4(uint inCol, const Vec4 &inV)					{ assert(inCol < 4); mCol[inCol] = inV; }

	// Store matrix to memory
	f_inline void				StoreFloat4x4(Float4 *outV) const;

	// Transpose matrix
	f_inline Mat44				Transposed() const;

	// Transpose 3x3 subpart of matrix
	f_inline Mat44				Transposed3x3() const;

	// Inverse 4x4 matrix
	f_inline Mat44				Inversed() const;

	// Inverse 4x4 matrix when it only contains rotation and translation
	f_inline Mat44				InversedRotationTranslation() const;

	// Get the determinant of a 3x3 matrix
	f_inline float				GetDeterminant3x3() const;

	// Inverse 3x3 matrix
	f_inline Mat44				Inversed3x3() const;

	// Get rotation part only (note: retains the first 3 values from the bottom row)
	f_inline Mat44				GetRotation() const;

	// Get rotation part only (note: also clears the bottom row)
	f_inline Mat44				GetRotationSafe() const;

	// Get matrix that transforms a direction with the same transform as this matrix (length is not preserved)
	f_inline const Mat44		GetDirectionPreservingMatrix() const					{ return GetRotation().Inversed3x3().Transposed3x3(); }

	// Scale a matrix: result = Mat44::sScale(inScale) * this
	f_inline Mat44				Scaled(const Vec3 &inScale) const;

	// To String
	friend ostream &			operator << (ostream &inStream, const Mat44 &inM)
	{
		inStream << inM.mCol[0] << ", " << inM.mCol[1] << ", " << inM.mCol[2] << ", " << inM.mCol[3];
		return inStream;
	}

private:
	Vec4						mCol[4];												// Column
};

#include "Mat44.inl"
