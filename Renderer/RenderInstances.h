// Simple wrapper around vertex and index buffers
#pragma once

#include <Renderer/Renderer.h>
#include <Core/Reference.h>

class RenderPrimitive;

class RenderInstances : public RefTarget<RenderInstances>
{
public:
	// Constructor
							RenderInstances(Renderer *inRenderer)											: mRenderer(inRenderer) { }

	// Erase all instance data
	void					Clear();

	// Instance buffer management functions
	void					CreateBuffer(int inNumInstances, int inInstanceSize, const void *inData = nullptr);
	void *					Lock();
	void					Unlock();

	// Draw the instances when context has been set by Renderer::BindShader
	void					Draw(RenderPrimitive *inPrimitive, int inStartInstance, int inNumInstances) const;

private:
	Renderer *				mRenderer;
	
	ComPtr<ID3D11Buffer>	mInstanceBuffer;
	int						mInstanceBufferSize = 0;
	int						mInstanceSize = 0;
};
