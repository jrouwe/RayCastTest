#pragma once

enum ECPUAccess
{
	CPU_ACCESS_NONE						= 0,
	CPU_ACCESS_READ						= 0x00000001,
	CPU_ACCESS_WRITE					= 0x00000002,
	CPU_ACCESS_READ_WRITE				= CPU_ACCESS_READ | CPU_ACCESS_WRITE,
	CPU_ACCESS_READ_COUNTER				= 0x00000004,
};

enum EGPUAccess
{
	GPU_ACCESS_READ						= 0x00000001,
	GPU_ACCESS_WRITE					= 0x00000002,
	GPU_ACCESS_READ_WRITE				= GPU_ACCESS_READ | GPU_ACCESS_WRITE,
};

enum ERaw
{
	RAW_NO,
	RAW_YES,
};

enum EAppend
{
	APPEND_NO,
	APPEND_YES,
};

enum EDispatch
{
	DISPATCH_NO,
	DISPATCH_YES,
};

class Renderer;

class StructuredBuffer
{
public:
	// Constructor
										StructuredBuffer(Renderer *inRenderer, ECPUAccess inCPUAccess, EGPUAccess inGPUAccess, ERaw inRaw, EAppend inAppend, EDispatch inDispatch, uint inElementSize, uint inElementCount, const void *inInitialContents);
										
	// Map / unmap buffer (get pointer to data, need CPU_ACCESS_READ). 
	template <typename T> T *			Map(ECPUAccess inCPUAccess)									{ return reinterpret_cast<T *>(MapInternal(inCPUAccess)); }
	void								Unmap(ECPUAccess inCPUAccess);

	// Get internal counter (need CPU_ACCESS_READ_COUNTER)
	uint32								GetCounter();

	// Copy internal counter to other buffer
	void								CopyCounterTo(StructuredBuffer *inDestinationBuffer, uint inOffset);

	// Size of buffer in bytes
	uint								GetByteSize() const;

private:
	friend class Renderer;

	void *								MapInternal(ECPUAccess inCPUAccess);

	Renderer *							mRenderer;
	ComPtr<ID3D11Buffer>				mBuffer;
	ComPtr<ID3D11Buffer>				mStagingBufferData;			// Buffer to read back data on the CPU
	ComPtr<ID3D11Buffer>				mStagingBufferCounter;		// Buffer to read back counter data on the CPU
	ComPtr<ID3D11ShaderResourceView>	mSRV;
	ComPtr<ID3D11UnorderedAccessView>	mUAV;
};
