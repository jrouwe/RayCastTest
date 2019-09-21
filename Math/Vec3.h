#pragma once

#include <Math/Float3.h>
#include <Math/Swizzle.h>

class Vec4;
class UVec4;

// 3 component vector (stored as 4 vectors). 
// Note that functions may modify the 4th component to any value during calculations.
class Vec3
{
public:
	// Constructor
	f_inline					Vec3()											{ }
	f_inline					Vec3(const Vec3 &inRHS) : mValue(inRHS.mValue)	{ }
	explicit f_inline			Vec3(const Vec4 &inRHS);
	f_inline					Vec3(__m128 inRHS) : mValue(inRHS)				{ }

	// Load 3 floats from memory
	explicit f_inline			Vec3(const Float3 &inV);

	// Create a vector from 3 components
	f_inline					Vec3(float inX, float inY, float inZ);

	// Vector with all zeros
	static f_inline Vec3		sZero();

	// Vector with all NaN's
	static f_inline Vec3		sNaN();

	// Vectors with the principal axis
	static f_inline Vec3		sAxisX()										{ return Vec3(1, 0, 0); }
	static f_inline Vec3		sAxisY()										{ return Vec3(0, 1, 0); }
	static f_inline Vec3		sAxisZ()										{ return Vec3(0, 0, 1); }

	// Replicate inV across all components
	static f_inline Vec3		sReplicate(float inV);
		
	// Load 3 floats from memory (reads 32 bits extra which it doesn't use)
	static f_inline Vec3		sLoadFloat3Unsafe(const Float3 &inV);

	// Return the minimum value of each of the components
	static f_inline Vec3		sMin(const Vec3 &inV1, const Vec3 &inV2);

	// Return the maximum of each of the components
	static f_inline Vec3		sMax(const Vec3 &inV1, const Vec3 &inV2);

	// Clamp a vector between min and max (component wise)
	static f_inline Vec3		sClamp(const Vec3 &inV, const Vec3 &inMin, const Vec3 &inMax);

	// Equals (component wise)
	static f_inline UVec4		sEquals(const Vec3 &inV1, const Vec3 &inV2);

	// Less than (component wise)
	static f_inline UVec4		sLess(const Vec3 &inV1, const Vec3 &inV2);

	// Less than or equal (component wise)
	static f_inline UVec4		sLessOrEqual(const Vec3 &inV1, const Vec3 &inV2);

	// Greater than (component wise)
	static f_inline UVec4		sGreater(const Vec3 &inV1, const Vec3 &inV2);

	// Greater than or equal (component wise)
	static f_inline UVec4		sGreaterOrEqual(const Vec3 &inV1, const Vec3 &inV2);

	// Calculates inMul1 * inMul2 + inAdd
	static f_inline Vec3		sFusedMultiplyAdd(const Vec3 &inMul1, const Vec3 &inMul2, const Vec3 &inAdd);

	// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
	static f_inline Vec3		sSelect(const Vec3 &inV1, const Vec3 &inV2, const UVec4 &inControl);

	// Logical or (component wise)
	static f_inline Vec3		sOr(const Vec3 &inV1, const Vec3 &inV2);

	// Logical xor (component wise)
	static f_inline Vec3		sXor(const Vec3 &inV1, const Vec3 &inV2);

	// Logical and (component wise)
	static f_inline Vec3		sAnd(const Vec3 &inV1, const Vec3 &inV2);

	// Get unit vector given spherical coordinates
	// inTheta e [0, pi] is angle between vector and z-axis
	// inPhi e [0, 2 pi] is the angle in the xy-plane starting from the x axis and rotating counter clockwise around the z-axis
	static f_inline Vec3		sUnitSpherical(float inTheta, float inPhi);

	// Get random unit vector
	template <class Random>
	static inline Vec3			sRandom(Random &inRandom);
	
	// Get individual components
	f_inline float				GetX() const									{ return _mm_cvtss_f32(mValue); }
	f_inline float				GetY() const									{ return mF32[1]; }
	f_inline float				GetZ() const									{ return mF32[2]; }
	
	// Set individual components
	f_inline void				SetX(float inX)									{ mF32[0] = inX; }
	f_inline void				SetY(float inY)									{ mF32[1] = inY; }
	f_inline void				SetZ(float inZ)									{ mF32[2] = inZ; }
	
	// Get float component by index
	f_inline float				operator [] (uint inCoordinate) const			{ assert(inCoordinate < 3); return mF32[inCoordinate]; }
	f_inline float &			operator [] (uint inCoordinate)					{ assert(inCoordinate < 3); return mF32[inCoordinate]; }

