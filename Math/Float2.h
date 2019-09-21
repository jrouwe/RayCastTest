#pragma once

class Float2
{
public:
						Float2()										{ }
						Float2(float inX, float inY)					: x(inX), y(inY) { }

	bool				operator == (const Float2 &inRHS) const			{ return x == inRHS.x && y == inRHS.y; }
	bool				operator != (const Float2 &inRHS) const			{ return x != inRHS.x || y != inRHS.y; }

	// To String
	friend ostream &	operator << (ostream &inStream, const Float2 &inV)
	{
		inStream << inV.x << ", " << inV.y;
		return inStream;
	}

	float				x;
	float				y;
};
