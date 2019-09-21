#pragma once

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <NodeCodec/NodeCodecAABBTree.h>

// Raycast against AABB tree on CPU
// Variant 1: Tests bounds at each level of the tree and recurses to left and right child if it intersects
template <class TriangleCodec>
class RayCastCPUAABBTree1 : public RayCastTest
{
	static const int stack_size = 64;

public:
									RayCastCPUAABBTree1(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTree1");
		ioRow += mStats;
	}

	virtual void					Initialize() override
	{
		// Check if stack is big enough
		if (mRoot->GetMaxDepth() >= (uint)stack_size)
			FatalError("RayCastCPUAABBTree1: Tree too deep");

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

			float closest = FLT_MAX;
			const NodeCodecAABBTree::Node *stack[stack_size];
			const NodeCodecAABBTree::Node *node = reinterpret_cast<const NodeCodecAABBTree::Node *>(mBuffer.GetRoot());
			int top = -1;
			for (;;)
			{
				// Test if node is closer than closest hit result
				Vec3 node_min = node->GetBoundsMin(), node_max = node->GetBoundsMax();
				if (RayAABoxHits(origin, inv_direction, is_parallel, node_min, node_max, closest))
				{
					// Test if node contains triangles
					if (!node->HasTriangles())
					{
						const NodeCodecAABBTree::Node *left_child = node->GetLeftChild();
						const NodeCodecAABBTree::Node *right_child = node->GetRightChild();

						// Left child before right child
						assert(top < stack_size - 1);
						stack[++top] = right_child;
						node = left_child;
						continue;
					}
					else
					{	
						// Node contains triangles, do individual tests
						ctx.TestRay(origin, direction, node_min, node_max, node->GetTriangles(), node->GetTriangleCount(), closest);
					}
				}

				// Fetch next node
				if (top < 0)
					break;
				const NodeCodecAABBTree::Node *new_node = stack[top--];
				assert(new_node > node); // We expect to walk forward in memory, for cache efficiency
				node = new_node;
			}

			out->mDistance = closest;
		}
	}

private:
	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	AABBTreeToBuffer<TriangleCodec, NodeCodecAABBTree> mBuffer;
	StatsRow						mStats;
};
