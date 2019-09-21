#pragma once

#ifdef _WIN32

#include <RayCastTest/RayCastTest.h>
#include <Renderer/StructuredBuffer.h>

class ConstantBuffer;

// Brute force, use GPU, 1 triangle per thread, bit packet vertices
class RayCastGPUBruteForceBitPack : public RayCastTest
{
public:
										RayCastGPUBruteForceBitPack(uint inGroupSize) : mGroupSize(inGroupSize) { }

	virtual void						GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastGPUBruteForceBitPack");
		ioRow.Set(StatsColumn::BufferNodesSize, 0);
		ioRow.Set(StatsColumn::BufferVerticesPerTriangle, 3);
		ioRow.Set(StatsColumn::BufferBytesPerTriangle, (uint)(3 * 2 * sizeof(uint32)));

		if (mTriangleBuffer.get() != nullptr)
		{
			ioRow.Set(StatsColumn::BufferTotalSize, mTriangleBuffer->GetByteSize());
			ioRow.Set(StatsColumn::BufferTrianglesSize, mTriangleBuffer->GetByteSize());
		}
	}

	virtual void						Initialize() override;

	virtual void						CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override;

private:
	uint								mGroupSize;

	struct DecompressionInfo
	{
		Float3							mOffset;
		float							mPad;
		Float3							mScale;
	};

	ComPtr<ID3D11ComputeShader>			mShaderStep1;
	ComPtr<ID3D11ComputeShader>			mShaderStep2;

	unique_ptr<ConstantBuffer>			mConstantBuffer;
	unique_ptr<StructuredBuffer>		mTriangleBuffer;
	unique_ptr<StructuredBuffer>		mRays;
	unique_ptr<StructuredBuffer>		mOutputBuffer;
};

#endif