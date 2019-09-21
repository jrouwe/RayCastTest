#include <pch.h> // IWYU pragma: keep

#include <Renderer/LineRenderer.h>

LineRenderer::LineRenderer()
{
	// Store singleton
	assert(sInstance == nullptr);
	sInstance = this;
}

LineRenderer::~LineRenderer()
{
	assert(sInstance == this);
	sInstance = nullptr;
}

void LineRenderer::DrawMarker(const Vec3 &inPosition, const Color &inColor, float inSize)
{
	Vec3 dx(inSize, 0, 0);
	Vec3 dy(0, inSize, 0);
	Vec3 dz(0, 0, inSize);
	DrawLine(inPosition - dy, inPosition + dy, inColor);
	DrawLine(inPosition - dx, inPosition + dx, inColor);
	DrawLine(inPosition - dz, inPosition + dz, inColor);
}

void LineRenderer::DrawArrow(const Vec3 &inFrom, const Vec3 &inTo, const Color &inColor, float inSize)
{
	// Draw base line
	DrawLine(inFrom, inTo, inColor);

	if (inSize > 0.0f)
	{
		// Draw arrow head
		Vec3 dir = inTo - inFrom;
		float len = dir.Length();
		if (len != 0.0f)
			dir = dir * (inSize / len);
		else
			dir = Vec3(inSize, 0, 0);
		Vec3 perp = inSize * dir.GetPerpendicular().Normalized();
		DrawLine(inTo - dir + perp, inTo, inColor);
		DrawLine(inTo - dir - perp, inTo, inColor);
	}
}

void LineRenderer::DrawCoordinateSystem(const Mat44 &inTransform, float inSize)
{
	DrawArrow(inTransform.GetTranslation(), inTransform * Vec3(inSize, 0, 0), Color::sRed, 0.1f * inSize);
	DrawArrow(inTransform.GetTranslation(), inTransform * Vec3(0, inSize, 0), Color::sGreen, 0.1f * inSize);
	DrawArrow(inTransform.GetTranslation(), inTransform * Vec3(0, 0, inSize), Color::sBlue, 0.1f * inSize);
}
