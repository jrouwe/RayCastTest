#pragma once

#include <RayCastTest/RayCastTest.h>
#include <Core/ByteBuffer.h>
#include <Core/AlignedAllocator.h>
#include <Math/HalfFloat.h>
#include <Utils/Model.h>
#include <TriangleGrouper/TriangleGrouperMorton.h>
#include <TriangleGrouper/TriangleGrouperClosestCentroid.h>
#include <Geometry/RayAABox8.h>

enum class ERayCastCPUAABBListVariant
{
	BOUNDS_PLAIN,
	BOUNDS_SOA4,
	BOUNDS_SOA8,
	BOUNDS_HALFFLOAT_SOA4
};

enum class ERayCastCPUAABBGrouper
{
	GROUPER_MORTON,
	GROUPER_CLOSEST_CENTROID,
};

// Divide triangles in batches, 1 aabb per batch
template <class TriangleCodec, int Alignment>
class RayCastCPUAABBList : public RayCastTest
{
public:
	static_assert(IsPowerOf2(Alignment), "Alignment should be power of 2");

	// Header for the triangles
	typedef typename TriangleCodec::TriangleHeader TriangleHeader;

										RayCastCPUAABBList(ERayCastCPUAABBListVariant inVariant, ERayCastCPUAABBGrouper inGrouper, uint inTrianglesPerBatch) :
		mVariant(inVariant), 
		mGrouper(inGrouper),
		mTrianglesPerBatch(inTrianglesPerBatch) 
	{ 
		// Must be power of 2
		if (!IsPowerOf2(mTrianglesPerBatch))
			FatalError("RayCastCPUAABBList: Triangles per batch not power of 2");
	}

	virtual void						GetStats(StatsRow &ioRow) const override
	{
		size_t triangles_size = sizeof(mTriangleHeader) + mTriangles.size();
		size_t nodes_size = mTrianglesStart.size() * sizeof(uint);

		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBList");

		string grouper;
		switch (mGrouper)
		{
		case ERayCastCPUAABBGrouper::GROUPER_MORTON:
			grouper = "GrouperMorton";
			break;
		case ERayCastCPUAABBGrouper::GROUPER_CLOSEST_CENTROID:
			grouper = "GrouperClosestCentroid";
			break;
		default:
			assert(false);
			break;
		}

		switch (mVariant)
		{
		case ERayCastCPUAABBListVariant::BOUNDS_PLAIN:		
			ioRow.Set(StatsColumn::TestVariant, "BoundsPlainAlign" + ConvertToString(Alignment) + "_" + grouper);			
			nodes_size += mBounds.size() * sizeof(AABox);
			break;
		case ERayCastCPUAABBListVariant::BOUNDS_SOA4:		
			ioRow.Set(StatsColumn::TestVariant, "BoundsSOA4Align" + ConvertToString(Alignment) + "_" + grouper);
			nodes_size += mBounds.size() * sizeof(float);
			break;
		case ERayCastCPUAABBListVariant::BOUNDS_SOA8:		
			ioRow.Set(StatsColumn::TestVariant, "BoundsSOA8Align" + ConvertToString(Alignment) + "_" + grouper);
			nodes_size += mBounds.size() * sizeof(float);
			break;
		case ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4:		
			ioRow.Set(StatsColumn::TestVariant, "BoundsHalfFloatSOA4Align" + ConvertToString(Alignment) + "_" + grouper);
			nodes_size += mBounds.size() * sizeof(uint16);
			break;
		default:
			assert(false);
			break;
		}

		size_t total_size = triangles_size + nodes_size;
		ioRow.Set(StatsColumn::TrianglesPerLeaf, mTrianglesPerBatch);
		ioRow.Set(StatsColumn::TreeSurfaceArea, mSurfaceArea);
		ioRow.Set(StatsColumn::BufferTotalSize, total_size);
		ioRow.Set(StatsColumn::BufferNodesSize, nodes_size);
		ioRow.Set(StatsColumn::BufferTrianglesSize, triangles_size);
		ioRow.Set(StatsColumn::BufferBytesPerTriangle, (float)total_size / mModel->GetTriangleCount());
		ioRow += mStats;
	}

