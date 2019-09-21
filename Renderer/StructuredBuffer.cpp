#include <pch.h>

#include <Renderer/StructuredBuffer.h>
#include <Renderer/Renderer.h>
#include <Renderer/FatalErrorIfFailed.h>

StructuredBuffer::StructuredBuffer(Renderer *inRenderer, ECPUAccess inCPUAccess, EGPUAccess inGPUAccess, ERaw inRaw, EAppend inAppend, EDispatch inDispatch, uint inElementSize, uint inElementCount, const void *inInitialContents) :
	mRenderer(inRenderer)
{
	bool cpu_write = (inCPUAccess & CPU_ACCESS_WRITE) != 0;
	bool gpu_write = (inGPUAccess & GPU_ACCESS_WRITE) != 0;
	
	D3D11_USAGE usage;
	uint bind_flags = 0;
	uint cpu_access = 0;
	if (!gpu_write && !cpu_write)
	{
		usage = D3D11_USAGE_IMMUTABLE;
		assert(inInitialContents != nullptr);
	}
	else if (gpu_write && !cpu_write)
	{
		usage = D3D11_USAGE_DEFAULT;
		bind_flags = D3D11_BIND_UNORDERED_ACCESS;
	}
	else if (!gpu_write && cpu_write)
	{
		usage = D3D11_USAGE_DYNAMIC;
		cpu_access = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		assert(false);
		usage = D3D11_USAGE_DEFAULT;
	}

	// Create main buffer
	CD3D11_BUFFER_DESC buffer_desc(
		inElementSize * inElementCount,
		D3D11_BIND_SHADER_RESOURCE | bind_flags,
		usage,
		cpu_access,
		(inRaw == RAW_YES? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) | (inDispatch == DISPATCH_YES? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0),
		inElementSize);

	// Setup initial contents
	D3D11_SUBRESOURCE_DATA buffer_data;
	memset(&buffer_data, 0, sizeof(buffer_data));
	buffer_data.pSysMem = inInitialContents;

	FatalErrorIfFailed(
		mRenderer->GetDevice()->CreateBuffer(
		&buffer_desc,
		inInitialContents != nullptr? &buffer_data : nullptr,
		&mBuffer));

	if (gpu_write)
	{
		// Create UAV
		CD3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc(
			mBuffer.Get(),
			inRaw == RAW_YES? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN,
			0,
			inElementCount,
			(inRaw == RAW_YES? D3D11_BUFFER_UAV_FLAG_RAW : 0) | (inAppend == APPEND_YES? D3D11_BUFFER_UAV_FLAG_APPEND : 0));
		FatalErrorIfFailed(
			mRenderer->GetDevice()->CreateUnorderedAccessView(
			mBuffer.Get(),
			&uav_desc,
			&mUAV));
	}
	else
	{
		assert(inAppend == APPEND_NO);

		// Create SRV
		CD3D11_SHADER_RESOURCE_VIEW_DESC srv_desc(
			mBuffer.Get(),
			inRaw == RAW_YES? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN,
			0,
			inElementCount, 
			inRaw == RAW_YES? D3D11_BUFFEREX_SRV_FLAG_RAW : 0);
		FatalErrorIfFailed(
			mRenderer->GetDevice()->CreateShaderResourceView(
			mBuffer.Get(),
			&srv_desc,
			&mSRV));
	}
	
	if (inCPUAccess & CPU_ACCESS_READ)
	{
		// Create staging buffer (for reading data on CPU)
		CD3D11_BUFFER_DESC staging_buffer_desc(
			inElementSize * inElementCount,
			0,
			D3D11_USAGE_STAGING,
			D3D11_CPU_ACCESS_READ,
			0,
			inElementSize);
		FatalErrorIfFailed(
			mRenderer->GetDevice()->CreateBuffer(
			&staging_buffer_desc,
			nullptr,
			&mStagingBufferData));
	}

	if (inAppend == APPEND_YES && (inCPUAccess & CPU_ACCESS_READ_COUNTER))
	{
		// Create staging buffer (for reading counter on CPU)
		CD3D11_BUFFER_DESC staging_buffer_desc(
			sizeof(uint32),
			0,
			D3D11_USAGE_STAGING,
			D3D11_CPU_ACCESS_READ,
			0,
			sizeof(uint32));
		FatalErrorIfFailed(
			mRenderer->GetDevice()->CreateBuffer(
			&staging_buffer_desc,
			nullptr,
			&mStagingBufferCounter));
	}
}

void *StructuredBuffer::MapInternal(ECPUAccess inCPUAccess)
{
	assert(inCPUAccess == CPU_ACCESS_READ || inCPUAccess == CPU_ACCESS_WRITE);

	if (inCPUAccess == CPU_ACCESS_READ)
	{
		assert(mStagingBufferData.Get() != nullptr);

		// Get result from GPU
		mRenderer->GetContext()->CopyResource(mStagingBufferData.Get(), mBuffer.Get());

		// Map for read
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		FatalErrorIfFailed(
			mRenderer->GetContext()->Map(
			mStagingBufferData.Get(),
			0,
			D3D11_MAP_READ,
			0,
			&mapped_resource));

		return mapped_resource.pData;
	}
	else
	{
		// Map for write
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
}

void StructuredBuffer::Unmap(ECPUAccess inCPUAccess)
{
	assert(inCPUAccess == CPU_ACCESS_READ || inCPUAccess == CPU_ACCESS_WRITE);

	if (inCPUAccess == CPU_ACCESS_READ)
	{
		assert(mStagingBufferData.Get() != nullptr);

		// Unmap for read
		mRenderer->GetContext()->Unmap(mStagingBufferData.Get(), 0);
	}
	else
	{
		// Unmap for write
		mRenderer->GetContext()->Unmap(mBuffer.Get(), 0);
	}
}

uint32 StructuredBuffer::GetCounter()
{
	assert(mStagingBufferCounter.Get() != nullptr);
	assert(mUAV.Get() != nullptr);

	// Get counter from GPU
	mRenderer->GetContext()->CopyStructureCount(mStagingBufferCounter.Get(), 0, mUAV.Get());

	// Map for read
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	FatalErrorIfFailed(
		mRenderer->GetContext()->Map(
		mStagingBufferCounter.Get(),
		0,
		D3D11_MAP_READ,
		0,
		&mapped_resource));

	uint32 count = *reinterpret_cast<uint32 *>(mapped_resource.pData);

	mRenderer->GetContext()->Unmap(mStagingBufferCounter.Get(), 0);

	return count;
}

void StructuredBuffer::CopyCounterTo(StructuredBuffer *inDestinationBuffer, uint inOffset)
{
	assert(inDestinationBuffer != nullptr);
	assert(mUAV.Get() != nullptr);

	mRenderer->GetContext()->CopyStructureCount(inDestinationBuffer->mBuffer.Get(), inOffset, mUAV.Get());
}

uint StructuredBuffer::GetByteSize() const
{
	D3D11_BUFFER_DESC desc;
	mBuffer->GetDesc(&desc);
	uint size = desc.ByteWidth;

	if (mStagingBufferData != nullptr)
	{
		mStagingBufferData->GetDesc(&desc);
		size += desc.ByteWidth;
	}

	if (mStagingBufferCounter != nullptr)
	{
		mStagingBufferCounter->GetDesc(&desc);
		size += desc.ByteWidth;
	}

	return size;
}
