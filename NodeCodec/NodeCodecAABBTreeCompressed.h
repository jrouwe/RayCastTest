#pragma once

#include <Core/ByteBuffer.h>

class NodeCodecAABBTreeCompressed
{
public:
	// Number of child nodes of this node
	enum { NumChildrenPerNode = 2 };

	// Header for the tree
	struct Header
	{
		Float3						mRootBoundsMin;
		Float3						mRootBoundsMax;
		uint32						mRootTriangleCount;
	};

	enum { HeaderSize = sizeof(Header) };

	// Compressed node structure
	// We compress the two child bounding boxes relative to its parent bounding box by storing the values that change.
	// Only 6 values change from parent to child and these 6 values are compressed in BoundsBits bits per value
	// relative to the parent bounding box. We store one additional bit per coordinate to indicate if the left
	// or the right child has the changed value, so if:
	// - The bit at BoundsIdxXShift is 0:
	//		child0.min.x = parent.min.x + <the compressed value for min.x> * (parent.max.x - parent.min.x) / BoundsMask
	//		child1.min.x = parent.min.x
	// - The bit at BoundsIdxXShift is 1:
	//		child0.min.x = parent.min.x
	//		child1.min.x = parent.min.x + <the compressed value for min.x> * (parent.max.x - parent.min.x) / BoundsMask
	// Same for X and Z and the maximum value of the bounding box.
	struct Node
	{
		// Constructor
									Node()												: mNodeDataMin(0), mNodeDataMax(0), mRightOffset(0) { }

		// If child nodes are nodes or triangles
		uint						GetLeftTriangleCount() const						{ return mNodeDataMin >> TrianglesShift; }
		uint						GetRightTriangleCount() const						{ return mNodeDataMax >> TrianglesShift; }

		// Pointer to child nodes or triangles
		const void *				GetLeftPtr() const									{ return reinterpret_cast<const uint8 *>(this) + sizeof(Node); }
		const void *				GetRightPtr() const									{ return reinterpret_cast<const uint8 *>(this) + mRightOffset; }

		static void					sUnpackBoundsHelperReference(const Vec3 &inNodeBoundsMin, const Vec3 &inScale, uint32 inNodeData, const Vec3 &inOriginalValue, Vec3 outChildBounds[2])
		{
			int idx = (inNodeData >> BoundsIdxXShift) & 1;
			outChildBounds[idx    ].SetX(inNodeBoundsMin.GetX() + inScale.GetX() * ((inNodeData >> BoundsXShift) & BoundsMask));
			outChildBounds[idx ^ 1].SetX(inOriginalValue.GetX());

			idx = (inNodeData >> BoundsIdxYShift) & 1;
			outChildBounds[idx    ].SetY(inNodeBoundsMin.GetY() + inScale.GetY() * ((inNodeData >> BoundsYShift) & BoundsMask));
			outChildBounds[idx ^ 1].SetY(inOriginalValue.GetY());

			idx = (inNodeData >> BoundsIdxZShift) & 1;
			outChildBounds[idx    ].SetZ(inNodeBoundsMin.GetZ() + inScale.GetZ() * ((inNodeData >> BoundsZShift) & BoundsMask));
			outChildBounds[idx ^ 1].SetZ(inOriginalValue.GetZ());
		}

		// Unpack child bounding boxes, reference imnplementation (not using any SSE tricks)
		void						UnpackBoundsReference(const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, Vec3 outChildBoundsMin[2], Vec3 outChildBoundsMax[2]) const
		{
			Vec3 scale = (inNodeBoundsMax - inNodeBoundsMin) * (1.0f / BoundsMask);
			sUnpackBoundsHelperReference(inNodeBoundsMin, scale, mNodeDataMin, inNodeBoundsMin, outChildBoundsMin);
			sUnpackBoundsHelperReference(inNodeBoundsMin, scale, mNodeDataMax, inNodeBoundsMax, outChildBoundsMax);
		}

