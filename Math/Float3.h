#pragma once

#include <Core/HashCombine.h>

class Float3
{
public:
				Float3() { }
				Float3(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) { }

	float		operator [] (int inCoordinate) const	
	{ 
		assert(inCoordinate < 3); 
		return *(&x + inCoordinate); 
	}

	bool		operator == (const Float3 &inRHS) const
	{
		return x == inRHS.x && y == inRHS.y && z == inRHS.z;
	}

	bool		operator != (const Float3 &inRHS) const
	{
		return x != inRHS.x || y != inRHS.y || z != inRHS.z;
	}

	float		x;
	float		y;
	float		z;
};

using VertexList = vector<Float3>;

// Create a std::hash for Float3
MAKE_HASHABLE(Float3, t.x, t.y, t.z)
