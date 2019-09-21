#pragma once

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <NodeCodec/NodeCodecAABBTree.h>

// Raycast against AABB tree on CPU
// Variant 2: Tests bounds at each level of the tree, then checks left and right subtrees to decide if / which child to visit first
template <class TriangleCodec>
class RayCastCPUAABBTree2 : public RayCastTest
{
	static const int stack_size = 64;

public:
									RayCastCPUAABBTree2(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTree2");
		ioRow += mStats;
	}

	virtual void					Initialize() override
	{
		// Check if stack is big enough
		if (mRoot->GetMaxDepth() >= (uint)stack_size)
			FatalError("RayCastCPUAABBTree2: Tree too deep");

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

						// Test bounds of left child
						float left_distance = RayAABox(origin, inv_direction, is_parallel, left_child->GetBoundsMin(), left_child->GetBoundsMax());
						bool left_intersects = left_distance < closest;

						// Test bounds of right child
						float right_distance = RayAABox(origin, inv_direction, is_parallel, right_child->GetBoundsMin(), right_child->GetBoundsMax());
						bool right_intersects = right_distance < closest;

						if (left_intersects && right_intersects)
						{
							// Both collide
							if (left_distance < right_distance)
							{
								// Left child before right child
								assert(top < stack_size - 1);
								stack[++top] = right_child;
								node = left_child;
							}
							else
							{
								// Right child before left child
								assert(top < stack_size - 1);
								stack[++top] = left_child;
								node = right_child;
							}
							continue;
						}
						else if (left_intersects)
						{
							// Only left collides
							node = left_child;
							continue;
						}
						else if (right_intersects)
						{
							// Only right collides
							node = right_child;
							continue;
						}
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
				node = stack[top--];
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