	// Assignment
	f_inline Vec3 &				operator = (const Vec3 &inV2);

	// Comparison
	f_inline bool				operator == (const Vec3 &inV2) const;
	f_inline bool				operator != (const Vec3 &inV2) const			{ return !(*this == inV2); }

	// Test if two vectors are close
	f_inline bool				IsClose(const Vec3 &inV2, float inMaxDistSq = 1.0e-12f) const;

	// Test if vector is near zero
	f_inline bool				IsNearZero(float inMaxDistSq = 1.0e-12f) const;

	// Test if vector is normalized
	f_inline bool				IsNormalized(float inTolerance = 1.0e-6f) const;

	// Test if vector contains NaN elements
	f_inline bool				IsNaN() const;

	// Multiply two float vectors (component wise)
	f_inline Vec3				operator * (const Vec3 &inV2) const;

	// Multiply vector with float
	f_inline Vec3				operator * (float inV2) const;

	// Multiply vector with float
	friend f_inline Vec3		operator * (float inV1, const Vec3 &inV2);

	// Divide vector by float
	f_inline Vec3				operator / (float inV2) const;

	// Multiply vector with float
	f_inline Vec3 &				operator *= (float inV2);

	// Multiply vector with vector
	f_inline Vec3 &				operator *= (const Vec3 &inV2);

	// Divide vector by float
	f_inline Vec3 &				operator /= (float inV2);

	// Add two float vectors (component wise)
	f_inline Vec3				operator + (const Vec3 &inV2) const;

	// Add two float vectors (component wise)
	f_inline Vec3 &				operator += (const Vec3 &inV2);

	// Negate
	f_inline Vec3				operator - () const;

	// Subtract two float vectors (component wise)
	f_inline Vec3				operator - (const Vec3 &inV2) const;

	// Add two float vectors (component wise)
	f_inline Vec3 &				operator -= (const Vec3 &inV2);

	// Divide (component wise)
	f_inline Vec3				operator / (const Vec3 &inV2) const;

	// Swizzle the elements in inV
	template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW = SWIZZLE_UNUSED>
	f_inline Vec3				Swizzle() const;

	// Replicate the X component to all components
	f_inline Vec4				SplatX() const;

	// Replicate the Y component to all components
	f_inline Vec4				SplatY() const;

	// Replicate the Z component to all components
	f_inline Vec4				SplatZ() const;

	// Get index of component with lowest value
	f_inline int				GetLowestComponentIndex() const;

	// Get index of component with highest value
	f_inline int				GetHighestComponentIndex() const;

	// Return the absolute value of each of the components
	f_inline Vec3				Abs() const;

	// Reciprocal vector (1 / value) for each of the components
	f_inline Vec3				Reciprocal() const;

	// Cross product
	f_inline Vec3				Cross(const Vec3 &inV2) const;

	// Dot product, returns the dot product in X, Y and Z components
	f_inline Vec3				DotV(const Vec3 &inV2) const;

	// Dot product
	f_inline float				Dot(const Vec3 &inV2) const;

	// Squared length of vector
	f_inline float				LengthSq() const;

	// Length of vector
	f_inline float				Length() const;

	// Normalize vector
	f_inline Vec3				Normalized() const;

	// Store 3 floats to memory
	f_inline void				StoreFloat3(Float3 *outV) const;

	// Convert each component from a float to an int
	f_inline UVec4				ToInt() const;

	// Reinterpret Vec3 as a UVec4 (doesn't change the bits)
	f_inline UVec4				ReinterpretAsInt() const;

	// Get the minimum of X, Y and Z
	f_inline float				ReduceMin() const;

	// Get the maximum of X, Y and Z
	f_inline float				ReduceMax() const;

	// Component wise square root
	f_inline Vec3				Sqrt() const;

	// Get vector that is perpendicular to this vector
	f_inline Vec3				GetPerpendicular() const;

	// Get vector that contains the sign of each element (returns 1.0f if positive, -1.0f if negative)
	f_inline Vec3				GetSign() const;

	// To String
	friend ostream &			operator << (ostream &inStream, const Vec3 &inV)
	{
		inStream << inV.mF32[0] << ", " << inV.mF32[1] << ", " << inV.mF32[2];
		return inStream;
	}

	union
	{
		__m128					mValue;
		float					mF32[4];
	};
};

#include "Vec3.inl"
