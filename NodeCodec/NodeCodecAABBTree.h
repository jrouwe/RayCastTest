#pragma once

#include <AABBTree/AABBTreeBuilder.h>
#include <Core/ByteBuffer.h>

class NodeCodecAABBTree
{
public:
	// Number of child nodes of this node
	enum { NumChildrenPerNode = 2 };

	// Header for the tree
	struct Header
	{
	};

	enum { HeaderSize = 0 };
	
	// Node properties
	static inline const uint32			HAS_TRIANGLES					= 0x80000000;
	static inline const uint32			MASK_OUT_HAS_TRIANGLES			= 0x7fffffff;

	// Compressed node structure
	struct Node
	{
		inline Vec3						GetBoundsMin() const
		{
			return Vec3(mBoundsMin);
		}

		inline Vec3						GetBoundsMax() const
		{
			return Vec3(mBoundsMax);
		}

		inline bool						HasTriangles() const
		{
			return (mNodeProperties & HAS_TRIANGLES) != 0;
		}

		inline const Node *				GetLeftChild() const
		{
			assert(!HasTriangles());
			return reinterpret_cast<const Node *>(reinterpret_cast<const uint8 *>(this) + sizeof(Node));
		}

		inline const Node *				GetRightChild() const
		{
			assert(!HasTriangles());
			return reinterpret_cast<const Node *>(reinterpret_cast<const uint8 *>(this) + (mNodeProperties & MASK_OUT_HAS_TRIANGLES));
		}

		inline const void *				GetTriangles() const
		{
			assert(HasTriangles());
			return reinterpret_cast<const uint8 *>(this) + sizeof(Node);
		}

		inline uint						GetTriangleCount() const
		{
			assert(HasTriangles());
			return mNodeProperties & MASK_OUT_HAS_TRIANGLES;
		}

	private:
		friend class NodeCodecAABBTree;

		Float3							mBoundsMin;
		Float3							mBoundsMax;
		uint32							mNodeProperties;
	};
	
	class EncodingContext
	{
	public:
		inline							EncodingContext()
		{
		}

		uint							GetPessimisticMemoryEstimate(uint inNodeCount, uint inLeafNodeCount) const
		{
			return inNodeCount * sizeof(Node);
		}

		void							Finalize(Header *outHeader, const AABBTreeBuilder::Node *inRoot, uint inRootNodeStart, uint inRootTrianglesStart) const
		{
		}
		
		uint							NodeAllocate(const AABBTreeBuilder::Node *inNode, const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, vector<const AABBTreeBuilder::Node *> &ioChildren, Vec3 outChildBoundsMin[NumChildrenPerNode], Vec3 outChildBoundsMax[NumChildrenPerNode], ByteBuffer &ioBuffer) const
		{
			uint node_start = (uint)ioBuffer.size();

			// Fill in bounds
			Node *node = ioBuffer.Allocate<Node>();
			inNode->mBounds.mMin.StoreFloat3(&node->mBoundsMin);
			inNode->mBounds.mMax.StoreFloat3(&node->mBoundsMax);

			if (!inNode->mTriangles.empty())
			{
				// Fill in node properties
				uint32 tri_count = inNode->GetTriangleCount();
				node->mNodeProperties = tri_count | HAS_TRIANGLES;

				// Check overflows
				if ((tri_count & ~MASK_OUT_HAS_TRIANGLES) != 0)
					FatalError("NodeCodecAABBTree: Too many triangles");
			}

			return node_start;
		}

		void							NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (inNode->mTriangles.empty())
			{
				// Check number of children
				if (inNumChildren != 2)
					FatalError("NodeCodecAABBTree: Invalid amount of children");

				// Check that left child immediately follows node
				if (inChildrenNodeStart[0] - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTree: Invalid offset for left node");

				// Fill in node properties
				Node *node = ioBuffer.Get<Node>(inNodeStart);
				uint32 delta = inChildrenNodeStart[1] - inNodeStart;
				node->mNodeProperties = delta;

				// Check for overflows
				if ((delta & ~MASK_OUT_HAS_TRIANGLES) != 0)
					FatalError("NodeCodecAABBTree: Too many triangles");
			}
			else
			{
				// Check offset between node and triangles
				if (inTrianglesStart - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTRee: Doesn't support offset between node and triangles");
			}
		}
	};
};
