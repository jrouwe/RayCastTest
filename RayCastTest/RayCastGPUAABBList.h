#pragma once

#ifdef _WIN32

#include <RayCastTest/RayCastTest.h>
#include <Renderer/StructuredBuffer.h>
#include <Utils/Model.h>

// Divide triangles in batches, 1 aabb per batch, GPU algorithm
class RayCastGPUAABBList : public RayCastTest
{
public:
	enum EVariant
	{
		GROUPER_MORTON,
		GROUPER_CLOSEST_CENTROID,
	};

										RayCastGPUAABBList(EVariant inVariant) : mVariant(inVariant), mSurfaceArea(0) { }

	virtual void						GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastGPUAABBList");
		switch (mVariant)
		{
		case GROUPER_MORTON:
			ioRow.Set(StatsColumn::TestVariant, "TriangleGrouperMorton");
			break;

		case GROUPER_CLOSEST_CENTROID:
			ioRow.Set(StatsColumn::TestVariant, "TriangleGrouperClosestCentroid");
			break;

		default:
			assert(false);
			break;
		}

		ioRow.Set(StatsColumn::TreeSurfaceArea, mSurfaceArea);
		ioRow.Set(StatsColumn::BufferVerticesPerTriangle, 3);

		if (mBoundingBoxBuffer.get() != nullptr && mVertexBuffer.get() != nullptr)
		{
			ioRow.Set(StatsColumn::BufferTotalSize, mBoundingBoxBuffer->GetByteSize() + mVertexBuffer->GetByteSize());
			ioRow.Set(StatsColumn::BufferNodesSize, mBoundingBoxBuffer->GetByteSize());
			ioRow.Set(StatsColumn::BufferTrianglesSize, mVertexBuffer->GetByteSize());
			ioRow.Set(StatsColumn::BufferBytesPerTriangle, (float)(mVertexBuffer->GetByteSize() + mBoundingBoxBuffer->GetByteSize()) / mModel->GetTriangleCount());
		}
	}

	virtual void						Initialize() override;

	virtual void						CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override;

private:
	EVariant							mVariant;
	float								mSurfaceArea;

	ComPtr<ID3D11ComputeShader>			mShader1;
	ComPtr<ID3D11ComputeShader>			mShader2;
	
	struct JobItem
	{
		uint32							mRayIndex;
		uint32							mBatchIndex;
	};

	unique_ptr<StructuredBuffer>		mBoundingBoxBuffer;
	unique_ptr<StructuredBuffer>		mVertexBuffer;
	unique_ptr<StructuredBuffer>		mRays;
	unique_ptr<StructuredBuffer>		mOutputBuffer;
	unique_ptr<StructuredBuffer>		mJobBuffer;
	unique_ptr<StructuredBuffer>		mDispatchBuffer;
};

#endif