#pragma once

#include <Math/Vec4.h>

// Define half float constant values
#define HALF_FLT_MAX			0x7bff
#define HALF_FLT_MAX_NEGATIVE	0xfbff
#define HALF_FLT_INF			0x7c00
#define HALF_FLT_INF_NEGATIVE	0xfc00

// Define rounding modes
enum ERoundingMode
{
	ROUND_TO_NEG_INF = _MM_FROUND_TO_NEG_INF,
	ROUND_TO_POS_INF = _MM_FROUND_TO_POS_INF,
	ROUND_TO_NEAREST = _MM_FROUND_TO_NEAREST_INT,
};

// Convert a float to a half float
template <int RoundingMode>
f_inline uint16 FloatToHalfFloat(float inV)
{
	union
	{
		__m128i u128;
		uint16	u16[8];
	} hf;	
	hf.u128 = _mm_cvtps_ph(_mm_load_ss(&inV), RoundingMode);
	return hf.u16[0];
}
