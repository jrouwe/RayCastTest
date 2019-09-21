#pragma once

#include <Core/ByteBuffer.h>

template <int Alignment>
class NodeCodecQuadTree
{
public:
	// Number of child nodes of this node
	enum { NumChildrenPerNode = 4 };

	// Header for the tree
	struct Header
	{
		Float3							mRootBoundsMin;
		Float3							mRootBoundsMax;
		uint32							mRootProperties;
	};

	enum { HeaderSize = sizeof(Header) };
	
	// Node properties
	enum
	{
		TRIANGLE_COUNT_BITS				= 5,
		TRIANGLE_COUNT_SHIFT			= 27,
		TRIANGLE_COUNT_MASK				= (1 << TRIANGLE_COUNT_BITS) - 1,
		OFFSET_BITS						= 27,
		OFFSET_MASK						= (1 << OFFSET_BITS) - 1,
		OFFSET_NON_SIGNIFICANT_BITS		= 1,
		OFFSET_NON_SIGNIFICANT_MASK		= (1 << OFFSET_NON_SIGNIFICANT_BITS) - 1,
	};

	// Node structure
	struct Node
	{
		Float4							mBoundsMinX;			// 4 child bounding boxes
		Float4							mBoundsMinY;
		Float4							mBoundsMinZ;
		Float4							mBoundsMaxX;
		Float4							mBoundsMaxY;
		Float4							mBoundsMaxZ;
		uint32							mNodeProperties[4];		// 4 child node properties
	};
	
	class EncodingContext
	{
	public:
		inline							EncodingContext()
		{
		}

		uint							GetPessimisticMemoryEstimate(uint inNodeCount, uint inLeafNodeCount) const
		{
			return inNodeCount * (sizeof(Node) + Alignment - 1);
		}

		void							Finalize(Header *outHeader, const AABBTreeBuilder::Node *inRoot, uint inRootNodeStart, uint inRootTrianglesStart) const
		{
			uint offset = inRoot->GetTriangleCount() > 0 ? inRootTrianglesStart : inRootNodeStart;
			if (offset & OFFSET_NON_SIGNIFICANT_MASK)
				FatalError("NodeCodecQuadTree: Offset has non-signifiant bits set");
			offset >>= OFFSET_NON_SIGNIFICANT_BITS;
			if (offset & ~OFFSET_MASK)
				FatalError("NodeCodecQuadTree: Offset too large");

			inRoot->mBounds.mMin.StoreFloat3(&outHeader->mRootBoundsMin);
			inRoot->mBounds.mMax.StoreFloat3(&outHeader->mRootBoundsMax);
			outHeader->mRootProperties = offset + (inRoot->GetTriangleCount() << TRIANGLE_COUNT_SHIFT);
			if (inRoot->GetTriangleCount() & ~TRIANGLE_COUNT_MASK)
				FatalError("NodeCodecQuadTree: Too many triangles");
		}
		
		uint							NodeAllocate(const AABBTreeBuilder::Node *inNode, const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, vector<const AABBTreeBuilder::Node *> &ioChildren, Vec3 outChildBoundsMin[NumChildrenPerNode], Vec3 outChildBoundsMax[NumChildrenPerNode], ByteBuffer &ioBuffer) const
		{
			// We don't emit nodes for leafs
			if (!inNode->mTriangles.empty())
				return (uint)ioBuffer.size();
				
			// Align the buffer
			ioBuffer.Align(Alignment);
			uint node_start = (uint)ioBuffer.size();

			// Fill in bounds
			Node *node = ioBuffer.Allocate<Node>();

			for (size_t i = 0; i < 4; ++i)
			{
				if (i < ioChildren.size())
				{
					const AABBTreeBuilder::Node *this_node = ioChildren[i];

					// Copy bounding box
					reinterpret_cast<float *>(&node->mBoundsMinX)[i] = this_node->mBounds.mMin.GetX();
					reinterpret_cast<float *>(&node->mBoundsMinY)[i] = this_node->mBounds.mMin.GetY();
					reinterpret_cast<float *>(&node->mBoundsMinZ)[i] = this_node->mBounds.mMin.GetZ();
					reinterpret_cast<float *>(&node->mBoundsMaxX)[i] = this_node->mBounds.mMax.GetX();
					reinterpret_cast<float *>(&node->mBoundsMaxY)[i] = this_node->mBounds.mMax.GetY();
					reinterpret_cast<float *>(&node->mBoundsMaxZ)[i] = this_node->mBounds.mMax.GetZ();

					// Store triangle count
					node->mNodeProperties[i] = this_node->GetTriangleCount() << TRIANGLE_COUNT_SHIFT;
					if (this_node->GetTriangleCount() & ~TRIANGLE_COUNT_MASK)
						FatalError("NodeCodecQuadTree: Too many triangles");
				}
				else
				{
					// Make this an invalid triangle node
					node->mNodeProperties[i] = uint32(TRIANGLE_COUNT_MASK) << TRIANGLE_COUNT_SHIFT; 

					// Make bounding box invalid
					reinterpret_cast<float *>(&node->mBoundsMinX)[i] = FLT_MAX;
					reinterpret_cast<float *>(&node->mBoundsMinY)[i] = FLT_MAX;
					reinterpret_cast<float *>(&node->mBoundsMinZ)[i] = FLT_MAX;
					reinterpret_cast<float *>(&node->mBoundsMaxX)[i] = FLT_MAX;
					reinterpret_cast<float *>(&node->mBoundsMaxY)[i] = FLT_MAX;
					reinterpret_cast<float *>(&node->mBoundsMaxZ)[i] = FLT_MAX;
				}
			}

			// Since we don't keep track of the bounding box while descending the tree, we keep the root bounds at all levels for triangle compression
			for (int i = 0; i < NumChildrenPerNode; ++i)
			{
				outChildBoundsMin[i] = inNodeBoundsMin;
				outChildBoundsMax[i] = inNodeBoundsMax;
			}

			return node_start;
		}

		void							NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (!inNode->mTriangles.empty())
				return;

			Node *node = ioBuffer.Get<Node>(inNodeStart);
			for (size_t i = 0; i < inNumChildren; ++i)
			{
				// If there are triangles, use the triangle offset otherwise use the node offset
				uint offset = node->mNodeProperties[i] != 0 ? inChildrenTrianglesStart[i] : inChildrenNodeStart[i];
				if (offset & OFFSET_NON_SIGNIFICANT_MASK)
					FatalError("NodeCodecQuadTree: Offset has non-signifiant bits set");
				offset >>= OFFSET_NON_SIGNIFICANT_BITS;
				if (offset & ~OFFSET_MASK)
					FatalError("NodeCodecQuadTree: Offset too large");

				// Store offset of next node / triangles
				node->mNodeProperties[i] |= offset;
			}
		}
	};
};
