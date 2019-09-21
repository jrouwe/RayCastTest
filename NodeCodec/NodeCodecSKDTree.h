#pragma once

#include <Core/ByteBuffer.h>

class NodeCodecSKDTree
{
public:
	// Number of child nodes of this node
	enum { NumChildrenPerNode = 2 };

	// Node properties
	enum
	{
		HAS_TRIANGLES				= 0x20000000,
		PLANE_AXIS_SHIFT			= 30,
		MASK_OUT_CHILD_OFFSET		= 0x1fffffff,
	};

	// Header for the tree
	struct Header
	{
		Float3						mRootBoundsMin;
		Float3						mRootBoundsMax;
	};

	enum { HeaderSize = sizeof(Header) };

	// Data for nodes that have no triangles
	struct Planes
	{
		float						mLeftPlane;
		float						mRightPlane;
	};
	
	// Compressed node structure
	struct Node
	{
		// Get split plane axis (x = 0, y = 1, z = 2)
		uint						GetPlaneAxis() const
		{
			return mNodeProperties >> PLANE_AXIS_SHIFT;
		}

		// Get coordinate of split plane for left child (max value)
		float						GetLeftPlane() const
		{
			return reinterpret_cast<const Planes *>(reinterpret_cast<const uint8 *>(this) + sizeof(Node))->mLeftPlane;
		}

		// Get coordinate of split plane for right child (min value)
		float						GetRightPlane() const
		{
			return reinterpret_cast<const Planes *>(reinterpret_cast<const uint8 *>(this) + sizeof(Node))->mRightPlane;
		}

		inline bool					HasTriangles() const
		{
			return (mNodeProperties & HAS_TRIANGLES) != 0;
		}

		inline const Node *			GetLeftChild() const
		{
			assert(!HasTriangles());
			return reinterpret_cast<const Node *>(reinterpret_cast<const uint8 *>(this) + sizeof(Node) + sizeof(Planes));
		}

		inline const Node *			GetRightChild() const
		{
			assert(!HasTriangles());
			return reinterpret_cast<const Node *>(reinterpret_cast<const uint8 *>(this) + (mNodeProperties & MASK_OUT_CHILD_OFFSET));
		}

		inline const void *			GetTriangles() const
		{
			assert(HasTriangles());
			return reinterpret_cast<const uint8 *>(this) + sizeof(Node);
		}

		inline uint					GetTriangleCount() const
		{
			assert(HasTriangles());
			return mNodeProperties & MASK_OUT_CHILD_OFFSET;
		}

	private:
		friend class NodeCodecSKDTree;

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
			return (inNodeCount - inLeafNodeCount) * sizeof(Planes) + inNodeCount * sizeof(Node);
		}

		void						Finalize(Header *outHeader, const AABBTreeBuilder::Node *inRoot, uint inRootNodeStart, uint inRootTrianglesStart) const
		{
			// Fill in tree header
			inRoot->mBounds.mMin.StoreFloat3(&outHeader->mRootBoundsMin);
			inRoot->mBounds.mMax.StoreFloat3(&outHeader->mRootBoundsMax);
		}
		
		uint						NodeAllocate(const AABBTreeBuilder::Node *inNode, const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, vector<const AABBTreeBuilder::Node *> &ioChildren, Vec3 outChildBoundsMin[NumChildrenPerNode], Vec3 outChildBoundsMax[NumChildrenPerNode], ByteBuffer &ioBuffer) const
		{
			uint node_start = (uint)ioBuffer.size();

			if (inNode->mTriangles.empty())
			{
				// Determine which axis has the biggest empty space between the left and right child
				uint32 best_split_axis = 0;
				float best_plane_left = 0, best_plane_right = 0;
				float best_empty_space = -FLT_MAX;
				int best_first_child = 0;
				for (int c = 0; c < 3; ++c)
					for (int order = 0; order < 2; ++order)
					{
						float plane_left = inNode->mChild[order]->mBounds.mMax[c];
						float plane_right = inNode->mChild[order ^ 1]->mBounds.mMin[c];
						float empty_space = plane_right - plane_left;
						if (empty_space > best_empty_space)
						{
							best_split_axis = c;
							best_empty_space = empty_space;
							best_plane_left = plane_left;
							best_plane_right = plane_right;
							best_first_child = order;
						}
					}

				// Fill in node structure
				Node *node = ioBuffer.Allocate<Node>();
				node->mNodeProperties = best_split_axis << PLANE_AXIS_SHIFT;

				// Fill in planes structure
				Planes *planes = ioBuffer.Allocate<Planes>();
				planes->mLeftPlane = best_plane_left;
				planes->mRightPlane = best_plane_right;

				// Swap the children when first child is not 0
				if (best_first_child == 1)
					swap(ioChildren[0], ioChildren[1]);

				// Since we don't keep track of the bounding box while descending the tree, we keep the root bounds at all levels for triangle compression
				for (int i = 0; i < NumChildrenPerNode; ++i)
				{
					outChildBoundsMin[i] = inNodeBoundsMin;
					outChildBoundsMax[i] = inNodeBoundsMax;
				}
			}
			else
			{
				// Fill in node properties
				Node *node = ioBuffer.Allocate<Node>();
				uint tri_count = inNode->GetTriangleCount();
				node->mNodeProperties = tri_count | HAS_TRIANGLES;

				// Check for overflows
				if ((tri_count & ~MASK_OUT_CHILD_OFFSET) != 0)
					FatalError("NodeCodecSKDTree: Offset too large");
			}

			return node_start;
		}

		void						NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (inNode->mTriangles.empty())
			{
				// Check number of children
				if (inNumChildren != 2)
					FatalError("NodeCodecSKDTree: Invalid amount of children");

				// Check that left child immediately follows node
				if (inChildrenNodeStart[0] - inNodeStart != sizeof(Node) + sizeof(Planes))
					FatalError("NodeCodecSKDTree: Invalid offset for left node");

				// Fill in right offset
				Node *node = ioBuffer.Get<Node>(inNodeStart);
				uint delta = inChildrenNodeStart[1] - inNodeStart;
				node->mNodeProperties |= delta;

				// Check for overflows
				if ((delta & ~MASK_OUT_CHILD_OFFSET) != 0)
					FatalError("NodeCodecSKDTree: Offset too large");
			}
			else
			{
				// Check offset between node and triangles
				if (inTrianglesStart - inNodeStart != sizeof(Node))
					FatalError("NodeCodecSKDTree: Doesn't support offset between node and triangles");
			}
		}
	};
};
