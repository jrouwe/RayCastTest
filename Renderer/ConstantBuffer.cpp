#include <pch.h>

#include <Renderer/ConstantBuffer.h>
#include <Renderer/Renderer.h>
#include <Renderer/FatalErrorIfFailed.h>

ConstantBuffer::ConstantBuffer(Renderer *inRenderer, uint inBufferSize) :
	mRenderer(inRenderer)
{
	// Create constant buffer
	CD3D11_BUFFER_DESC constant_desc(
		AlignUp(inBufferSize, 16),
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	FatalErrorIfFailed(
		mRenderer->GetDevice()->CreateBuffer(
		&constant_desc,
		nullptr,
		mBuffer.GetAddressOf()));
}

void *ConstantBuffer::MapInternal()
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;

	FatalErrorIfFailed(
		mRenderer->GetContext()->Map(
		mBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource));

	return mapped_resource.pData;
}

void ConstantBuffer::Unmap()
{
	mRenderer->GetContext()->Unmap(mBuffer.Get(), 0);
}
