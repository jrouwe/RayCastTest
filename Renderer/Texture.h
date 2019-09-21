#pragma once

#include <Core/Reference.h>

// Forward declares
class Renderer;

class Texture : public RefTarget<Texture>
{
public:
	// Constructor, called by Renderer::CreateTexture
										Texture(Renderer *inRenderer, int inWidth, int inHeight, DXGI_FORMAT inFormat);

	// Get dimensions of texture
	inline int							GetWidth() const		{ return mWidth; }
	inline int							GetHeight() const		{ return mHeight; }

	// Bind / unbind texture to the pixel shader
	void								Bind(int inSlot) const;
	void								UnBind(int inSlot) const;

	// Clear this texture (only possible for render targets)
	void								ClearRenderTarget(const Vec4 &inColor = Vec4::sZero(), float inDepth = 1.0f);

	// Activate this texture as the current render target
	void								SetAsRenderTarget() const;

private:
	Renderer *							mRenderer;
	int									mWidth;
	int									mHeight;
	ComPtr<ID3D11Texture2D>				mTexture;
	ComPtr<ID3D11RenderTargetView>		mRenderTargetView;
	ComPtr<ID3D11ShaderResourceView>	mSRV;
	ComPtr<ID3D11Texture2D>				mDepthStencilBuffer;
	ComPtr<ID3D11DepthStencilView>		mDepthStencilView;
};
