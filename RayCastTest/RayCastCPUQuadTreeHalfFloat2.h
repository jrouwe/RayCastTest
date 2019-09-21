#pragma once

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <NodeCodec/NodeCodecQuadTreeHalfFloat.h>

// Raycast against Quad tree on CPU
template <class TriangleCodec, int Alignment>
class RayCastCPUQuadTreeHalfFloat2 : public RayCastTest
{
public:
	static_assert(IsPowerOf2(Alignment), "Alignment should be power of 2");

	typedef NodeCodecQuadTreeHalfFloat<Alignment> NodeCodec;

									RayCastCPUQuadTreeHalfFloat2(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot, EAABBTreeToBufferConvertMode inConvertMode = EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST) : mVertices(inVertices), mRoot(inRoot), mConvertMode(inConvertMode) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUQuadTreeHalfFloat2");
		ioRow.Set(StatsColumn::TestVariant, "NodeAlign" + ConvertToString(Alignment) + "_" + ConvertToString(mConvertMode));
		ioRow += mStats;
	}

	virtual void					Initialize() override
	{
		AABBTreeToBufferStats stats;
		mBuffer.Convert(mVertices, mRoot, stats, mConvertMode);
		mStats.Set(stats);
	}

	virtual void					TrashCache() override
	{
		CacheTrasher::sTrash(mBuffer.GetBuffer());
	}

	virtual void					CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override
	{
		const typename TriangleCodec::DecodingContext ctx(mBuffer.GetTriangleHeader(), mBuffer.GetBuffer());

		const typename NodeCodec::Header *header = mBuffer.GetNodeHeader();
		const Vec3 root_bounds_min(header->mRootBoundsMin);
		const Vec3 root_bounds_max(header->mRootBoundsMax);
		const uint8 *buffer_start = &mBuffer.GetBuffer()[0];
		uint root_properties = header->mRootProperties;
		
		RayCastTestOut *out = outRayCasts;
		for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
		{
			Vec3 origin(ray->mOrigin);
			Vec3 direction(ray->mDirection);
			Vec3 inv_direction = direction.Reciprocal();
			UVec4 is_parallel = RayIsParallel(direction);

			float closest = FLT_MAX;
			const int stack_size = 128;
			uint32 node_stack[stack_size];
			float distance_stack[stack_size];
			node_stack[0] = root_properties;
			distance_stack[0] = 0;
			int top = 0;
			do
			{
				// Test if node contains triangles
				uint32 node_properties = node_stack[top];
				uint32 tri_count = node_properties >> NodeCodec::TRIANGLE_COUNT_SHIFT;
				if (tri_count == 0)
				{
					const typename NodeCodec::Node *node = reinterpret_cast<const typename NodeCodec::Node *>(buffer_start + (node_properties << NodeCodec::OFFSET_NON_SIGNIFICANT_BITS));
					assert(IsAligned(node, Alignment));

					// Load all data in one go (see: http://gdcvault.com/play/1023026/Taming-the-Jaguar-x86-Optimization)
					UVec4 bounds_minxy = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(reinterpret_cast<const uint32 *>(&node->mBoundsMinX[0]));
					UVec4 bounds_minzmaxx = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(reinterpret_cast<const uint32 *>(&node->mBoundsMinZ[0]));
					UVec4 bounds_maxyz = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(reinterpret_cast<const uint32 *>(&node->mBoundsMaxY[0]));
					UVec4 properties = UVec4LoadInt4ConditionallyAligned<Alignment % 16 == 0>(&node->mNodeProperties[0]);

					// Unpack bounds
					Vec4 bounds_minx = bounds_minxy.HalfFloatToFloat();
					Vec4 bounds_miny = bounds_minxy.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();					
					Vec4 bounds_minz = bounds_minzmaxx.HalfFloatToFloat();
					Vec4 bounds_maxx = bounds_minzmaxx.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();
					Vec4 bounds_maxy = bounds_maxyz.HalfFloatToFloat();
					Vec4 bounds_maxz = bounds_maxyz.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();

					// Test bounds of 4 children
					Vec4 distance = RayAABox4(origin, inv_direction, is_parallel, bounds_minx, bounds_miny, bounds_minz, bounds_maxx, bounds_maxy, bounds_maxz);

					// Sort so that highest values are first (we want to first process closer hits and we process stack top to bottom)
					Vec4::sSort4Reverse(distance, properties);

					// Count how many results are closer
					UVec4 closer = Vec4::sLess(distance, Vec4::sReplicate(closest));
					int num_results = closer.CountTrues();

					// Shift the results so that only the closer ones remain
					distance = distance.ReinterpretAsInt().ShiftComponents4Minus(num_results).ReinterpretAsFloat();
					properties = properties.ShiftComponents4Minus(num_results);

					// Push them onto the stack
					assert(top + 4 < stack_size);
					distance.StoreFloat4((Float4 *)&distance_stack[top]);
					properties.StoreInt4(&node_stack[top]);
					top += num_results;
				}
				else
				{	
					// Node contains triangles, do individual tests
					assert(tri_count != NodeCodec::TRIANGLE_COUNT_MASK); // This is a padding node, it should have an invalid bounding box so we shouldn't get here
					const void *triangles = buffer_start + ((node_properties & NodeCodec::OFFSET_MASK) << NodeCodec::OFFSET_NON_SIGNIFICANT_BITS);
					ctx.TestRay(origin, direction, root_bounds_min, root_bounds_max, triangles, tri_count, closest);
				}

				// Fetch next node that could give a closer hit
				do 
					--top;
				while (top >= 0 && distance_stack[top] >= closest);
			}
			while (top >= 0);

			out->mDistance = closest;
		}
	}

private:
	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	EAABBTreeToBufferConvertMode	mConvertMode;
	AABBTreeToBuffer<TriangleCodec, NodeCodec> mBuffer;
	StatsRow						mStats;
};
