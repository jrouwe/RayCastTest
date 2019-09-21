#pragma once

#include <Math/Vec4.h>

class UVec4
{
public:
	// Constructor
	f_inline					UVec4()												{ }
	f_inline					UVec4(const UVec4 &inRHS) : mValue(inRHS.mValue)	{ }
	f_inline					UVec4(__m128i inRHS) : mValue(inRHS)				{ }

	// Create a vector from 4 integer components
	f_inline					UVec4(uint32 inX, uint32 inY, uint32 inZ, uint32 inW);

	// Comparison
	f_inline bool				operator == (const UVec4 &inV2) const;
	f_inline bool				operator != (const UVec4 &inV2) const				{ return !(*this == inV2); }

	// Swizzle the elements in inV
	template<uint32 SwizzleX, uint32 SwizzleY, uint32 SwizzleZ, uint32 SwizzleW>
	f_inline UVec4				Swizzle() const;

	// Vector with all zeros
	static f_inline UVec4		sZero();

	// Replicate int inV across all components
	static f_inline UVec4		sReplicate(uint32 inV);

	// Load 1 int from memory and place it in the X component, zeros Y, Z and W
	static f_inline UVec4		sLoadInt(const uint32 *inV);

	// Load 4 ints from memory
	static f_inline UVec4		sLoadInt4(const uint32 *inV);

	// Load 4 ints from memory, aligned to 16 bytes
	static f_inline UVec4		sLoadInt4Aligned(const uint32 *inV);

	// Load 3 bytes of inData into X, Y, Z (as ints)
	static f_inline UVec4		sLoadBytes3(const uint32 *inData);

	// Gather 4 ints from memory at inBase + inOffsets[i] * Scale
	template <const int Scale>
	static f_inline UVec4		sGatherInt4(const uint32 *inBase, const UVec4 &inOffsets);

	// Return the minimum value of each of the components
	static f_inline UVec4		sMin(const UVec4 &inV1, const UVec4 &inV2);

	// Return the maximum of each of the components
	static f_inline UVec4		sMax(const UVec4 &inV1, const UVec4 &inV2);

	// Equals (component wise)
	static f_inline UVec4		sEquals(const UVec4 &inV1, const UVec4 &inV2);

	// Component wise select, returns inV1 when highest bit of inControl = 0 and inV2 when highest bit of inControl = 1
	static f_inline UVec4		sSelect(const UVec4 &inV1, const UVec4 &inV2, const UVec4 &inControl);

	// Logical or (component wise)
	static f_inline UVec4		sOr(const UVec4 &inV1, const UVec4 &inV2);

	// Logical xor (component wise)
	static f_inline UVec4		sXor(const UVec4 &inV1, const UVec4 &inV2);

	// Logical and (component wise)
	static f_inline UVec4		sAnd(const UVec4 &inV1, const UVec4 &inV2);

	// Logical not (component wise)
	static f_inline UVec4		sNot(const UVec4 &inV1);

	// Sorts 4 elements so that the True values go first (highest bit set), sorts ioIndex at the same time
	// Based on a sorting network: http://en.wikipedia.org/wiki/Sorting_network
	static f_inline void		sSort4True(UVec4 &ioValue, UVec4 &ioIndex);

	// Get individual components
	f_inline uint32				GetX() const										{ return mU32[0]; }
	f_inline uint32				GetY() const										{ return mU32[1]; }
	f_inline uint32				GetZ() const										{ return mU32[2]; }
	f_inline uint32				GetW() const										{ return mU32[3]; }

	// Set individual components
	f_inline void				SetX(uint32 inX)									{ mU32[0] = inX; }
	f_inline void				SetY(uint32 inY)									{ mU32[1] = inY; }
	f_inline void				SetZ(uint32 inZ)									{ mU32[2] = inZ; }
	f_inline void				SetW(uint32 inW)									{ mU32[3] = inW; }

	// Get component by index
	f_inline uint32				operator [] (uint inCoordinate) const				{ assert(inCoordinate < 4); return mU32[inCoordinate]; }
	f_inline uint32 &			operator [] (uint inCoordinate)						{ assert(inCoordinate < 4); return mU32[inCoordinate]; }

