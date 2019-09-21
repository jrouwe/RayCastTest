#pragma once

#include <Core/ByteBuffer.h>

// Stores the split axis for determining which order to traverse children based on 1 dimension only
class NodeCodecAABBTreeSplitAxis
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
	enum
	{
		SPLIT_AXIS_BITS				= 2,
		SPLIT_AXIS_MASK				= (1 << SPLIT_AXIS_BITS) - 1,
		HAS_TRIANGLES				= SPLIT_AXIS_MASK,
		RIGHT_BITS					= 30,
		RIGHT_MASK					= (1 << RIGHT_BITS) - 1,
		RIGHT_SHIFT					= 2,
	};

	// Compressed node structure
	struct Node
	{
		inline Vec3					GetBoundsMin() const
		{
			return Vec3(mBoundsMin);
		}

		inline Vec3					GetBoundsMax() const
		{
			return Vec3(mBoundsMax);
		}

		inline bool					HasTriangles() const
		{
			return (mNodeProperties & SPLIT_AXIS_MASK) == HAS_TRIANGLES;
		}

		inline const Node *			GetLeftChild() const
		{
			assert(!HasTriangles());
			return reinterpret_cast<const Node *>(reinterpret_cast<const uint8 *>(this) + sizeof(Node));
		}

		inline const Node *			GetRightChild() const
		{
			assert(!HasTriangles());
			return reinterpret_cast<const Node *>(reinterpret_cast<const uint8 *>(this) + ((mNodeProperties >> RIGHT_SHIFT) & RIGHT_MASK));
		}

		inline const void *			GetTriangles() const
		{
			assert(HasTriangles());
			return reinterpret_cast<const uint8 *>(this) + sizeof(Node);
		}

		inline uint					GetTriangleCount() const
		{
			assert(HasTriangles());
			return (mNodeProperties >> RIGHT_SHIFT) & RIGHT_MASK;
		}

		inline uint					GetSplitAxis() const
		{
			assert(!HasTriangles());
			return mNodeProperties & SPLIT_AXIS_MASK;
		}

	private:
		friend class NodeCodecAABBTreeSplitAxis;

		Float3						mBoundsMin;
		Float3						mBoundsMax;
		uint32						mNodeProperties;
	};
	
	class EncodingContext
	{
	public:
		inline						EncodingContext()
		{
		}

		uint						GetPessimisticMemoryEstimate(uint inNodeCount, uint inLeafNodeCount) const
		{
			return inNodeCount * sizeof(Node);
		}

		void						Finalize(Header *outHeader, const AABBTreeBuilder::Node *inRoot, uint inRootNodeStart, uint inRootTrianglesStart) const
		{
		}
		
		uint						NodeAllocate(const AABBTreeBuilder::Node *inNode, const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, vector<const AABBTreeBuilder::Node *> &ioChildren, Vec3 outChildBoundsMin[NumChildrenPerNode], Vec3 outChildBoundsMax[NumChildrenPerNode], ByteBuffer &ioBuffer) const
		{
			uint node_start = (uint)ioBuffer.size();

			// Fill in bounds
			Node *node = ioBuffer.Allocate<Node>();
			inNode->mBounds.mMin.StoreFloat3(&node->mBoundsMin);
			inNode->mBounds.mMax.StoreFloat3(&node->mBoundsMax);

			if (inNode->mTriangles.empty())
			{
				// Check that we have a valid split dimension (not all algorithms calculate this)
				if (inNode->mSplitDimension >= 3)
					FatalError("NodeCodecAABBTreeSplitAxis: Invalid split dimension");

				// Fill in node properties
				node->mNodeProperties = inNode->mSplitDimension;
			}
			else
			{
				// Fill in node properties
				uint32 tri_count = inNode->GetTriangleCount();
				node->mNodeProperties = (tri_count << RIGHT_SHIFT) | HAS_TRIANGLES;

				// Check overflows
				if ((tri_count & ~RIGHT_MASK) != 0)
					FatalError("NodeCodecAABBTreeSplitAxis: Offset too big");
			}

			return node_start;
		}

		void						NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (inNode->mTriangles.empty())
			{
				// Check number of children
				if (inNumChildren != 2)
					FatalError("NodeCodecAABBTreeSplitAxis: Invalid amount of children");

				// Check that left child immediately follows node
				if (inChildrenNodeStart[0] - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTreeSplitAxis: Invalid offset for left node");

				// Fill in node properties
				Node *node = ioBuffer.Get<Node>(inNodeStart);
				uint32 delta = inChildrenNodeStart[1] - inNodeStart;
				node->mNodeProperties |= delta << RIGHT_SHIFT;

				// Check for overflows
				if ((delta & ~RIGHT_MASK) != 0)
					FatalError("NodeCodecAABBTreeSplitAxis: Offset too big");
			}
			else
			{
				// Check offset between node and triangles
				if (inTrianglesStart - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTreeSplitAxis: Doesn't support offset between node and triangles");
			}
		}
	};
};
