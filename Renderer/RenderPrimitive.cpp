#include <pch.h>

#include <Renderer/RenderPrimitive.h>
#include <Renderer/FatalErrorIfFailed.h>

void RenderPrimitive::Clear()
{
	mVtxBuffer = nullptr;
	mNumVtx = 0;
	mVtxSize = 0;

	mIdxBuffer = nullptr;
	mNumIdx = 0;
	mNumIdxToDraw = 0;
}

void RenderPrimitive::CreateVertexBuffer(int inNumVtx, int inVtxSize, const void *inData)
{
	// Buffer data description
	D3D11_SUBRESOURCE_DATA vertex_buffer_data;
	memset(&vertex_buffer_data, 0, sizeof(vertex_buffer_data));
	vertex_buffer_data.pSysMem = inData;

	// Vertex description
	CD3D11_BUFFER_DESC vertex_buffer_desc(
		(uint)(inNumVtx * inVtxSize),
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	// Allocate resource and copy the data
	FatalErrorIfFailed(
		mRenderer->GetDevice()->CreateBuffer(
			&vertex_buffer_desc,
			inData != nullptr? &vertex_buffer_data : nullptr,
			&mVtxBuffer));

	mNumVtx = inNumVtx;
	mVtxSize = inVtxSize;
}

void *RenderPrimitive::LockVertexBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;

	FatalErrorIfFailed(
		mRenderer->GetContext()->Map(
		mVtxBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource));

	return mapped_resource.pData;
}

void RenderPrimitive::UnlockVertexBuffer()
{
	mRenderer->GetContext()->Unmap(mVtxBuffer.Get(), 0);
}
	
void RenderPrimitive::CreateIndexBuffer(int inNumIdx, const uint32 *inData)
{
	// Buffer data description
	D3D11_SUBRESOURCE_DATA index_buffer_data;
	memset(&index_buffer_data, 0, sizeof(index_buffer_data));
	index_buffer_data.pSysMem = inData;

	// Vertex description
	CD3D11_BUFFER_DESC index_buffer_desc(
		(uint)(inNumIdx * sizeof(uint32)),
		D3D11_BIND_INDEX_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	// Allocate resource and copy the data
	FatalErrorIfFailed(
		mRenderer->GetDevice()->CreateBuffer(
			&index_buffer_desc,
			inData != nullptr? &index_buffer_data : nullptr,
			&mIdxBuffer));

	mNumIdx = inNumIdx;
	mNumIdxToDraw = inNumIdx;
}

uint32 *RenderPrimitive::LockIndexBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;

	FatalErrorIfFailed(
		mRenderer->GetContext()->Map(
		mIdxBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource));

	return (uint32 *)mapped_resource.pData;
}

void RenderPrimitive::UnlockIndexBuffer()
{
	mRenderer->GetContext()->Unmap(mIdxBuffer.Get(), 0);
}

void RenderPrimitive::Draw(ID3D11VertexShader *inVertexShader, ID3D11InputLayout *inInputLayout, ID3D11PixelShader *inPixelShader) const
{
	ID3D11DeviceContext *context = mRenderer->GetContext();

	mRenderer->BindShader(mType, inVertexShader, inInputLayout, inPixelShader);

	// Send vertex data to the Input Assembler stage
	UINT stride = mVtxSize;
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,  // start with the first vertex buffer
		1,  // one vertex buffer
		mVtxBuffer.GetAddressOf(),
		&stride,
		&offset);

	if (mIdxBuffer == nullptr)
	{
		// Set index buffer
		context->IASetIndexBuffer(
			nullptr, 
			DXGI_FORMAT_R32_UINT, 
			0);

		// Draw the non indexed primitive
		context->Draw(
			mNumVtx, 
			0); // Start with first vertex
	}
	else
	{
		// Set index buffer
		context->IASetIndexBuffer(
			mIdxBuffer.Get(), 
			DXGI_FORMAT_R32_UINT, 
			0); // Offset to first index

		// Draw indexed primitive
		context->DrawIndexed(
			mNumIdxToDraw, 
			0, // Start with first index
			0); // Start with first vertex
	}
}
