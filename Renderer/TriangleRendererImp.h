#pragma once

#include <Renderer/TriangleRenderer.h>
#include <Renderer/RenderPrimitive.h>
#include <Renderer/RenderInstances.h>
#include <Renderer/Texture.h>
#include <unordered_map>

class Renderer;

// Implementation of TriangleRenderer
class TriangleRendererImp final : public TriangleRenderer
{
public:
	// Constructor
										TriangleRendererImp(Renderer *inRenderer);

	// Implementation of TriangleRenderer interface
	virtual Batch						CreateTriangleBatch(const TriangleList &inTriangles) override;
	virtual void						DrawTriangleBatch(const Mat44 &inModelMatrix, const Color &inModelColor, Batch inBatch) override;
	virtual void						DrawTriangleBatchBackFacing(const Mat44 &inModelMatrix, const Color &inModelColor, Batch inBatch) override;
	
	// Draw all primitives that were added
	void								Draw();

	// Clear all primitives (to be called after drawing)
	void								Clear();
	
private:
	// Implementation specific batch object
	class BatchImpl : public RefTargetVirtual, public RenderPrimitive
	{
	public:
										BatchImpl(Renderer *inRenderer, D3D_PRIMITIVE_TOPOLOGY inType) : RenderPrimitive(inRenderer, inType) { }

		virtual void					AddRef() override			{ RenderPrimitive::AddRef(); }
		virtual void					Release() override			{ if (--mRefCount == 0) delete this; }
	};

	Renderer *							mRenderer;

	ComPtr<ID3D11InputLayout>			mInputLayout;
	ComPtr<ID3D11VertexShader>			mVertexShader;
	ComPtr<ID3D11PixelShader>			mPixelShader;

	ComPtr<ID3D11VertexShader>			mDepthVertexShader;
	ComPtr<ID3D11PixelShader>			mDepthPixelShader;
	Ref<Texture>						mDepthTexture;

	// List of primitives that are finished and ready for drawing
	struct Instance
	{
		Mat44							mModelMatrix;
		Mat44							mModelMatrixInvTrans;
		Color							mModelColor;
	};
	using InstanceMap = unordered_map<Batch, vector<Instance>>;
	InstanceMap							mPrimitives;
	InstanceMap							mPrimitivesBackFacing;
	int									mNumInstances = 0;
	Ref<RenderInstances>				mInstancesBuffer;
};
