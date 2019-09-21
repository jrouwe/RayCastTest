#pragma once

#include <Core/Color.h>

// Simple class for drawing lines for debug purposes
class LineRenderer
{
public:
	// Constructor / destructor
									LineRenderer();
	virtual							~LineRenderer();

	// Draw line
	virtual void					DrawLine(const Vec3 &inFrom, const Vec3 &inTo, const Color &inColor) = 0;
	virtual void					DrawLine(const Float3 &inFrom, const Float3 &inTo, const Color &inColor) = 0;

	// Draw a marker on a position
	void							DrawMarker(const Vec3 &inPosition, const Color &inColor, float inSize);

	// Draw an arrow
	void							DrawArrow(const Vec3 &inFrom, const Vec3 &inTo, const Color &inColor, float inSize);

	// Draw coordinate system (3 arrows, x = red, y = green, z = blue)
	void							DrawCoordinateSystem(const Mat44 &inTransform, float inSize = 1.0f);

	// Singleton instance
	inline static LineRenderer *	sInstance = nullptr;
};
