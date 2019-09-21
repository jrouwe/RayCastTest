#include <pch.h> // IWYU pragma: keep

#include <Renderer/TriangleRenderer.h>
#include <Geometry/AABox.h>

TriangleRenderer::TriangleRenderer()
{
	// Store singleton
	assert(sInstance == nullptr);
	sInstance = this;
}

TriangleRenderer::~TriangleRenderer()
{
	assert(sInstance == this);
	sInstance = nullptr;
}
