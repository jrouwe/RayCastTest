#pragma once

class Float4
{
public:
				Float4() { }
				Float4(float inX, float inY, float inZ, float inW) : x(inX), y(inY), z(inZ), w(inW) { }

	float		operator [] (int inCoordinate) const	
	{ 
		assert(inCoordinate < 4); 
		return *(&x + inCoordinate); 
	}

	float		x;
	float		y;
	float		z;
	float		w;
};
