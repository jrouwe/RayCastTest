#include <pch.h>

#include <Renderer/Texture.h>
#include <Renderer/Renderer.h>
#include <Renderer/FatalErrorIfFailed.h>

Texture::Texture(Renderer *inRenderer, int inWidth, int inHeight, DXGI_FORMAT inFormat) :
	mRenderer(inRenderer)
{
	// Store dimensions
	mWidth = inWidth;
	mHeight = inHeight;

	bool has_color = inFormat != DXGI_FORMAT_UNKNOWN;
	if (has_color)
	{
		// Create render target
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = inWidth;
		desc.Height = inHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = inFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		FatalErrorIfFailed(mRenderer->GetDevice()->CreateTexture2D(&desc, nullptr, &mTexture));

		// Create the render target view
		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
		memset(&rtv_desc, 0, sizeof(rtv_desc));
		rtv_desc.Format = desc.Format;
		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Texture2D.MipSlice = 0;
		FatalErrorIfFailed(mRenderer->GetDevice()->CreateRenderTargetView(mTexture.Get(), &rtv_desc, &mRenderTargetView));
	}

	// Create depth buffer
	D3D11_TEXTURE2D_DESC depth_desc;
	memset(&depth_desc, 0, sizeof(depth_desc));
	depth_desc.Width = inWidth;
	depth_desc.Height = inHeight;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depth_desc.SampleDesc.Count = 1;
	depth_desc.SampleDesc.Quality = 0;
	depth_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_desc.BindFlags = has_color? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depth_desc.CPUAccessFlags = 0;
	depth_desc.MiscFlags = 0;
	FatalErrorIfFailed(mRenderer->GetDevice()->CreateTexture2D(&depth_desc, nullptr, &mDepthStencilBuffer));
	
	// Create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC ds_view;
	memset(&ds_view, 0, sizeof(ds_view));
	ds_view.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ds_view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	ds_view.Texture2D.MipSlice = 0;
	FatalErrorIfFailed(mRenderer->GetDevice()->CreateDepthStencilView(mDepthStencilBuffer.Get(), &ds_view, &mDepthStencilView));

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	memset(&srv_desc, 0, sizeof(srv_desc));
	srv_desc.Format = has_color? inFormat : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 1;
	FatalErrorIfFailed(mRenderer->GetDevice()->CreateShaderResourceView(has_color? mTexture.Get() : mDepthStencilBuffer.Get(), &srv_desc, &mSRV));
}

void Texture::Bind(int inSlot) const
{
	// Set shader texture resource in the pixel shader
	mRenderer->GetContext()->PSSetShaderResources(inSlot, 1, mSRV.GetAddressOf());
}

void Texture::UnBind(int inSlot) const
{
	// Remove shader texture resource in the pixel shader
	ID3D11ShaderResourceView *srv[] = { nullptr };
	mRenderer->GetContext()->PSSetShaderResources(inSlot, 1, srv);
}

void Texture::ClearRenderTarget(const Vec4 &inColor, float inDepth)
{
	// Clear the render target
	if (mRenderTargetView.Get() != nullptr)
		mRenderer->GetContext()->ClearRenderTargetView(mRenderTargetView.Get(), (const float *)&inColor);
    
	// Clear the depth buffer
	mRenderer->GetContext()->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH, inDepth, 0);
}

void Texture::SetAsRenderTarget() const
{
	// Check if this was created as a render target
	assert(mRenderTargetView.Get() != nullptr || mDepthStencilView.Get() != nullptr);

	// Bind the render target and depth stencil buffer
	mRenderer->GetContext()->OMSetRenderTargets(1, mRenderTargetView.GetAddressOf(), mDepthStencilView.Get());
	
	// Set the viewport to the entire texture
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)mWidth;
	viewport.Height = (float)mHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	mRenderer->GetContext()->RSSetViewports(1, &viewport);
}
