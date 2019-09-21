#pragma once

#include <Renderer/LineRenderer.h>

class Renderer;

// Implmenentation of LineRenderer class
class LineRendererImp final : public LineRenderer
{
public:
	// Constructor
									LineRendererImp(Renderer *inRenderer);

	// Implementation of LineRenderer interface
	virtual void					DrawLine(const Vec3 &inFrom, const Vec3 &inTo, const Color &inColor)  override;
	virtual void					DrawLine(const Float3 &inFrom, const Float3 &inTo, const Color &inColor) override;

	// Draw all lines
	void							Draw();

	// Clear all lines (to be called after drawing)
	void							Clear();

private:
	struct Line
	{
									Line(const Float3 &inFrom, const Float3 &inTo, const Color &inColor)				: mFrom(inFrom), mFromColor(inColor), mTo(inTo), mToColor(inColor) { }
									Line(const Vec3 inFrom, const Vec3 inTo, const Color &inColor)						: mFromColor(inColor), mToColor(inColor) { inFrom.StoreFloat3(&mFrom); inTo.StoreFloat3(&mTo); }

		Float3						mFrom;
		Color						mFromColor;
		Float3						mTo;
		Color						mToColor;
	};

	vector<Line>					mLines;

	Renderer *						mRenderer;
	ComPtr<ID3D11VertexShader>		mVertexShader;
	ComPtr<ID3D11InputLayout>		mInputLayout;
	ComPtr<ID3D11PixelShader>		mPixelShader;
};