	virtual void						Initialize() override
	{
		const IndexedTriangleList &triangle_list = mModel->mIndexedTriangles;
		const uint triangle_count = (uint)triangle_list.size();
		const uint num_batches = (triangle_count + mTrianglesPerBatch - 1) / mTrianglesPerBatch;

		// Group triangles according to locality
		vector<uint> sorted_triangle_idx;
		if (mGrouper == ERayCastCPUAABBGrouper::GROUPER_MORTON)
		{
			TriangleGrouperMorton grouper;
			grouper.Group(mModel->mTriangleVertices, triangle_list, mTrianglesPerBatch, sorted_triangle_idx);
		}
		else if (mGrouper == ERayCastCPUAABBGrouper::GROUPER_CLOSEST_CENTROID)
		{
			TriangleGrouperClosestCentroid grouper;
			grouper.Group(mModel->mTriangleVertices, triangle_list, mTrianglesPerBatch, sorted_triangle_idx);
		}

		// Calculate bounds for each group and split up triangles
		mBounds.resize(num_batches);
		mTrianglesStart.resize(num_batches);
		vector<IndexedTriangleList> containers;
		containers.resize(num_batches);
		for (uint t = 0; t < triangle_count; ++t)
		{
			const IndexedTriangle &triangle = triangle_list[sorted_triangle_idx[t]];
			mBounds[t / mTrianglesPerBatch].Encapsulate(mModel->mTriangleVertices, triangle);
			containers[t / mTrianglesPerBatch].push_back(triangle);
		}

		// Make the bounding boxes a bit bigger to avoid missing hits due to numerical imprecision
		for (uint b = 0; b < num_batches; ++b)
		{
			mBounds[b].WidenByFactor(0.002f);
			mBounds[b].EnsureMinimalEdgeLength(1.0e-5f);
		}

		// Fill up the last container to the right batch size
		while (containers[num_batches - 1].size() < mTrianglesPerBatch)
			containers[num_batches - 1].push_back(triangle_list[sorted_triangle_idx[triangle_count - 1]]);

		if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_SOA4)
		{
			// Convert bounds to a structure of array format with 4 bounding boxes at a time
			uint num_bounds = uint(AlignUp(mBounds.size(), 4));
			mBoundsSOA4.resize(6 * num_bounds);
			float *bounds_out = &mBoundsSOA4[0];
			for (uint b = 0; b < num_bounds; b += 4)
			{
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMin.GetX() : FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMin.GetY() : FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMin.GetZ() : FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMax.GetX() : FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMax.GetY() : FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMax.GetZ() : FLT_MAX;
			}
		}		
		
