#pragma once

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <NodeCodec/NodeCodecAABBTreeCompressed.h>

// Raycast against compressed AABB tree on CPU. Optimized with SSE intrinsics.
template <class TriangleCodec>
class RayCastCPUAABBTreeCompressed : public RayCastTest
{
	static const int stack_size = 64;

public:
								RayCastCPUAABBTreeCompressed(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void				GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTreeCompressed");
		ioRow += mStats;
	}

	virtual void				Initialize() override
	{
		// Check if stack is big enough
		if (mRoot->GetMaxDepth() >= (uint)stack_size)
			FatalError("RayCastCPUAABBTreeCompressed: Tree too deep");

		AABBTreeToBufferStats stats;
		mBuffer.Convert(mVertices, mRoot, stats);
		mStats.Set(stats);
	}

	virtual void				TrashCache() override
	{
		CacheTrasher::sTrash(mBuffer.GetBuffer());
	}

	virtual void				CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override
	{
		const NodeCodecAABBTreeCompressed::Header *header = mBuffer.GetNodeHeader();

		const typename TriangleCodec::DecodingContext ctx(mBuffer.GetTriangleHeader(), mBuffer.GetBuffer());

		RayCastTestOut *out = outRayCasts;
		for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
		{
			Vec3 origin(ray->mOrigin);
			Vec3 direction(ray->mDirection);
			Vec3 inv_direction = direction.Reciprocal();
			UVec4 is_parallel = RayIsParallel(direction);

			float closest = FLT_MAX;

			struct StackEntry
			{
				Vec3		mBoundsMin;
				Vec3		mBoundsMax;
				const void *mPtr;
				uint32		mTriangleCount;
			};
			StackEntry stack[stack_size];

			Vec3 node_min(header->mRootBoundsMin);
			Vec3 node_max(header->mRootBoundsMax);
			const void *ptr = mBuffer.GetRoot();
			uint32 triangle_count = header->mRootTriangleCount;

			int top = -1;
			for (;;)
			{
				// Test if node contains triangles
				if (triangle_count == 0)
				{
					const NodeCodecAABBTreeCompressed::Node *node = reinterpret_cast<const NodeCodecAABBTreeCompressed::Node *>(ptr);

					// Get child bounding boxes
					Vec3 child_bounds_min[2], child_bounds_max[2];
					node->UnpackBounds(node_min, node_max, child_bounds_min, child_bounds_max);
					
					// Test bounds of left child
					float left_distance = RayAABox(origin, inv_direction, is_parallel, child_bounds_min[0], child_bounds_max[0]);
					bool left_intersects = left_distance < closest;

					// Test bounds of right child
					float right_distance = RayAABox(origin, inv_direction, is_parallel, child_bounds_min[1], child_bounds_max[1]);
					bool right_intersects = right_distance < closest;

					if (left_intersects && right_intersects)
					{
						// Both collide
						if (left_distance < right_distance)
						{
							// Test left child before right child
							++top;
							stack[top].mBoundsMin = child_bounds_min[1];
							stack[top].mBoundsMax = child_bounds_max[1];
							stack[top].mPtr = node->GetRightPtr();
							stack[top].mTriangleCount = node->GetRightTriangleCount();
							node_min = child_bounds_min[0];
							node_max = child_bounds_max[0];
							ptr = node->GetLeftPtr();
							triangle_count = node->GetLeftTriangleCount();
						}
						else
						{
							// Test right child before left child
							++top;
							stack[top].mBoundsMin = child_bounds_min[0];
							stack[top].mBoundsMax = child_bounds_max[0];
							stack[top].mPtr = node->GetLeftPtr();
							stack[top].mTriangleCount = node->GetLeftTriangleCount();
							node_min = child_bounds_min[1];
							node_max = child_bounds_max[1];
							ptr = node->GetRightPtr();
							triangle_count = node->GetRightTriangleCount();
						}
						continue;
					}
					else if (left_intersects)
					{
						// Only left collides
						node_min = child_bounds_min[0];
						node_max = child_bounds_max[0];
						ptr = node->GetLeftPtr();
						triangle_count = node->GetLeftTriangleCount();
						continue;
					}
					else if (right_intersects)
					{
						// Only right collides
						node_min = child_bounds_min[1];
						node_max = child_bounds_max[1];
						ptr = node->GetRightPtr();
						triangle_count = node->GetRightTriangleCount();
						continue;
					}
				}
				else
				{	
					// Node contains triangles, do individual tests
					ctx.TestRay(origin, direction, node_min, node_max, ptr, triangle_count, closest);
				}

				// Fetch next node
				if (top < 0)
					break;
				node_min = stack[top].mBoundsMin;
				node_max = stack[top].mBoundsMax;
				ptr = stack[top].mPtr;
				triangle_count = stack[top].mTriangleCount;
				--top;
			}

			out->mDistance = closest;
		}
	}

private:
	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	AABBTreeToBuffer<TriangleCodec, NodeCodecAABBTreeCompressed> mBuffer;
	StatsRow						mStats;
};
