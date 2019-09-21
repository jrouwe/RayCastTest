#include <pch.h>

#include <Renderer/RenderInstances.h>
#include <Renderer/RenderPrimitive.h>
#include <Renderer/FatalErrorIfFailed.h>

void RenderInstances::Clear()
{
	mInstanceBuffer = nullptr;
	mInstanceBufferSize = 0;
	mInstanceSize = 0;
}

void RenderInstances::CreateBuffer(int inNumInstances, int inInstanceSize, const void *inData)
{
	if (mInstanceBuffer != nullptr && mInstanceBufferSize >= inNumInstances * inInstanceSize)
	{
		// Update parameters
		mInstanceSize = inInstanceSize;

		// Copy data
		if (inData != nullptr)
		{
			void *data = Lock();
			memcpy(data, inData, inNumInstances * inInstanceSize);
			Unlock();
		}
	}
	else
	{
		// Calculate size
		mInstanceBufferSize = inNumInstances * inInstanceSize;

		// Buffer data description
		D3D11_SUBRESOURCE_DATA vertex_buffer_data;
		memset(&vertex_buffer_data, 0, sizeof(vertex_buffer_data));
		vertex_buffer_data.pSysMem = inData;

		// Vertex description
		CD3D11_BUFFER_DESC vertex_buffer_desc(
			(uint)mInstanceBufferSize,
			D3D11_BIND_VERTEX_BUFFER,
			D3D11_USAGE_DYNAMIC,
			D3D11_CPU_ACCESS_WRITE);

		// Allocate resource and copy the data
		FatalErrorIfFailed(
			mRenderer->GetDevice()->CreateBuffer(
				&vertex_buffer_desc,
				inData != nullptr? &vertex_buffer_data : nullptr,
				&mInstanceBuffer));

		mInstanceSize = inInstanceSize;
	}
}

void *RenderInstances::Lock()
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;

	FatalErrorIfFailed(
		mRenderer->GetContext()->Map(
		mInstanceBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource));

	return mapped_resource.pData;
}

void RenderInstances::Unlock()
{
	mRenderer->GetContext()->Unmap(mInstanceBuffer.Get(), 0);
}
	
void RenderInstances::Draw(RenderPrimitive *inPrimitive, int inStartInstance, int inNumInstances) const
{
	ID3D11DeviceContext *context = mRenderer->GetContext();

	// Send vertex data to the Input Assembler stage
	ID3D11Buffer *buffers[] = { inPrimitive->mVtxBuffer.Get(), mInstanceBuffer.Get() };
	UINT stride[] = { (UINT)inPrimitive->mVtxSize, (UINT)mInstanceSize };
	UINT offset[] = { 0, 0 };
	context->IASetVertexBuffers(
		0,  // start with the first vertex buffer
		2,  // two vertex buffers
		buffers,
		stride,
		offset);

	if (inPrimitive->mIdxBuffer == nullptr)
	{
		// Set index buffer
		context->IASetIndexBuffer(
			nullptr, 
			DXGI_FORMAT_R32_UINT, 
			0);

		// Draw instanced primitive
		context->DrawInstanced(inPrimitive->mNumVtx, inNumInstances, 0, inStartInstance);
	}
	else
	{
		// Set index buffer
		context->IASetIndexBuffer(
			inPrimitive->mIdxBuffer.Get(), 
			DXGI_FORMAT_R32_UINT, 
			0); // Offset to first index

		// Draw instanced primitive
		context->DrawIndexedInstanced(inPrimitive->mNumIdxToDraw, inNumInstances, 0, 0, inStartInstance);
	}
}