		if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_SOA8)
		{
			// Convert bounds to a structure of array format with 8 bounding boxes at a time
			uint num_bounds = uint(AlignUp(mBounds.size(), 8));
			mBoundsSOA8.resize(6 * num_bounds);
			float *bounds_out = &mBoundsSOA8[0];
			for (uint b = 0; b < num_bounds; b += 8)
			{
				for (uint i = 0; i < 8; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMin.GetX() : FLT_MAX;
				for (uint i = 0; i < 8; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMin.GetY() : FLT_MAX;
				for (uint i = 0; i < 8; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMin.GetZ() : FLT_MAX;
				for (uint i = 0; i < 8; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMax.GetX() : FLT_MAX;
				for (uint i = 0; i < 8; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMax.GetY() : FLT_MAX;
				for (uint i = 0; i < 8; ++i)
					*bounds_out++ = b + i < mBounds.size()? mBounds[b + i].mMax.GetZ() : FLT_MAX;
			}
		}
		
		if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4)
		{
			// Convert bounds to a structure of array format with 4 bounding boxes at a time
			uint num_bounds = uint(AlignUp(mBounds.size(), 4));
			mBoundsHalfFloatSOA4.resize(6 * num_bounds);
			uint16 *bounds_out = &mBoundsHalfFloatSOA4[0];
			for (uint b = 0; b < num_bounds; b += 4)
			{
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? FloatToHalfFloat<ROUND_TO_NEG_INF>(mBounds[b + i].mMin.GetX()) : HALF_FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? FloatToHalfFloat<ROUND_TO_NEG_INF>(mBounds[b + i].mMin.GetY()) : HALF_FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? FloatToHalfFloat<ROUND_TO_NEG_INF>(mBounds[b + i].mMin.GetZ()) : HALF_FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? FloatToHalfFloat<ROUND_TO_POS_INF>(mBounds[b + i].mMax.GetX()) : HALF_FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? FloatToHalfFloat<ROUND_TO_POS_INF>(mBounds[b + i].mMax.GetY()) : HALF_FLT_MAX;
				for (uint i = 0; i < 4; ++i)
					*bounds_out++ = b + i < mBounds.size()? FloatToHalfFloat<ROUND_TO_POS_INF>(mBounds[b + i].mMax.GetZ()) : HALF_FLT_MAX;
			}
		}
		
		// Calculate total surface area
		mSurfaceArea = 0.0f;
		for (const AABox &b : mBounds)
			mSurfaceArea += b.GetSurfaceArea();

		typename TriangleCodec::EncodingContext tri_ctx;

		// Reserve enough memory for the triangles
		uint total_size = tri_ctx.GetPessimisticMemoryEstimate(AlignUp(triangle_count, mTrianglesPerBatch));
		mTriangles.reserve(total_size);

		// Add triangles
		for (uint b = 0; b < num_batches; ++b)
			mTrianglesStart[b] = tri_ctx.Pack(mModel->mTriangleVertices, containers[b], mBounds[b].mMin, mBounds[b].mMax, mTriangles);

		// Finalize the triangles
		tri_ctx.Finalize(&mTriangleHeader, mTriangles);

		// Get stats
		string triangle_codec_name;
		float vertices_per_triangle;
		tri_ctx.GetStats(triangle_codec_name, vertices_per_triangle);
		mStats.Set(StatsColumn::TriangleCodec, triangle_codec_name);
		mStats.Set(StatsColumn::BufferVerticesPerTriangle, vertices_per_triangle);

		// Validate that we reserved enough memory
		if (total_size < (uint)mTriangles.size())
			FatalError("RayCastCPUAABBList: Not enough memory reserved");
		mTriangles.shrink_to_fit();
	}
	
	virtual void						TrashCache() override
	{
		CacheTrasher::sTrash(mBounds);
		CacheTrasher::sTrash(mBoundsSOA4);
		CacheTrasher::sTrash(mBoundsSOA8);
		CacheTrasher::sTrash(mBoundsHalfFloatSOA4);
		CacheTrasher::sTrash(mTrianglesStart);
		CacheTrasher::sTrash(&mTriangleHeader);
		CacheTrasher::sTrash(mTriangles);
	}

	virtual void						CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override
	{
		const typename TriangleCodec::DecodingContext ctx(&mTriangleHeader, mTriangles);

		RayCastTestOut *out = outRayCasts;

		if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_PLAIN)
		{
			for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
			{
				Vec3 origin(ray->mOrigin);
				Vec3 direction(ray->mDirection);
				Vec3 inv_direction = direction.Reciprocal();
				UVec4 is_parallel = RayIsParallel(direction);

				float closest = FLT_MAX;

				for (uint b = 0, n = (uint)mBounds.size(); b < n; ++b)
				{
					const Vec3 &bounds_min = mBounds[b].mMin;
					const Vec3 &bounds_max = mBounds[b].mMax;

					// Test bounds hit
					if (RayAABoxHits(origin, inv_direction, is_parallel, bounds_min, bounds_max, closest))
					{
						// Test triangles in batch
						ctx.TestRay(origin, direction, bounds_min, bounds_max, &mTriangles[mTrianglesStart[b]], mTrianglesPerBatch, closest);
					}
				}

				out->mDistance = closest;
			}
		}
		else if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_SOA4)
		{
			for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
			{
				Vec3 origin(ray->mOrigin);
				Vec3 direction(ray->mDirection);
				Vec3 inv_direction = direction.Reciprocal();
				UVec4 is_parallel = RayIsParallel(direction);

				float closest = FLT_MAX;

				const Float4 *ptr = reinterpret_cast<const Float4 *>(&mBoundsSOA4[0]);
				for (uint b = 0, n = (uint)mBounds.size(); b < n; b += 4)
				{
					Vec4 bounds_minx = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(ptr++);
					Vec4 bounds_miny = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(ptr++);
					Vec4 bounds_minz = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(ptr++);

					Vec4 bounds_maxx = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(ptr++);
					Vec4 bounds_maxy = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(ptr++);
					Vec4 bounds_maxz = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(ptr++);

					// Test bounds hit 4 bounding boxes at a time
					Vec4 distance = RayAABox4(origin, inv_direction, is_parallel, bounds_minx, bounds_miny, bounds_minz, bounds_maxx, bounds_maxy, bounds_maxz);
					UVec4 closer = Vec4::sLess(distance, Vec4::sReplicate(closest));
					if (closer.TestAnyTrue())
						for (int i = 0; i < 4; ++i)
							if (closer[i])
							{
								// Test triangles in batch
								assert(b + i < n); // We provided an invalid bounding box for these, so they should never be closer
								Vec3 bounds_min(bounds_minx[i], bounds_miny[i], bounds_minz[i]);
								Vec3 bounds_max(bounds_maxx[i], bounds_maxy[i], bounds_maxz[i]);
								ctx.TestRay(origin, direction, bounds_min, bounds_max, &mTriangles[mTrianglesStart[b + i]], mTrianglesPerBatch, closest);
							}
				}

				out->mDistance = closest;
			}
		}
		else if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_SOA8)
		{
			for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
			{
				Vec3 origin(ray->mOrigin);
				Vec3 direction(ray->mDirection);
				Vec3 inv_direction = direction.Reciprocal();
				UVec4 is_parallel = RayIsParallel(direction);

				float closest = FLT_MAX;

				const float *ptr = reinterpret_cast<const float *>(&mBoundsSOA8[0]);
				for (uint b = 0, n = (uint)mBounds.size(); b < n; b += 8)
				{
					Vec8 bounds_minx = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(ptr); ptr += 8;
					Vec8 bounds_miny = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(ptr); ptr += 8;
					Vec8 bounds_minz = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(ptr); ptr += 8;

					Vec8 bounds_maxx = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(ptr); ptr += 8;
					Vec8 bounds_maxy = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(ptr); ptr += 8;
					Vec8 bounds_maxz = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(ptr); ptr += 8;

					// Test bounds hit 8 bounding boxes at a time
					Vec8 distance = RayAABox8(origin, inv_direction, is_parallel, bounds_minx, bounds_miny, bounds_minz, bounds_maxx, bounds_maxy, bounds_maxz);
					UVec8 closer = Vec8::sLess(distance, Vec8::sReplicate(closest));
					if (closer.TestAnyTrue())
						for (int i = 0; i < 8; ++i)
							if (closer[i])
							{
								// Test triangles in batch
								assert(b + i < n); // We provided an invalid bounding box for these, so they should never be closer
								Vec3 bounds_min(bounds_minx[i], bounds_miny[i], bounds_minz[i]);
								Vec3 bounds_max(bounds_maxx[i], bounds_maxy[i], bounds_maxz[i]);
								ctx.TestRay(origin, direction, bounds_min, bounds_max, &mTriangles[mTrianglesStart[b + i]], mTrianglesPerBatch, closest);
							}
				}

				out->mDistance = closest;
			}
		}
		else if (mVariant == ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4)
		{
			for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
			{
				Vec3 origin(ray->mOrigin);
				Vec3 direction(ray->mDirection);
				Vec3 inv_direction = direction.Reciprocal();
				UVec4 is_parallel = RayIsParallel(direction);

				float closest = FLT_MAX;

				const uint32 *ptr = reinterpret_cast<const uint32 *>(&mBoundsHalfFloatSOA4[0]);
				for (uint b = 0, n = (uint)mBounds.size(); b < n; b += 4)
				{
					// Unpack bounds
					UVec4 bounds_minxy = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(ptr); ptr += 4;
					Vec4 bounds_minx = bounds_minxy.HalfFloatToFloat();
					Vec4 bounds_miny = bounds_minxy.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();

					UVec4 bounds_minzmaxx = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(ptr); ptr += 4;
					Vec4 bounds_minz = bounds_minzmaxx.HalfFloatToFloat();
					Vec4 bounds_maxx = bounds_minzmaxx.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();

					UVec4 bounds_maxyz = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(ptr); ptr += 4;
					Vec4 bounds_maxy = bounds_maxyz.HalfFloatToFloat();
					Vec4 bounds_maxz = bounds_maxyz.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();
					
					// Test bounds hit 4 bounding boxes at a time
					Vec4 distance = RayAABox4(origin, inv_direction, is_parallel, bounds_minx, bounds_miny, bounds_minz, bounds_maxx, bounds_maxy, bounds_maxz);
					UVec4 closer = Vec4::sLess(distance, Vec4::sReplicate(closest));
					if (closer.TestAnyTrue())
						for (int i = 0; i < 4; ++i)
							if (closer[i])
							{
								// Test triangles in batch
								assert(b + i < n); // We provided an invalid bounding box for these, so they should never be closer
								Vec3 bounds_min(bounds_minx[i], bounds_miny[i], bounds_minz[i]);
								Vec3 bounds_max(bounds_maxx[i], bounds_maxy[i], bounds_maxz[i]);
								ctx.TestRay(origin, direction, bounds_min, bounds_max, &mTriangles[mTrianglesStart[b + i]], mTrianglesPerBatch, closest);
							}
				}

				out->mDistance = closest;
			}
		}
	}

private:
	vector<AABox, AlignedAllocator<AABox, Alignment>>				mBounds;
	vector<float, AlignedAllocator<float, Alignment>>				mBoundsSOA4;
	vector<float, AlignedAllocator<float, Alignment>>				mBoundsSOA8;
	vector<uint16, AlignedAllocator<uint16, Alignment>>				mBoundsHalfFloatSOA4;
	vector<uint>						mTrianglesStart;
	TriangleHeader						mTriangleHeader;
	ByteBuffer							mTriangles;

	ERayCastCPUAABBListVariant			mVariant;
	ERayCastCPUAABBGrouper				mGrouper;
	uint								mTrianglesPerBatch;
	float								mSurfaceArea;
	StatsRow							mStats;
};
