#pragma once

#include <Math/Vec8.h>

class UVec8
{
public:
	f_inline					UVec8()												{ }
	f_inline					UVec8(const UVec8 &inRHS) : mValue(inRHS.mValue)	{ }
	f_inline					UVec8(__m256i inRHS) : mValue(inRHS)				{ }

	// Set 256 bit vector from 2 128 bit vectors
	f_inline					UVec8(const UVec4 &inLo, const UVec4 &inHi);
	
	// Comparison
	f_inline bool				operator == (const UVec8 &inV2) const;
	f_inline bool				operator != (const UVec8 &inV2) const				{ return !(*this == inV2); }

	// Replicate int across all components
	static f_inline UVec8		sReplicate(uint32 inV);

	// Replicate the X component of inV to all components
	static f_inline UVec8		sSplatX(const UVec4 &inV);

	// Replicate the Y component of inV to all components
	static f_inline UVec8		sSplatY(const UVec4 &inV);

	// Replicate the Z component of inV to all components
	static f_inline UVec8		sSplatZ(const UVec4 &inV);

	// Equals (component wise)
	static f_inline UVec8		sEquals(const UVec8 &inV1, const UVec8 &inV2);

	// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
	static f_inline UVec8		sSelect(const UVec8 &inV1, const UVec8 &inV2, const UVec8 &inControl);

	// Logical or
	static f_inline UVec8		sOr(const UVec8 &inV1, const UVec8 &inV2);

	// Logical xor
	static f_inline UVec8		sXor(const UVec8 &inV1, const UVec8 &inV2);

	// Logical and
	static f_inline UVec8		sAnd(const UVec8 &inV1, const UVec8 &inV2);

	// Get float component by index
	f_inline uint32				operator [] (uint inCoordinate) const			{ assert(inCoordinate < 8); return mU32[inCoordinate]; }
	f_inline uint32 &			operator [] (uint inCoordinate)					{ assert(inCoordinate < 8); return mU32[inCoordinate]; }
	
	// 256 bit variant of Vec::Swizzle (no cross 128 bit lane swizzle)
	template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
	f_inline UVec8				Swizzle() const;

	// Test if any of the components are true (true is when highest bit of component is set)
	f_inline bool				TestAnyTrue() const;

	// Test if all components are true (true is when highest bit of component is set)
	f_inline bool				TestAllTrue() const;

	// Fetch the lower 128 bit from a 256 bit variable
	f_inline UVec4				LowerVec4() const;

	// Fetch the higher 128 bit from a 256 bit variable
	f_inline UVec4				UpperVec4() const;

	// Converts int to float
	f_inline Vec8				ToFloat() const;
	
	// Shift all components by inCount bits to the left (filling with zeros from the left)
	f_inline UVec8				LogicalShiftLeft(int inCount) const;

	// Shift all components by inCount bits to the right (filling with zeros from the right)
	f_inline UVec8				LogicalShiftRight(int inCount) const;

	// Shift all components by inCount bits to the right (shifting in the value of the highest bit)
	f_inline UVec8				ArithmeticShiftRight(int inCount) const;

	union
	{
		__m256i					mValue;
		uint32					mU32[8];
	};
};

#include "UVec8.inl"
