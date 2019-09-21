#pragma once

class Renderer;

class ConstantBuffer
{
public:
	// Constructor
										ConstantBuffer(Renderer *inRenderer, uint inBufferSize);

	// Map / unmap buffer (get pointer to data). This will discard all data in the buffer.
	template <typename T> T *			Map()											{ return reinterpret_cast<T *>(MapInternal()); }
	void								Unmap();

private:
	friend class Renderer;

	void *								MapInternal();

	Renderer *							mRenderer;
	ComPtr<ID3D11Buffer>				mBuffer;
};
