#pragma once

#include <RayCastTest/RayCastTest.h>
#include <NodeCodec/NodeCodecAABBTreePNS.h>

// Raycast against AABB tree on CPU, using Pierre Terdiman's Precomputed Node Sorting (see: http://www.codercorner.com/blog/?p=734)
template <class TriangleCodec>
class RayCastCPUAABBTreePNS : public RayCastTest
{
	static const int stack_size = 64;

public:
								RayCastCPUAABBTreePNS(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void				GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTreePNS");
		ioRow += mStats;
	}

	virtual void				Initialize() override
	{
		// Check if stack is big enough
		if (mRoot->GetMaxDepth() >= (uint)stack_size)
			FatalError("RayCastCPUAABBTreePNS: Tree too deep");

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
		const typename TriangleCodec::DecodingContext ctx(mBuffer.GetTriangleHeader(), mBuffer.GetBuffer());

		RayCastTestOut *out = outRayCasts;
		for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)
		{
			Vec3 origin(ray->mOrigin);
			Vec3 direction(ray->mDirection);
			Vec3 inv_direction = direction.Reciprocal();
			UVec4 is_parallel = RayIsParallel(direction);
	#ifdef _DEBUG
			Vec3 sign_direction = Vec3(UVec4::sOr(UVec4::sAnd(direction.ReinterpretAsInt(), UVec4::sReplicate(0x80000000)), Vec3::sReplicate(1.0f).ReinterpretAsInt()).ReinterpretAsFloat());
	#endif

			// Calculate bit index for PNS
			uint32 x = signbit(ray->mDirection.x);
			uint32 y = signbit(ray->mDirection.y);
			uint32 z = signbit(ray->mDirection.z);
			uint32 bit_index = z | (y << 1) | (x << 2);

			float closest = FLT_MAX;
			const NodeCodecAABBTreePNS::Node *stack[stack_size];
			const NodeCodecAABBTreePNS::Node *node = reinterpret_cast<const NodeCodecAABBTreePNS::Node *>(mBuffer.GetRoot());
			int top = -1;
			for (;;)
			{
				// Test if node is closer than closest hit result
				if (RayAABoxHits(origin, inv_direction, is_parallel, node->GetBoundsMin(), node->GetBoundsMax(), closest))
				{
					// Test if node contains triangles
					if (!node->HasTriangles())
					{
						// Use PNS to determine which child to visit first
						const NodeCodecAABBTreePNS::Node *children[] = { node->GetLeftChild(), node->GetRightChild() };
						uint bit = node->GetPNSBit(bit_index);
						const NodeCodecAABBTreePNS::Node *first = children[bit];
						const NodeCodecAABBTreePNS::Node *second = children[bit ^ 1];
						assert(sign_direction.Dot((second->GetBoundsMin() + second->GetBoundsMax()) - (first->GetBoundsMin() + first->GetBoundsMax())) >= 0.0f); // Test that the center of the bounding boxes are so that the closest one is visited first
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
	AABBTreeToBuffer<TriangleCodec, NodeCodecAABBTreePNS> mBuffer;
	StatsRow						mStats;
};
