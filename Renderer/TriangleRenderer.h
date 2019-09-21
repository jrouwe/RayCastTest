#pragma once

#include <Core/Color.h>
#include <Core/Reference.h>
#include <Math/Float2.h>
#include <Geometry/Triangle.h>

// Simple triangle renderer for debugging purposes
//
// When inColor.a == 0, the shape is rendered without casting or receiving shadows
class TriangleRenderer
{
public:
	// Constructor / destructor
										TriangleRenderer();
	virtual								~TriangleRenderer();	

	// Singleton instance
	inline static TriangleRenderer *	sInstance = nullptr;

	// Vertex format used by the triangle renderer
	class Vertex
	{
	public:
		Float3							mPosition;
		Float3							mNormal;
		Float2							mUV;
		Color							mColor;
	};

	// Handle for a batch
	using Batch = Ref<RefTargetVirtual>;
	
	// Create a batch of triangles that can be drawn efficiently
	virtual Batch						CreateTriangleBatch(const TriangleList &inTriangles) = 0;

	// Draw them
	virtual void						DrawTriangleBatch(const Mat44 &inModelMatrix, const Color &inModelColor, Batch inBatch) = 0;
	virtual void						DrawTriangleBatchBackFacing(const Mat44 &inModelMatrix, const Color &inModelColor, Batch inBatch) = 0;
};
