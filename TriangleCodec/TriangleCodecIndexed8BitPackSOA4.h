#pragma once

#include <map>

// Store vertices in 64 bits and indices in 8 bits like this:
//
// TriangleBlockHeader,
// TriangleBlock (4 triangles in 12 bytes),
// TriangleBlock...
//
// Vertices are stored:
//
// VertexData (1 vertex in 64 bits),
// VertexData...
//
// They're compressed relative to the bounding box as provided by the node codec.
class TriangleCodecIndexed8BitPackSOA4
{
public:
	class TriangleHeader
	{
	public:
		Float3						mOffset;			// Offset of all vertices
		Float3						mScale;				// Scale of all vertices, vertex_position = mOffset + mScale * compressed_vertex_position
	};

	// Size of the header (an empty struct is always > 0 bytes so this needs a separate variable)
	static constexpr int			TriangleHeaderSize = sizeof(TriangleHeader);

	// If this codec could return a different offset than the current buffer size when calling Pack()
	static constexpr bool			ChangesOffsetOnPack = false; 

	// Amount of bits per component
	enum EComponentData
	{
		COMPONENT_BITS = 21,
		COMPONENT_MASK = (1 << COMPONENT_BITS) - 1,
	};

	// Packed X and Y coordinate
	enum EVertexXY
	{
		COMPONENT_X = 0,
		COMPONENT_Y1 = COMPONENT_BITS,
		COMPONENT_Y1_BITS = 32 - COMPONENT_BITS,
	};

	// Packed Z and Y coordinate
	enum EVertexZY
	{
		COMPONENT_Z = 0,
		COMPONENT_Y2 = COMPONENT_BITS,
		COMPONENT_Y2_BITS = 31 - COMPONENT_BITS,
	};

	// A single packed vertex
	struct VertexData
	{
		uint32						mVertexXY;
		uint32						mVertexZY;
	};

	static_assert(sizeof(VertexData) == 8, "Compiler added padding");
	
	// A block of 4 triangles
	struct TriangleBlock
	{
		uint8						mIndices[3][4];				// 8 bit indices to triangle vertices for 4 triangles in the form mIndices[vertex][triangle] where vertex in [0, 2] and triangle in [0, 3]
	};

	static_assert(sizeof(TriangleBlock) == 12, "Compiler added padding");

	// A triangle header, will be followed by one or more TriangleBlocks
	struct TriangleBlockHeader
	{
		const VertexData *			GetVertexData() const		{ return reinterpret_cast<const VertexData *>(reinterpret_cast<const uint8 *>(this) + mOffsetToVertices); }
		const TriangleBlock *		GetTriangleBlock() const	{ return reinterpret_cast<const TriangleBlock *>(reinterpret_cast<const uint8 *>(this) + sizeof(TriangleBlockHeader)); }

		uint32						mOffsetToVertices;			// Offset from current block to start of vertices in bytes
	};

	static_assert(sizeof(TriangleBlockHeader) == 4, "Compiler added padding");

	// This class is used to encode and compress triangle data into a byte buffer
	class EncodingContext
	{
	public:
		EncodingContext() :
			mNumTriangles(0)
		{
		}

		// Get an upper bound on the amount of bytes needed to store inTriangleCount triangles
		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			// Worst case each triangle is alone in a block, none of the vertices are shared and we need to add 3 bytes to align the vertices
			return inTriangleCount * (sizeof(TriangleBlockHeader) + sizeof(TriangleBlock) + 3 * sizeof(VertexData)) + 3;
		}

		// Pack the triangles in inContainer to ioBuffer
		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Update stats
			uint tri_count = (uint)inTriangles.size();
			mNumTriangles += tri_count;

			// Allocate triangle block header
			TriangleBlockHeader *header = ioBuffer.Allocate<TriangleBlockHeader>();

			// Compute first vertex that this batch will use (ensuring there's enough room if none of the vertices are shared)
			uint start_vertex = Clamp((int)mVertices.size() - 256 + (int)tri_count * 3, 0, (int)mVertices.size());

