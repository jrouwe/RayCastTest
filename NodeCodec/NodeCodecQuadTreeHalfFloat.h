#pragma once

#include <Core/ByteBuffer.h>
#include <Math/HalfFloat.h>
#include <AABBTree/AABBTreeBuilder.h>

template <int Alignment>
class NodeCodecQuadTreeHalfFloat
{
public:
	// Number of child nodes of this node
	static constexpr int				NumChildrenPerNode = 4;

	// Header for the tree
	struct Header
	{
		Float3							mRootBoundsMin;
		Float3							mRootBoundsMax;
		uint32							mRootProperties;
	};

	// Size of the header (an empty struct is always > 0 bytes so this needs a separate variable)
	static constexpr int				HeaderSize = sizeof(Header);
	
	// Stack size to use during DecodingContext::sWalkTree
	static constexpr int				StackSize = 128;

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
		uint16							mBoundsMinX[4];			// 4 child bounding boxes
		uint16							mBoundsMinY[4];
		uint16							mBoundsMinZ[4];
		uint16							mBoundsMaxX[4];
		uint16							mBoundsMaxY[4];
		uint16							mBoundsMaxZ[4];
		uint32							mNodeProperties[4];		// 4 child node properties
	};
	
	static_assert(sizeof(Node) == 64, "Node should be 64 bytes");

	// This class encodes and compresses quad tree nodes
	class EncodingContext
	{
	public:
		// Get an upper bound on the amount of bytes needed for a node tree with inNodeCount nodes and inLeafNodeCount leaf nodes (those that contain the triangles)
		uint							GetPessimisticMemoryEstimate(uint inNodeCount, uint inLeafNodeCount) const
		{
			return inNodeCount * (sizeof(Node) + Alignment - 1);
		}

		// Allocate a new node for inNode. 
		// Algorithm can modify the order of ioChildren to indicate in which order children should be compressed
		// Algorithm can enlarge the bounding boxes of the children during compression and returns these in outChildBoundsMin, outChildBoundsMax
		// inNodeBoundsMin, inNodeBoundsMax is the bounding box if inNode possibly widened by compressing the parent node
		uint							NodeAllocate(const AABBTreeBuilder::Node *inNode, const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, vector<const AABBTreeBuilder::Node *> &ioChildren, Vec3 outChildBoundsMin[NumChildrenPerNode], Vec3 outChildBoundsMax[NumChildrenPerNode], ByteBuffer &ioBuffer) const
		{
			// We don't emit nodes for leafs
			if (!inNode->HasChildren())
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
					node->mBoundsMinX[i] = FloatToHalfFloat<ROUND_TO_NEG_INF>(this_node->mBounds.mMin.GetX());
					node->mBoundsMinY[i] = FloatToHalfFloat<ROUND_TO_NEG_INF>(this_node->mBounds.mMin.GetY());
					node->mBoundsMinZ[i] = FloatToHalfFloat<ROUND_TO_NEG_INF>(this_node->mBounds.mMin.GetZ());
					node->mBoundsMaxX[i] = FloatToHalfFloat<ROUND_TO_POS_INF>(this_node->mBounds.mMax.GetX());
					node->mBoundsMaxY[i] = FloatToHalfFloat<ROUND_TO_POS_INF>(this_node->mBounds.mMax.GetY());
					node->mBoundsMaxZ[i] = FloatToHalfFloat<ROUND_TO_POS_INF>(this_node->mBounds.mMax.GetZ());

					// Store triangle count
					node->mNodeProperties[i] = this_node->GetTriangleCount() << TRIANGLE_COUNT_SHIFT;
					if (this_node->GetTriangleCount() & ~TRIANGLE_COUNT_MASK)
						FatalError("NodeCodecQuadTreeHalfFloat: Too many triangles");
				}
				else
				{
					// Make this an invalid triangle node
					node->mNodeProperties[i] = uint32(TRIANGLE_COUNT_MASK) << TRIANGLE_COUNT_SHIFT; 

					// Make bounding box invalid
					node->mBoundsMinX[i] = HALF_FLT_MAX;
					node->mBoundsMinY[i] = HALF_FLT_MAX;
					node->mBoundsMinZ[i] = HALF_FLT_MAX;
					node->mBoundsMaxX[i] = HALF_FLT_MAX;
					node->mBoundsMaxY[i] = HALF_FLT_MAX;
					node->mBoundsMaxZ[i] = HALF_FLT_MAX;
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

		// Once all nodes have been added, this call finalizes all nodes by patching in the offsets of the child nodes (that were added after the node itself was added)
		void							NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (!inNode->HasChildren())
				return;

			Node *node = ioBuffer.Get<Node>(inNodeStart);
			for (uint i = 0; i < inNumChildren; ++i)
			{
				// If there are triangles, use the triangle offset otherwise use the node offset
				uint offset = node->mNodeProperties[i] != 0? inChildrenTrianglesStart[i] : inChildrenNodeStart[i];
				if (offset & OFFSET_NON_SIGNIFICANT_MASK)
					FatalError("NodeCodecQuadTreeHalfFloat: Offset has non-signifiant bits set");
				offset >>= OFFSET_NON_SIGNIFICANT_BITS;
				if (offset & ~OFFSET_MASK)
					FatalError("NodeCodecQuadTreeHalfFloat: Offset too large");

				// Store offset of next node / triangles
				node->mNodeProperties[i] |= offset;
			}
		}

		// Once all nodes have been finalized, this will finalize the header of the nodes
		void							Finalize(Header *outHeader, const AABBTreeBuilder::Node *inRoot, uint inRootNodeStart, uint inRootTrianglesStart) const
		{
			uint offset = inRoot->HasChildren()? inRootNodeStart : inRootTrianglesStart;
			if (offset & OFFSET_NON_SIGNIFICANT_MASK)
				FatalError("NodeCodecQuadTreeHalfFloat: Offset has non-signifiant bits set");
			offset >>= OFFSET_NON_SIGNIFICANT_BITS;
			if (offset & ~OFFSET_MASK)
				FatalError("NodeCodecQuadTreeHalfFloat: Offset too large");

			inRoot->mBounds.mMin.StoreFloat3(&outHeader->mRootBoundsMin);
			inRoot->mBounds.mMax.StoreFloat3(&outHeader->mRootBoundsMax);
			outHeader->mRootProperties = offset + (inRoot->GetTriangleCount() << TRIANGLE_COUNT_SHIFT);
			if (inRoot->GetTriangleCount() & ~TRIANGLE_COUNT_MASK)
				FatalError("NodeCodecQuadTreeHalfFloat: Too many triangles");
		}		
	};

	// This class decodes and decompresses quad tree nodes
	class DecodingContext
	{
	public:
		// Get the amount of bits needed to store an ID to a triangle block
		inline static uint				sTriangleBlockIDBits(const ByteBuffer &inTree)
		{
			return 32 - CountLeadingZeros((uint32)inTree.size()) - OFFSET_NON_SIGNIFICANT_BITS;
		}

		// Convert a triangle block ID to the start of the triangle buffer
		inline static const void *		sGetTriangleBlockStart(const uint8 *inBufferStart, uint inTriangleBlockID)
		{
			return inBufferStart + (inTriangleBlockID << OFFSET_NON_SIGNIFICANT_BITS);
		}

		// Walk the node tree calling the Visitor::VisitNodes for each node encountered and Visitor::VisitTriangles for each triangle encountered
		template <class TriangleContext, class Visitor>
		inline static void				sWalkTree(const Header *inHeader, const uint8 *inBufferStart, const TriangleContext &inTriangleContext, Visitor &ioVisitor)
		{
			const Vec3 root_bounds_min = Vec3::sLoadFloat3Unsafe(inHeader->mRootBoundsMin);
			const Vec3 root_bounds_max = Vec3::sLoadFloat3Unsafe(inHeader->mRootBoundsMax);
			uint root_properties = inHeader->mRootProperties;
		
			uint32 node_stack[StackSize];
			node_stack[0] = root_properties;
			int top = 0;
			do
			{
				// Test if node contains triangles
				uint32 node_properties = node_stack[top];
				uint32 tri_count = node_properties >> TRIANGLE_COUNT_SHIFT;
				if (tri_count == 0)
				{
					const Node *node = reinterpret_cast<const Node *>(inBufferStart + (node_properties << OFFSET_NON_SIGNIFICANT_BITS));

					// Unpack bounds
					UVec4 bounds_minxy = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(&node->mBoundsMinX[0]));
					Vec4 bounds_minx = bounds_minxy.HalfFloatToFloat();
					Vec4 bounds_miny = bounds_minxy.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();
					
					UVec4 bounds_minzmaxx = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(&node->mBoundsMinZ[0]));
					Vec4 bounds_minz = bounds_minzmaxx.HalfFloatToFloat();
					Vec4 bounds_maxx = bounds_minzmaxx.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();

					UVec4 bounds_maxyz = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(&node->mBoundsMaxY[0]));
					Vec4 bounds_maxy = bounds_maxyz.HalfFloatToFloat();
					Vec4 bounds_maxz = bounds_maxyz.Swizzle<SWIZZLE_Z, SWIZZLE_W, SWIZZLE_UNUSED, SWIZZLE_UNUSED>().HalfFloatToFloat();

					// Load properties for 4 children
					UVec4 properties = UVec4::sLoadInt4(&node->mNodeProperties[0]);

					// Check which sub nodes to visit
					int num_results = ioVisitor.VisitNodes(bounds_minx, bounds_miny, bounds_minz, bounds_maxx, bounds_maxy, bounds_maxz, properties, top);

					// Push them onto the stack
					assert(top + 4 < StackSize);
					properties.StoreInt4(&node_stack[top]);
					top += num_results;
				}
				else
				{	
					// Node contains triangles, do individual tests
					assert(tri_count != TRIANGLE_COUNT_MASK); // This is a padding node, it should have an invalid bounding box so we shouldn't get here
					uint32 triangle_block_id = node_properties & OFFSET_MASK;
					const void *triangles = sGetTriangleBlockStart(inBufferStart, triangle_block_id);

					ioVisitor.VisitTriangles(inTriangleContext, root_bounds_min, root_bounds_max, triangles, tri_count, triangle_block_id);
				}

				// Fetch next node until we find one that the visitor wants to see
				do 
					--top;
				while (top >= 0 && !ioVisitor.ShouldVisitNode(top));
			}
			while (top >= 0);
		}
	};
};
