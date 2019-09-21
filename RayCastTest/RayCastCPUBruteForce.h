#pragma once

#include <RayCastTest/RayCastTest.h>
#include <Core/ByteBuffer.h>
#include <Utils/Model.h>
#include <Utils/PerfTimer.h>

// Raycast against each individual triangle on the CPU
template <class TriangleCodec, uint32 MaxVerticesPerBlock = INT_MAX>
class RayCastCPUBruteForce : public RayCastTest
{
public:
	virtual void				GetStats(StatsRow &ioRow) const override
	{
		ioRow += mStats;
	}

	virtual void				TrashCache() override
	{
		CacheTrasher::sTrash(&mBounds);
		CacheTrasher::sTrash(&mHeader);
		CacheTrasher::sTrash(mBuffer);
	}

	virtual void				Initialize() override
	{
		// Start timer
		PerfTimer timer("RayCastCPUBruteForce::Initialize");
		timer.Start();

		typename TriangleCodec::EncodingContext tri_ctx;

		// Reserve size for triangles
		uint total_size = tri_ctx.GetPessimisticMemoryEstimate(mModel->GetTriangleCount());
		mBuffer.reserve(total_size);

		// Calculate bounds
		mBounds = mModel->mBounds;

		// Make sure box is not degenerate
		mBounds.EnsureMinimalEdgeLength(1.0e-5f);

		// Divide triangles into blocks
		for (size_t v = 0; v < mModel->mIndexedTriangles.size(); v += MaxVerticesPerBlock)
		{
			// Calculate amount of triangles in this block
			uint32 num_triangles = min((uint32)(mModel->mIndexedTriangles.size() - v), MaxVerticesPerBlock);

			// Fetch block of triangles
			IndexedTriangleList block;
			block.assign(mModel->mIndexedTriangles.begin() + v, mModel->mIndexedTriangles.begin() + v + num_triangles);

			// Convert triangles
			uint32 offset = tri_ctx.Pack(mModel->mTriangleVertices, block, mBounds.mMin, mBounds.mMax, mBuffer);
			mBlocks.push_back({ offset, num_triangles });
		}

		// Finalize the triangles
		tri_ctx.Finalize(&mHeader, mBuffer);

		// Get stats
		string triangle_codec_name;
		float vertices_per_triangle;
		tri_ctx.GetStats(triangle_codec_name, vertices_per_triangle);
		mStats.Set(StatsColumn::TriangleCodec, triangle_codec_name);
		mStats.Set(StatsColumn::BufferVerticesPerTriangle, vertices_per_triangle);

		// Validate that we reserved enough memory
		assert(total_size >= (uint)mBuffer.size());
		mBuffer.shrink_to_fit();

		// Stop timer
		timer.Stop(1);
		timer.Output();

		// Add stats
		mStats.Set(StatsColumn::TestName, "RayCastCPUBruteForce");
		mStats.Set(StatsColumn::BufferTotalSize, (uint)mBuffer.size());
		mStats.Set(StatsColumn::BufferTrianglesSize, (uint)mBuffer.size());
		mStats.Set(StatsColumn::BufferBytesPerTriangle, (float)mBuffer.size() / mModel->GetTriangleCount());
	}

	virtual void				CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override
	{
		const typename TriangleCodec::DecodingContext ctx(&mHeader, mBuffer);

		const uint8 *triangles = &mBuffer[0];

		RayCastTestOut *out = outRayCasts;
		for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
		{
			Vec3 origin(ray->mOrigin);
			Vec3 direction(ray->mDirection);

			float closest = FLT_MAX;
			for (const TriangleBlock &b : mBlocks)
				ctx.TestRay(origin, direction, mBounds.mMin, mBounds.mMax, triangles + b.mOffset, b.mNumTriangles, closest);
			out->mDistance = closest;
		}
	}

private:
	AABox						mBounds;
	typename TriangleCodec::TriangleHeader mHeader;

	ByteBuffer					mBuffer;

	struct TriangleBlock
	{
		uint32					mOffset;
		uint32					mNumTriangles;
	};

	vector<TriangleBlock>		mBlocks;

	StatsRow					mStats;
};
