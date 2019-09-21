#ifndef _MATH_H_
#define _MATH_H_

// Typedefs
typedef unsigned int uint;
typedef float<3> float3;
typedef bool<3> bool3;
typedef uint<3> uint3;

// Constants
#define EPSILON 1.0e-20f
#define FLT_MAX 3.402823466e38f

static const float3 zero3 = { 0, 0, 0 };
static const float3 epsilon3 = { EPSILON, EPSILON, EPSILON };
static const float3 flt_min3 = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
static const float3 flt_max3 = { FLT_MAX, FLT_MAX, FLT_MAX };
static const uint3 uint3_one = { 1, 1, 1 };
static const uint3 uint3_all_but_lowest_bit_set = { 0xfffffff7, 0xfffffff7, 0xfffffff7 };

// Dot product
inline float dot(float3 V1, float3 V2)
{
	return V1.x * V2.x + V1.y * V2.y + V1.z * V2.z;
}

// Cross product
inline float3 cross(float3 V1, float3 V2)
{
	float3 ret;
    ret.x = V1.y * V2.z - V1.z * V2.y;
    ret.y = V1.z * V2.x - V1.x * V2.z;
    ret.z = V1.x * V2.y - V1.y * V2.x;
	return ret;
}

// Absolute
inline float3 abs(float3 V)
{
	float3 ret;
	ret.x = abs(V.x);
	ret.y = abs(V.y);
	ret.z = abs(V.z);
	return ret;
}

// Min
inline float3 min(float3 V1, float3 V2)
{
	float3 ret;
	ret.x = min(V1.x, V2.x);
	ret.y = min(V1.y, V2.y);
	ret.z = min(V1.z, V2.z);
	return ret;
}

// Max
inline float3 max(float3 V1, float3 V2)
{
	float3 ret;
	ret.x = max(V1.x, V2.x);
	ret.y = max(V1.y, V2.y);
	ret.z = max(V1.z, V2.z);
	return ret;
}

// Select by component
inline float3 select(bool3 C, float3 V1, float3 V2)
{
	float3 ret;
	ret.x = C.x? V1.x : V2.x;
	ret.y = C.y? V1.y : V2.y;
	ret.z = C.z? V1.z : V2.z;
	return ret;
}

// If any bool is true
inline bool any(bool3 V)
{
	return V.x || V.y || V.z;
}

// Convert float -> int
inline uint3 asuint(float3 V)
{
	uint3 ret;
	ret.x = intbits(V.x);
	ret.y = intbits(V.y);
	ret.z = intbits(V.z);
	return ret;
}

// Convert int -> float
inline float3 asfloat(uint3 V)
{
	float3 ret;
	ret.x = floatbits(V.x);
	ret.y = floatbits(V.y);
	ret.z = floatbits(V.z);
	return ret;
}

// Operator ||
inline bool3 or(bool3 V1, bool3 V2)
{
	bool3 ret;
	ret.x = V1.x || V2.x;
	ret.y = V1.y || V2.y;
	ret.z = V1.z || V2.z;
	return ret;
}

// Operator &&
inline bool3 and(bool3 V1, bool3 V2)
{
	bool3 ret;
	ret.x = V1.x && V2.x;
	ret.y = V1.y && V2.y;
	ret.z = V1.z && V2.z;
	return ret;
}

// Reciprocal
inline float3 rcp(float3 V)
{
	float3 ret;
	ret.x = rcp(V.x);
	ret.y = rcp(V.y);
	ret.z = rcp(V.z);
	return ret;
}

#endif
