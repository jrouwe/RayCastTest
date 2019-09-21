#pragma once

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <NodeCodec/NodeCodecSKDTree.h>

// Raycast against SKD tree on CPU, see e.g.: "On the Fast Construction of Spatial Hierarchies for Ray Tracing" - Vlastimil Havran
template <class TriangleCodec>
class RayCastCPUSKDTree : public RayCastTest
{
	static const int stack_size = 64;

public:
								RayCastCPUSKDTree(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void				GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUSKDTree");
		ioRow += mStats;
	}

	virtual void				Initialize() override
	{
		// Check if stack is big enough
		if (mRoot->GetMaxDepth() >= (uint)stack_size)
			FatalError("RayCastCPUSKDTree: Tree too deep");

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

		const NodeCodecSKDTree::Header *header = mBuffer.GetNodeHeader();
		const Vec3 root_bounds_min(header->mRootBoundsMin);
		const Vec3 root_bounds_max(header->mRootBoundsMax);

		RayCastTestOut *out = outRayCasts;
		for (const RayCastTestIn *ray = inRayCastsBegin; ray < inRayCastsEnd; ++ray, ++out)		
		{
			Vec3 origin(ray->mOrigin);
			Vec3 direction(ray->mDirection);
			Vec3 inv_direction = direction.Reciprocal();
			UVec4 is_parallel = RayIsParallel(direction);

			struct StackEntry
			{
				const NodeCodecSKDTree::Node *	mNode;
				float							mFractionNear;
				float							mFractionFar;
			};

			float closest = FLT_MAX;

			StackEntry stack[stack_size];
			float fraction_near, fraction_far;
			const NodeCodecSKDTree::Node *node = reinterpret_cast<const NodeCodecSKDTree::Node *>(mBuffer.GetRoot());
			RayAABox(origin, inv_direction, is_parallel, root_bounds_min, root_bounds_max, fraction_near, fraction_far);
			int top = -1;
			for (;;)
			{
				if (fraction_near <= fraction_far)
				{
					// Test if node contains triangles
					if (!node->HasTriangles())
					{
						const NodeCodecSKDTree::Node *left_child = node->GetLeftChild();
						const NodeCodecSKDTree::Node *right_child = node->GetRightChild();

						float left_fraction_near = fraction_near, left_fraction_far = fraction_far;
						bool left_intersects = GetHitFraction(origin, direction, node->GetPlaneAxis(), node->GetLeftPlane(), -1.0f, left_fraction_near, left_fraction_far);

						float right_fraction_near = fraction_near, right_fraction_far = fraction_far;
						bool right_intersects = GetHitFraction(origin, direction, node->GetPlaneAxis(), node->GetRightPlane(), 1.0f, right_fraction_near, right_fraction_far);

						if (left_intersects && right_intersects)
						{
							// Both collide
							if (left_fraction_near < right_fraction_near)
							{
								// Left child before right child
								assert(top < stack_size - 1);
								++top;
								stack[top].mNode = right_child;
								stack[top].mFractionNear = right_fraction_near;
								stack[top].mFractionFar = right_fraction_far;
								node = left_child;
								fraction_near = left_fraction_near;
								fraction_far = left_fraction_far;
							}
							else
							{
								// Right child before left child
								assert(top < stack_size - 1);
								++top;
								stack[top].mNode = left_child;
								stack[top].mFractionNear = left_fraction_near;
								stack[top].mFractionFar = left_fraction_far;
								node = right_child;
								fraction_near = right_fraction_near;
								fraction_far = right_fraction_far;
							}
							continue;
						}
						else if (left_intersects)
						{
							// Only left collides
							node = left_child;
							fraction_near = left_fraction_near;
							fraction_far = left_fraction_far;
							continue;
						}
						else if (right_intersects)
						{
							// Only right collides
							node = right_child;
							fraction_near = right_fraction_near;
							fraction_far = right_fraction_far;
							continue;
						}
					}
					else
					{	
						// Node contains triangles, do individual tests
						ctx.TestRay(origin, direction, root_bounds_min, root_bounds_max, node->GetTriangles(), node->GetTriangleCount(), closest);
					}
				}

				// Fetch next node
				if (top < 0)
					break;
				node = stack[top].mNode;
				fraction_near = stack[top].mFractionNear;
				fraction_far = min(closest, stack[top].mFractionFar);
				--top;
			}

			out->mDistance = closest;
		}
	}

private:
	static f_inline bool 		GetHitFraction(const Vec3 &inOrigin, const Vec3 &inDirection, uint inAxis, float inCoordinate, float inSide, float &outFractionNear, float &outFractionFar)
	{
		float dist_to_plane = inOrigin[inAxis] - inCoordinate;
		float direction = inDirection[inAxis];

		// Check if ray is parallel to plane
		if (direction > -1.0e-12f && direction < 1.0e-12f)
		{
			// Check if ray is on the right side of the plane
			return inSide * dist_to_plane >= 0.0f;
		}
		else
		{
			// Update fraction
			float intersection = -dist_to_plane / direction;
			if (inSide * direction > 0.0f)
				outFractionNear = max(outFractionNear, intersection);
			else
				outFractionFar = min(outFractionFar, intersection);

			// Return if there is still a possibility for a hit
			return outFractionNear <= outFractionFar;
		}
	}

	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	AABBTreeToBuffer<TriangleCodec, NodeCodecSKDTree> mBuffer;
	StatsRow						mStats;
};
