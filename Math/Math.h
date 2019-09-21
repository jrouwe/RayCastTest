#pragma once

// Constants
#define F_PI       3.14159265358979323846f

// Convert a value from degrees to radians
inline constexpr float DegreesToRadians(float inV)
{
	return inV * (F_PI / 180.0f);
}

// Convert a value from radians to degrees
inline constexpr float RadiansToDegrees(float inV)
{
	return inV * (180.0f / F_PI);
}

// Clamp a value between two values
template <typename T>
inline constexpr T Clamp(T inV, T inMin, T inMax)
{
	return min(max(inV, inMin), inMax);
}

// Square a value
template <typename T>
inline constexpr T Square(T inV)
{
	return inV * inV;
}

// Returns inV^3
template <typename T>
inline constexpr T Cubed(T inV)
{
	return inV * inV * inV;
}

// Get the sign of a value
template <typename T>
inline constexpr T Sign(T inV)
{
	return inV < 0? T(-1) : T(1);
}

// Check if inV is a power of 2
template <typename T>
inline constexpr bool IsPowerOf2(T inV)
{
	return (inV & (inV - 1)) == 0;
}

// Align inV up to the next inAlignment bytes
template <typename T>
inline T AlignUp(T inV, uint64 inAlignment)
{
	assert(IsPowerOf2(inAlignment));
	return T((uint64(inV) + inAlignment - 1) & ~(inAlignment - 1));
}

// Check if inV is inAlignment aligned
template <typename T>
inline bool IsAligned(T inV, uint64 inAlignment)
{
	assert(IsPowerOf2(inAlignment));
	return (uint64(inV) & (inAlignment - 1)) == 0;
}

// Compute number of trailing zero bits (how many low bits are zero)
inline uint CountTrailingZeros(uint32 inValue)
{
	return _tzcnt_u32(inValue);
}

// Compute the number of leading zero bits (how many high bits are zero)
inline uint CountLeadingZeros(uint32 inValue)
{
	return _lzcnt_u32(inValue);
}

// Count the number of 1 bits in a value
inline uint CountBits(uint32 inValue)
{
	return _mm_popcnt_u32(inValue);
}
