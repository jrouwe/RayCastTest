#include <pch.h>

#include <Renderer/LineRendererImp.h>
#include <Renderer/Renderer.h>
#include <Renderer/RenderPrimitive.h>

LineRendererImp::LineRendererImp(Renderer *inRenderer) :
	mRenderer(inRenderer)
{
	// Create input layout
	const D3D11_INPUT_ELEMENT_DESC vertex_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Load vertex shader
	mRenderer->CreateVertexShader("Shaders/LineVertexShader.hlsl", vertex_desc, ARRAYSIZE(vertex_desc), mVertexShader, mInputLayout);

	// Load pixel shader
	mPixelShader = mRenderer->CreatePixelShader("Shaders/LinePixelShader.hlsl");
}

void LineRendererImp::DrawLine(const Vec3 &inFrom, const Vec3 &inTo, const Color &inColor)
{ 
	mLines.push_back(Line(inFrom, inTo, inColor)); 
}

void LineRendererImp::DrawLine(const Float3 &inFrom, const Float3 &inTo, const Color &inColor) 
{ 
	mLines.push_back(Line(inFrom, inTo, inColor)); 
}

void LineRendererImp::Draw()
{
	// Draw the lines
	if (!mLines.empty())
	{
		RenderPrimitive primitive(mRenderer, D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		primitive.CreateVertexBuffer((int)mLines.size() * 2, sizeof(Line) / 2, &mLines[0]);
		primitive.Draw(mVertexShader.Get(), mInputLayout.Get(), mPixelShader.Get());
	}
}

void LineRendererImp::Clear()
{ 
	mLines.clear(); 
}
