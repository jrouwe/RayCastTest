#pragma once

#ifdef _WIN32

#include <RayCastTest/RayCastTest.h>
#include <Renderer/StructuredBuffer.h>

// Brute force, use GPU, X triangles per thread, stored in different ways
class RayCastGPUBruteForce : public RayCastTest
{
public:
	enum EVariant
	{
		STORE_PER_TRIANGLE,
		STORE_PER_VECTOR,
		STORE_PER_COMPONENT
	};

										RayCastGPUBruteForce(EVariant inVariant, uint inGroupSize, int inTrianglesPerThread) : mVariant(inVariant), mGroupSize(inGroupSize), mTrianglesPerThread(inTrianglesPerThread) { }

	virtual void						GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastGPUBruteForce");
		switch (mVariant)
		{
		case STORE_PER_TRIANGLE:
			ioRow.Set(StatsColumn::TriangleCodec, "StorePerTriangle");
			break;
		case STORE_PER_VECTOR:
			ioRow.Set(StatsColumn::TriangleCodec, "StorePerVector");
			break;
		case STORE_PER_COMPONENT:
			ioRow.Set(StatsColumn::TriangleCodec, "StorePerComponent");
			break;
		}
		
		ioRow.Set(StatsColumn::TestVariant, mGroupSize);
		ioRow.Set(StatsColumn::TrianglesPerLeaf, mTrianglesPerThread);
		ioRow.Set(StatsColumn::BufferNodesSize, 0);
		ioRow.Set(StatsColumn::BufferVerticesPerTriangle, 3);
		ioRow.Set(StatsColumn::BufferBytesPerTriangle, (uint)(3 * sizeof(Float3)));
		
		if (mVertexBuffer.get() != nullptr)
		{
			ioRow.Set(StatsColumn::BufferTotalSize, mVertexBuffer->GetByteSize());
			ioRow.Set(StatsColumn::BufferTrianglesSize, mVertexBuffer->GetByteSize());
		}
	}

	virtual void						Initialize() override;

	virtual void						CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override;

private:
	EVariant							mVariant;
	uint								mGroupSize;
	int									mTrianglesPerThread;

	ComPtr<ID3D11ComputeShader>			mShaderStep1;
	ComPtr<ID3D11ComputeShader>			mShaderStep2;

	unique_ptr<StructuredBuffer>		mVertexBuffer;
	unique_ptr<StructuredBuffer>		mRays;
	unique_ptr<StructuredBuffer>		mOutputBuffer;
};

#endif