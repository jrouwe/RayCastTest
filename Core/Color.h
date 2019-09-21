#pragma once

class Color
{
public:
	// Constructors
	inline					Color()																	{ }
							Color(const Color &inRHS)												: r(inRHS.r), g(inRHS.g), b(inRHS.b), a(inRHS.a) { }
	explicit				Color(uint32 inColor)													: mU32(inColor) { }
							Color(uint8 inRed, uint8 inGreen, uint8 inBlue, uint8 inAlpha = 255)	: r(inRed), g(inGreen), b(inBlue), a(inAlpha) { }
							Color(const Color &inRHS, uint8 inAlpha)								: r(inRHS.r), g(inRHS.g), b(inRHS.b), a(inAlpha) { }
											
	// Assignment			
	inline const Color &	operator = (const Color &inRHS)											{ mU32 = inRHS.mU32; return *this; }
							
	// Comparison			
	inline bool				operator == (const Color &inRHS) const									{ return mU32 == inRHS.mU32; }
	inline bool				operator != (const Color &inRHS) const									{ return mU32 != inRHS.mU32; }
	
	// Convert to uint32
	uint32					GetUInt32() const														{ return mU32; }

	// Element access, 0 = red, 1 = green, 2 = blue, 3 = alpha
	inline uint8			operator () (uint inIdx) const											{ assert(inIdx < 4); return (&r)[inIdx]; }
	inline uint8 &			operator () (uint inIdx)												{ assert(inIdx < 4); return (&r)[inIdx]; }

	// Convert to Vec4 with range [0, 1]
	inline Vec4				ToVec4() const															{ return Vec4(r, g, b, a) / 255.0f; }

	// Get grayscale intensity of color
	inline uint8			GetIntensity() const													{ return uint8((uint32(r) * 54 + g * 183 + b * 19) >> 8); }

	// Get a visually distinct color
	static Color			sGetDistinctColor(int inIndex);

	// Predefined colors
	static const Color		sBlack;
	static const Color		sDarkRed;
	static const Color		sRed;
	static const Color		sDarkGreen;
	static const Color		sGreen;
	static const Color		sDarkBlue;
	static const Color		sBlue;
	static const Color		sYellow;
	static const Color		sPurple;
	static const Color		sCyan;
	static const Color		sOrange;
	static const Color		sGrey;
	static const Color		sLightGrey;
	static const Color		sWhite;

	union
	{
		uint32				mU32;
		struct
		{
			uint8			r, g, b, a;
		};
	};
};
