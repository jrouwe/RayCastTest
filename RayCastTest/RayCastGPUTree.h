#pragma once

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <Renderer/Renderer.h>
#include <Renderer/StructuredBuffer.h>

// Raycast against a tree on GPU
template <class TreeBuilder>
class RayCastGPUTree : public RayCastTest
{
	enum 
	{
		max_rays	= 16384,		// Maximum supported number of raycasts
		group_size	= 64,
	};

public:
									RayCastGPUTree(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot, const char *inShader) : mVertices(inVertices), mRoot(inRoot), mShaderName(inShader) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastGPUTree");
		ioRow.Set(StatsColumn::TestVariant, mShaderName);
		ioRow += mStats;
	}

	virtual void					Initialize() override
	{
		// Convert tree
		AABBTreeToBufferStats stats;
		mBuffer.Convert(mVertices, mRoot, stats);
		mStats.Set(stats);

		// Load shader
		mShader = mRenderer->CreateComputeShader((string("Shaders/") + mShaderName).c_str());

		// Create buffer for rays
		mRays = mRenderer->CreateStructuredBuffer(CPU_ACCESS_WRITE, sizeof(RayCastTestIn), max_rays);

		// Create output buffer
		mOutput = mRenderer->CreateRWStructuredBuffer(CPU_ACCESS_READ, sizeof(RayCastTestOut), max_rays);

		// Convert to ByteAddressBuffer
		mTreeBuffer = mRenderer->CreateByteAddressBuffer(CPU_ACCESS_NONE, (uint)mBuffer.GetBuffer().size(), &mBuffer.GetBuffer()[0]);
	}

	virtual void					CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override
	{
		// Check we're not overflowing our buffer (need to write an extra loop if we do)
		const uint ray_cast_count = (uint)(inRayCastsEnd - inRayCastsBegin);
		assert(ray_cast_count < max_rays);

		// Copy rays to buffer
		RayCastTestIn *rays = mRays->Map<RayCastTestIn>(CPU_ACCESS_WRITE);
		memcpy(rays, inRayCastsBegin, ray_cast_count * sizeof(RayCastTestIn));
		mRays->Unmap(CPU_ACCESS_WRITE);

		// Bind stuff
		mRenderer->CSBindShader(mShader.Get());
		mRenderer->CSBindRBuffers(mRays.get(), mTreeBuffer.get());
		mRenderer->CSBindRWBuffers(mOutput.get());

		const uint dispatch_size = (ray_cast_count + group_size - 1) / group_size;

		// Do raycasts
		mRenderer->Dispatch(dispatch_size, 1, 1);

		// Get output
		RayCastTestOut *output = mOutput->Map<RayCastTestOut>(CPU_ACCESS_READ);
		memcpy(outRayCasts, output, ray_cast_count * sizeof(RayCastTestOut));
		mOutput->Unmap(CPU_ACCESS_READ);
	}

private:
	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	TreeBuilder						mBuffer;
	StatsRow						mStats;
	const char *					mShaderName;
	ComPtr<ID3D11ComputeShader>		mShader;
	unique_ptr<StructuredBuffer>	mTreeBuffer;
	unique_ptr<StructuredBuffer>	mRays;
	unique_ptr<StructuredBuffer>	mOutput;
};
