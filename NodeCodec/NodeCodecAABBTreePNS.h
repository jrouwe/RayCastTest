#pragma once

#include <Core/ByteBuffer.h>

// Uses Pierre Terdiman's Precomputed Node Sorting (see: http://www.codercorner.com/blog/?p=734)
class NodeCodecAABBTreePNS
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
		PNS_BITS					= 8,
		PNS_MASK					= 0xFF,
		RIGHT_BITS					= 24,
		RIGHT_MASK					= 0xFFFFFF,
		RIGHT_SHIFT					= 8,
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
			return (mNodeProperties & PNS_MASK) == 0;
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

		inline uint					GetPNSBit(uint inBitIndex) const
		{
			assert(inBitIndex < PNS_BITS);
			return (mNodeProperties >> inBitIndex) & 1;
		}

	private:
		friend class NodeCodecAABBTreePNS;

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
				// Fill in node properties
				node->mNodeProperties = sCalculatePNSBits(inNode->mChild[0]->mBounds, inNode->mChild[1]->mBounds);
			}
			else
			{
				// Fill in node properties
				uint32 tri_count = inNode->GetTriangleCount();
				node->mNodeProperties = tri_count << RIGHT_SHIFT;

				// Check overflows
				if ((tri_count & ~RIGHT_MASK) != 0)
					FatalError("NodeCodecAABBTreePNS: Offset too large");
			}

			return node_start;
		}

		void						NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (inNode->mTriangles.empty())
			{
				// Check number of children
				if (inNumChildren != 2)
					FatalError("NodeCodecAABBTreePNS: Invalid amount of children");

				// Check that left child immediately follows node
				if (inChildrenNodeStart[0] - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTreePNS: Invalid offset for left node");

				// Fill in node properties
				Node *node = ioBuffer.Get<Node>(inNodeStart);
				uint32 delta = inChildrenNodeStart[1] - inNodeStart;
		
				// Fill in node properties
				node->mNodeProperties |= delta << RIGHT_SHIFT;

				// Check for overflows
				if ((delta & ~RIGHT_MASK) != 0)
					FatalError("NodeCodecAABBTreePNS: Offset too large");
			}
			else
			{
				// Check offset between node and triangles
				if (inTrianglesStart - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTreePNS: Doesn't support offset between node and triangles");
			}
		}

	private:
		static uint					sCalculatePNSBits(const AABox &inLeft, const AABox &inRight)
		{
			Vec3 dir = inRight.GetCenter() - inLeft.GetCenter();

			Vec3 dir_PPP(1.0f, 1.0f, 1.0f);
			Vec3 dir_PPN(1.0f, 1.0f, -1.0f);
			Vec3 dir_PNP(1.0f, -1.0f, 1.0f);
			Vec3 dir_PNN(1.0f, -1.0f, -1.0f);
			Vec3 dir_NPP(-1.0f, 1.0f, 1.0f);
			Vec3 dir_NPN(-1.0f, 1.0f, -1.0f);
			Vec3 dir_NNP(-1.0f, -1.0f, 1.0f);
			Vec3 dir_NNN(-1.0f, -1.0f, -1.0f);
	
			const bool bit_PPP = dir.Dot(dir_PPP) > 0.0f;
			const bool bit_PPN = dir.Dot(dir_PPN) > 0.0f;
			const bool bit_PNP = dir.Dot(dir_PNP) > 0.0f;
			const bool bit_PNN = dir.Dot(dir_PNN) > 0.0f;
			const bool bit_NPP = dir.Dot(dir_NPP) > 0.0f;
			const bool bit_NPN = dir.Dot(dir_NPN) > 0.0f;
			const bool bit_NNP = dir.Dot(dir_NNP) > 0.0f;
			const bool bit_NNN = dir.Dot(dir_NNN) > 0.0f;

			uint code = 0;
			if (bit_PPP) code |= (1 << 7);
			if (bit_PPN) code |= (1 << 6);
			if (bit_PNP) code |= (1 << 5);
			if (bit_PNN) code |= (1 << 4);
			if (bit_NPP) code |= (1 << 3);
			if (bit_NPN) code |= (1 << 2);
			if (bit_NNP) code |= (1 << 1);
			if (bit_NNN) code |= (1 << 0);
			if (code == 0) // All bits zero means that there are triangles in our encoding, this can only happen if the box centers overlap in which case it doesn't matter in which order we visit the sub trees
				code = 0xf;
			return code;
		}
	};
};
