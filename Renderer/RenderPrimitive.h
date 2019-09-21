// Simple wrapper around vertex and index buffers
#pragma once

#include <Renderer/Renderer.h>
#include <Core/Reference.h>

class RenderPrimitive : public RefTarget<RenderPrimitive>
{
public:
	// Constructor
							RenderPrimitive(Renderer *inRenderer, D3D_PRIMITIVE_TOPOLOGY inType)			: mRenderer(inRenderer), mType(inType) { }

	// Erase all primitive data
	void					Clear();

	// Check if this primitive contains any data
	bool					IsEmpty() const																	{ return mNumVtx == 0 && mNumIdx == 0; }

	// Vertex buffer management functions
	void					CreateVertexBuffer(int inNumVtx, int inVtxSize, const void *inData = nullptr);
	void *					LockVertexBuffer();
	void					UnlockVertexBuffer();
	int						GetNumVtx() const																{ return mNumVtx; }

	// Index buffer management functions
	void					CreateIndexBuffer(int inNumIdx, const uint32 *inData = nullptr);
	uint32 *				LockIndexBuffer();
	void					UnlockIndexBuffer();
	int						GetNumIdx() const																{ return mNumIdx; }
	int						GetNumIdxToDraw() const															{ return mNumIdxToDraw; }
	void					SetNumIdxToDraw(int inUsed)														{ mNumIdxToDraw = inUsed; }

	// Draw the primitive
	void					Draw(ID3D11VertexShader *inVertexShader, ID3D11InputLayout *inInputLayout, ID3D11PixelShader *inPixelShader) const;

private:
	friend class RenderInstances;

	Renderer *				mRenderer;

	D3D_PRIMITIVE_TOPOLOGY	mType;
	
	ComPtr<ID3D11Buffer>	mVtxBuffer;	
	int						mNumVtx = 0;
	int						mVtxSize = 0;
																					
	ComPtr<ID3D11Buffer>	mIdxBuffer;
	int						mNumIdx = 0;
	int						mNumIdxToDraw = 0;
};																					