			// Store the start vertex offset, this will later be patched to give the delta offset relative to the triangle block
			mOffsetsToPatch.push_back(uint((uint8 *)&header->mOffsetToVertices - (uint8 *)&ioBuffer[0]));
			header->mOffsetToVertices = start_vertex * sizeof(VertexData);

			// Pack vertices
			uint padded_triangle_count = AlignUp(tri_count, 4);
			for (uint t = 0; t < padded_triangle_count; t += 4)
			{
				TriangleBlock *block = ioBuffer.Allocate<TriangleBlock>();
				for (uint vertex_nr = 0; vertex_nr < 3; ++vertex_nr)
					for (uint block_tri_idx = 0; block_tri_idx < 4; ++block_tri_idx)
					{
						// Fetch vertex index. Create degenerate triangles for padding triangles.
						uint32 src_vertex_index = t + block_tri_idx < tri_count? inTriangles[t + block_tri_idx].mIdx[vertex_nr] : inTriangles[tri_count - 1].mIdx[0];

						// Check if we've seen this vertex before and if it is in the range that we can encode
						uint32 vertex_index;
						map<uint32, uint32>::const_iterator found = mVertexMap.find(src_vertex_index);
						if (found == mVertexMap.end() || found->second < start_vertex)
						{
							// Add vertex
							vertex_index = (uint32)mVertices.size();
							mVertexMap[src_vertex_index] = vertex_index;
							mVertices.push_back(inVertices[src_vertex_index]);
						}
						else
						{
							// Reuse vertex
							vertex_index = found->second;
						}

						// Store vertex index
						uint32 vertex_offset = vertex_index - start_vertex;
						if (vertex_offset > 0xff)
							FatalError("TriangleCodecIndexed8BitPackSOA4: Offset doesn't fit in 8 bit");
						block->mIndices[vertex_nr][block_tri_idx] = (uint8)vertex_offset;
					}
			}

