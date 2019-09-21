#pragma once

#include <RayCastTest/RayCastTest.h>
#include <NodeCodec/NodeCodecAABBTreeSplitAxis.h>

// Raycast against AABB tree on CPU, uses the split axis for determining which order to traverse children
template <class TriangleCodec>
class RayCastCPUAABBTreeSplitAxis : public RayCastTest
{
	static const int stack_size = 64;

public:
									RayCastCPUAABBTreeSplitAxis(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTreeSplitAxis");
		ioRow += mStats;
	}

	virtual void					Initialize() override
	{
		// Check if stack is big enough
		if (mRoot->GetMaxDepth() >= (uint)stack_size)
			FatalError("RayCastCPUAABBTreeSplitAxis: Tree too deep");

		AABBTreeToBufferStats stats;
		mBuffer.Convert(mVertices, mRoot, stats);
		mStats.Set(stats);
	}

	virtual void					TrashCache() override
	{
		CacheTrasher::sTrash(mBuffer.GetBuffer());
	}

	virtual void					CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override
	{
		const typename TriangleCodec::DecodingContext ctx(mBuffer.GetTriangleHeader(), mBuffer.GetBuffer());

		RayCastTestOut *out = outRayCasts;
		for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
		{
			Vec3 origin(ray->mOrigin);
			Vec3 direction(ray->mDirection);
			Vec3 inv_direction = direction.Reciprocal();
			UVec4 is_parallel = RayIsParallel(direction);
			int sign_direction[] = { ray->mDirection.x > 0.0f? 1 : 0, ray->mDirection.y > 0.0f? 1 : 0, ray->mDirection.z > 0.0f? 1 : 0 };

			float closest = FLT_MAX;
			const NodeCodecAABBTreeSplitAxis::Node *stack[stack_size];
			const NodeCodecAABBTreeSplitAxis::Node *node = reinterpret_cast<const NodeCodecAABBTreeSplitAxis::Node *>(mBuffer.GetRoot());
			int top = -1;
			for (;;)
			{
				// Test if node is closer than closest hit result
				if (RayAABoxHits(origin, inv_direction, is_parallel, node->GetBoundsMin(), node->GetBoundsMax(), closest))
				{
					// Test if node contains triangles
					if (!node->HasTriangles())
					{
						// Use split axis to determine which child to visit first
						const NodeCodecAABBTreeSplitAxis::Node *children[] = { node->GetLeftChild(), node->GetRightChild() };
						uint32 sign = sign_direction[node->GetSplitAxis()];
						const NodeCodecAABBTreeSplitAxis::Node *first = children[sign ^ 1];
						const NodeCodecAABBTreeSplitAxis::Node *second = children[sign];
						assert(top < stack_size - 1);
						stack[++top] = second;
						node = first;
						continue;
					}
					else
					{	
						// Node contains triangles, do individual tests
						ctx.TestRay(origin, direction, node->GetBoundsMin(), node->GetBoundsMax(), node->GetTriangles(), node->GetTriangleCount(), closest);
					}
				}

				// Fetch next node
				if (top < 0)
					break;
				node = stack[top--];
			}

			out->mDistance = closest;
		}
	}

private:
	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	AABBTreeToBuffer<TriangleCodec, NodeCodecAABBTreeSplitAxis> mBuffer;
	StatsRow						mStats;
};