	// Multiplies each of the 4 integer components with an integer (discards any overflow)
	f_inline UVec4				operator * (const UVec4 &inV2) const;

	// Adds an integer value to all integer components (discards any overflow)
	f_inline UVec4				operator + (const UVec4 &inV2);

	// Add two integer vectors (component wise)
	f_inline UVec4 &			operator += (const UVec4 &inV2);

	// Replicate the X component to all components
	f_inline UVec4				SplatX() const;

	// Replicate the Y component to all components
	f_inline UVec4				SplatY() const;

	// Replicate the Z component to all components
	f_inline UVec4				SplatZ() const;

	// Replicate the W component to all components
	f_inline UVec4				SplatW() const;

	// Convert each component from an int to a float
	f_inline Vec4				ToFloat() const;

	// Reinterpret UVec4 as a Vec4 (doesn't change the bits)
	f_inline Vec4				ReinterpretAsFloat() const;

	// Convert 4 half floats (lower 64 bits) to floats
	f_inline Vec4				HalfFloatToFloat() const;

	// Store 4 ints to memory
	f_inline void				StoreInt4(uint32 *outV) const;

	// Test if any of the components are true (true is when highest bit of component is set)
	f_inline bool				TestAnyTrue() const;

	// Test if any of X, Y or Z components are true (true is when highest bit of component is set)
	f_inline bool				TestAnyXYZTrue() const;

	// Test if all components are true (true is when highest bit of component is set)
	f_inline bool				TestAllTrue() const;

	// Test if X, Y and Z components are true (true is when highest bit of component is set)
	f_inline bool				TestAllXYZTrue() const;

	// Count the number of components that are true (true is when highest bit of component is set)
	f_inline int				CountTrues() const;

	// Store if X is true in bit 0, Y in bit 1, Z in bit 2 and W in bit 3 (true is when highest bit of component is set)
	f_inline int				GetTrues() const;

	// Shift all components by inCount bits to the left (filling with zeros from the left)
	f_inline UVec4				LogicalShiftLeft(int inCount) const;

	// Shift all components by inCount bits to the right (filling with zeros from the right)
	f_inline UVec4				LogicalShiftRight(int inCount) const;

	// Shift all components by inCount bits to the right (shifting in the value of the highest bit)
	f_inline UVec4				ArithmeticShiftRight(int inCount) const;

	// Takes the lower 4 16 bits and expands them to X, Y, Z and W
	f_inline UVec4				Expand4Uint16Lo() const;	

	// Takes the upper 4 16 bits and expands them to X, Y, Z and W
	f_inline UVec4				Expand4Uint16Hi() const;
	
	// Takes byte 0 .. 3 and expands them to X, Y, Z and W
	f_inline UVec4				Expand4Byte0() const;

	// Takes byte 4 .. 7 and expands them to X, Y, Z and W
	f_inline UVec4				Expand4Byte4() const;

	// Takes byte 8 .. 11 and expands them to X, Y, Z and W
	f_inline UVec4				Expand4Byte8() const;
	
	// Takes byte 12 .. 15 and expands them to X, Y, Z and W
	f_inline UVec4				Expand4Byte12() const;

	// Shift vector components by 4 - Count floats to the left, so if Count = 1 the resulting vector is (Y, Z, W, 0)
	f_inline UVec4				ShiftComponents4Minus(int inCount) const;

	union
	{
		__m128i					mValue;
		uint32					mU32[4];
	};

private:
	static const UVec4			sFourMinusXShuffle[];
};

// Load 4 ints from memory, conditionally aligned to 16 bytes
template <bool Aligned>
static f_inline UVec4 UVec4LoadInt4ConditionallyAligned(const uint32 *inV)
{
	return UVec4::sLoadInt4(inV);
}

template <>
f_inline UVec4 UVec4LoadInt4ConditionallyAligned<true>(const uint32 *inV)
{
	return UVec4::sLoadInt4Aligned(inV);
}

#include "UVec4.inl"