			return offset;
		}

		// After all triangles have been packed, this finalizes the header and triangle buffer
		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
			// Align buffer to 4 bytes
			uint vertices_idx = (uint)ioBuffer.Align(4);

			// Patch the offsets
			for (uint o : mOffsetsToPatch)
				*ioBuffer.Get<uint32>(o) += vertices_idx - o;

			// Calculate bounding box
			AABox bounds;
			for (const Float3 &v : mVertices)
				bounds.Encapsulate(Vec3(v));

			// Compress vertices
			VertexData *vertices = ioBuffer.Allocate<VertexData>(mVertices.size());
			Vec3 compress_scale = Vec3::sSelect(Vec3::sReplicate(COMPONENT_MASK) / bounds.GetSize(), Vec3::sZero(), Vec3::sLess(bounds.GetSize(), Vec3::sReplicate(1.0e-20f)));
			for (const Float3 &v : mVertices)
			{
				UVec4 c = ((Vec3(v) - bounds.mMin) * compress_scale + Vec3::sReplicate(0.5f)).ToInt();
				assert(c.GetX() <= COMPONENT_MASK);
				assert(c.GetY()  <= COMPONENT_MASK);
				assert(c.GetZ() <= COMPONENT_MASK);
				vertices->mVertexXY = c.GetX() + (c.GetY() << COMPONENT_Y1);
				vertices->mVertexZY = c.GetZ() + ((c.GetY() >> COMPONENT_Y1_BITS) << COMPONENT_Y2);
				++vertices;
			}

			// Store decompression information
			bounds.mMin.StoreFloat3(&ioHeader->mOffset);
			(bounds.GetSize() / Vec3::sReplicate(COMPONENT_MASK)).StoreFloat3(&ioHeader->mScale);
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Indexed8BitPackSOA4";
			outVerticesPerTriangle = (float)mVertices.size() / mNumTriangles;
		}

	private:
		uint						mNumTriangles;
		vector<Float3>				mVertices;				// Output vertices, sorted according to occurance
		map<uint32, uint32>			mVertexMap;				// Maps from the original mesh vertex index (inVertices) to the index in our output vertices (mVertices)
		vector<uint>				mOffsetsToPatch;		// Offsets to the vertex buffer that need to be patched in once all nodes have been packed
	};

	// This class is used to decode and decompress triangle data packed by the EncodingContext
	class DecodingContext
	{
	private:
		// Private helper functions to unpack the 1 vertex of 4 triangles (outX contains the x coordinate of triangle 0 .. 3 etc.)
		f_inline void				Unpack(const VertexData *inVertices, UVec4 inIndex, Vec4 &outX, Vec4 &outY, Vec4 &outZ) const
		{
			// Get compressed data
			UVec4 c1 = UVec4::sGatherInt4<8>(&inVertices->mVertexXY, inIndex);
			UVec4 c2 = UVec4::sGatherInt4<8>(&inVertices->mVertexZY, inIndex);

			// Unpack the x y and z component
			UVec4 xc = UVec4::sAnd(c1, UVec4::sReplicate(COMPONENT_MASK));
			UVec4 yc = UVec4::sOr(c1.LogicalShiftRight(COMPONENT_Y1), c2.LogicalShiftRight(COMPONENT_Y2).LogicalShiftLeft(COMPONENT_Y1_BITS));
			UVec4 zc = UVec4::sAnd(c2, UVec4::sReplicate(COMPONENT_MASK));

			// Convert to float
			outX = Vec4::sFusedMultiplyAdd(xc.ToFloat(), mScaleX, mOffsetX);
			outY = Vec4::sFusedMultiplyAdd(yc.ToFloat(), mScaleY, mOffsetY);
			outZ = Vec4::sFusedMultiplyAdd(zc.ToFloat(), mScaleZ, mOffsetZ);
		}

	public:
		f_inline					DecodingContext(const TriangleHeader *inHeader, const ByteBuffer &inBuffer) :
			mOffsetX(Vec4::sReplicate(inHeader->mOffset.x)),
			mOffsetY(Vec4::sReplicate(inHeader->mOffset.y)),
			mOffsetZ(Vec4::sReplicate(inHeader->mOffset.z)),
			mScaleX(Vec4::sReplicate(inHeader->mScale.x)),
			mScaleY(Vec4::sReplicate(inHeader->mScale.y)),
			mScaleZ(Vec4::sReplicate(inHeader->mScale.z))
		{
		}

		// Tests a ray against the packed triangles
		f_inline void				TestRay(const Vec3 &inRayOrigin, const Vec3 &inRayDirection, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, const void *inTriangleStart, uint32 inNumTriangles, float &ioClosest) const
		{
			assert(inNumTriangles > 0);
			const TriangleBlockHeader *header = reinterpret_cast<const TriangleBlockHeader *>(inTriangleStart);
			const VertexData *vertices = header->GetVertexData();			
			const TriangleBlock *t = header->GetTriangleBlock();
			const TriangleBlock *end = t + ((inNumTriangles + 3) >> 2);

			Vec4 closest = Vec4::sReplicate(ioClosest);

			do
			{
				// Get the indices for the three vertices
				UVec4 iv1 = UVec4::sLoadInt(reinterpret_cast<const uint32 *>(&t->mIndices[0])).Expand4Byte0();
				UVec4 iv2 = UVec4::sLoadInt(reinterpret_cast<const uint32 *>(&t->mIndices[1])).Expand4Byte0();
				UVec4 iv3 = UVec4::sLoadInt(reinterpret_cast<const uint32 *>(&t->mIndices[2])).Expand4Byte0();

				// Decompress the triangle data
				Vec4 v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
				Unpack(vertices, iv1, v1x, v1y, v1z);
				Unpack(vertices, iv2, v2x, v2y, v2z);
				Unpack(vertices, iv3, v3x, v3y, v3z);

				// Perform ray vs triangle test
				Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z);
				closest = Vec4::sMin(distance, closest);

				++t;
			} 
			while (t < end);

			ioClosest = closest.ReduceMin();
		}

	private:
		Vec4						mOffsetX;
		Vec4						mOffsetY;
		Vec4						mOffsetZ;
		Vec4						mScaleX;
		Vec4						mScaleY;
		Vec4						mScaleZ;
	};
};