		static f_inline void		sUnpackBoundsHelper(const Vec3 &inNodeBoundsMin, const Vec3 &inScale, uint32 inNodeData, const Vec3 &inOriginalValue, Vec3 outChildBounds[2])
		{
			// Move the X, Y and Z bytes from inNodeData into compressed.X, Y and Z
			UVec4 compressed = UVec4::sLoadBytes3(&inNodeData);

			// And out the BoundsIdx*Shift bits and scale the result so we get a split point in object space
			const UVec4 bounds_mask = UVec4::sReplicate(BoundsMask);
			Vec3 decompressed = Vec3::sFusedMultiplyAdd(Vec3(UVec4::sAnd(compressed, bounds_mask).ToFloat()), inScale, inNodeBoundsMin);

			// Shift the BoundsIdx*Shift bit left and then right so that the vector is 0xffffffff if the bit was set and 0 when it was not set
			UVec4 control = compressed.LogicalShiftLeft(24).ArithmeticShiftRight(31);

			// Select the split point or inOriginalValue based on the control bitmask
			outChildBounds[0] = Vec3::sSelect(decompressed, inOriginalValue, control);
			outChildBounds[1] = Vec3(UVec4::sXor(UVec4::sXor(outChildBounds[0].ReinterpretAsInt(), decompressed.ReinterpretAsInt()), inOriginalValue.ReinterpretAsInt()).ReinterpretAsFloat());
		}

		// Unpack child bounding boxes, using 128 bit SSE/AVX instructions
		f_inline void				UnpackBounds(const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, Vec3 outChildBoundsMin[2], Vec3 outChildBoundsMax[2]) const
		{
			Vec3 scale = (inNodeBoundsMax - inNodeBoundsMin) * (1.0f / BoundsMask);
			sUnpackBoundsHelper(inNodeBoundsMin, scale, mNodeDataMin, inNodeBoundsMin, outChildBoundsMin);
			sUnpackBoundsHelper(inNodeBoundsMin, scale, mNodeDataMax, inNodeBoundsMax, outChildBoundsMax);
		}
				
	private:
		friend class NodeCodecAABBTreeCompressed;

		// Construction helpers
		void						SetLeftTriangleCount(uint inCount)					{ if (inCount >= 0xff) FatalError("NodeCodecAABBTreeCompressed: Too many triangles"); mNodeDataMin |= inCount << TrianglesShift; }
		void						SetRightTriangleCount(uint inCount)					{ if (inCount >= 0xff) FatalError("NodeCodecAABBTreeCompressed: Too many triangles"); mNodeDataMax |= inCount << TrianglesShift; }

		static void					sPackBoundsHelper(bool inDetermineMin, float inNodeMin, float inNodeMax, float inLeft, float inRight, uint32 &outIndex, uint32 &outValue)
		{
			float min_or_max = inDetermineMin? inNodeMin : inNodeMax;

			float val;
 			if (abs(inLeft - min_or_max) < abs(inRight - min_or_max))
			{
				// Right value is the one to change
				outIndex = 1;
				val = inRight;
			}
			else 
			{
				// Left value is the one to change
				outIndex = 0;
				val = inLeft;
			}
				
			// Compress to range [0, BoundsMask]
			float cval = float(BoundsMask) * (val - inNodeMin) / (inNodeMax - inNodeMin);

			// Round up or down and convert to integer
			int ival = int(inDetermineMin? floor(cval) : ceil(cval));

			// Decompress value 
			float dval = inNodeMin + float(ival) * (inNodeMax - inNodeMin) * (1.0f / BoundsMask);
			if (inDetermineMin)
			{
				// Value became bigger than original, subtract 1
				if (dval > val)
					--ival;
			}
			else
			{
				// Value became smaller than original, add 1
				if (dval < val)
					++ival;
			}

			// Clamp value
			outValue = uint32(max(int(0), min(int(BoundsMask), ival)));

			// Decompress again to check that everything is still ok
			float dval2 = inNodeMin + float(ival) * (inNodeMax - inNodeMin) * (1.0f / BoundsMask);
			if (inDetermineMin? (dval2 > val) : (dval2 < val))
				FatalError("NodeCodecAABBTreeCompressed: Decompression failed!");
		}

		static void					sPackBoundsHelper(bool inDetermineMin, const Vec3 &inNodeMin, const Vec3 &inNodeMax, const Vec3 &inLeft, const Vec3 &inRight, uint32 &ioValue)
		{
			uint32 idx, val;

			sPackBoundsHelper(inDetermineMin, inNodeMin.GetX(), inNodeMax.GetX(), inLeft.GetX(), inRight.GetX(), idx, val);
			ioValue |= idx << BoundsIdxXShift;
			ioValue |= val << BoundsXShift;

			sPackBoundsHelper(inDetermineMin, inNodeMin.GetY(), inNodeMax.GetY(), inLeft.GetY(), inRight.GetY(), idx, val);
			ioValue |= idx << BoundsIdxYShift;
			ioValue |= val << BoundsYShift;

			sPackBoundsHelper(inDetermineMin, inNodeMin.GetZ(), inNodeMax.GetZ(), inLeft.GetZ(), inRight.GetZ(), idx, val);
			ioValue |= idx << BoundsIdxZShift;
			ioValue |= val << BoundsZShift;
		}

		void						PackBounds(const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, const Vec3 &inLeftBoundsMin, const Vec3 &inLeftBoundsMax, const Vec3 &inRightBoundsMin, const Vec3 &inRightBoundsMax)
		{
			sPackBoundsHelper(true, inNodeBoundsMin, inNodeBoundsMax, inLeftBoundsMin, inRightBoundsMin, mNodeDataMin);
			sPackBoundsHelper(false, inNodeBoundsMin, inNodeBoundsMax, inLeftBoundsMax, inRightBoundsMax, mNodeDataMax);
		}

		enum ENodeData
		{
			BoundsBits				= 7,
			BoundsMask				= (1 << BoundsBits) - 1,
			BoundsXShift			= 0,
			BoundsIdxXShift			= 7,
			BoundsYShift			= 8,
			BoundsIdxYShift			= 15,
			BoundsZShift			= 16,
			BoundsIdxZShift			= 23,
			TrianglesShift			= 24
		};
		
		uint32						mNodeDataMin;
		uint32						mNodeDataMax;
		uint32						mRightOffset;
	};

	class EncodingContext
	{
	public:
		inline						EncodingContext()
		{
		}

		uint						GetPessimisticMemoryEstimate(uint inNodeCount, uint inLeafNodeCount) const
		{
			return (inNodeCount - inLeafNodeCount) * sizeof(Node);
		}

		void						Finalize(Header *outHeader, const AABBTreeBuilder::Node *inRoot, uint inRootNodeStart, uint inRootTrianglesStart) const
		{
			// Fill in tree header
			inRoot->mBounds.mMin.StoreFloat3(&outHeader->mRootBoundsMin);
			inRoot->mBounds.mMax.StoreFloat3(&outHeader->mRootBoundsMax);
			outHeader->mRootTriangleCount = inRoot->GetTriangleCount();
		}

		uint						NodeAllocate(const AABBTreeBuilder::Node *inNode, const Vec3 &inNodeBoundsMin, const Vec3 &inNodeBoundsMax, vector<const AABBTreeBuilder::Node *> &ioChildren, Vec3 outChildBoundsMin[NumChildrenPerNode], Vec3 outChildBoundsMax[NumChildrenPerNode], ByteBuffer &ioBuffer) const
		{
			uint node_start = (uint)ioBuffer.size();

			if (inNode->mTriangles.empty())
			{
				// Fill in bounds
				Node *node = ioBuffer.Allocate<Node>();
				node->PackBounds(inNodeBoundsMin, inNodeBoundsMax, 
					inNode->mChild[0]->mBounds.mMin, inNode->mChild[0]->mBounds.mMax, 
					inNode->mChild[1]->mBounds.mMin, inNode->mChild[1]->mBounds.mMax);

				// Set flags to indicate if left and right have triangles
				node->SetLeftTriangleCount(inNode->mChild[0]->GetTriangleCount());
				node->SetRightTriangleCount(inNode->mChild[1]->GetTriangleCount());

				// Decompress bounds again
				node->UnpackBounds(inNodeBoundsMin, inNodeBoundsMax, outChildBoundsMin, outChildBoundsMax);
			}

			return node_start;
		}

		void						NodeFinalize(const AABBTreeBuilder::Node *inNode, uint inNodeStart, uint inTrianglesStart, uint inNumChildren, const uint *inChildrenNodeStart, const uint *inChildrenTrianglesStart, ByteBuffer &ioBuffer) const
		{
			if (inNode->mTriangles.empty())
			{
				// Check number of children
				if (inNumChildren != 2)
					FatalError("NodeCodecAABBTreeCompressed: Invalid amount of children");

				// Check that left child immediately follows node
				if (inChildrenNodeStart[0] - inNodeStart != sizeof(Node))
					FatalError("NodeCodecAABBTreeCompressed: Invalid offset for left node");

				// Fill in right offset
				Node *node = ioBuffer.Get<Node>(inNodeStart);
				node->mRightOffset = inChildrenNodeStart[1] - inNodeStart;
			}
			else
			{
				// Check offset between node and triangles
				if (inTrianglesStart - inNodeStart != 0)
					FatalError("NodeCodecAABBTreeCompressed: Doesn't support offset between node and triangles");
			}
		}
	};
};
