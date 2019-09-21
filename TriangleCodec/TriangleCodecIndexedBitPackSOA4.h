#pragma once

#include <map>

// Store vertices as uint64 and indices as type Index in structure of array with size 4 format, indices are stored like this:
//
// i1v1, i2v1, i3v1, i4v1
// i1v2, 12v2, i3v2, i4v2
// i1v3, i2v3, i3v3, i4v3
//
// where iXvY is the index for triangle X and vertex Y
//
// Vertices are stored:
//
// ----32b----
// v1x (bit 0 .. 20 -> bit 0 .. 20)
// v1y (bit 21 .. 31 -> bit 0 .. 10)
//
// ----32b----
// v1z (bit 0 .. 20 -> bit 0 .. 20)
// v1y (bit 21 .. 30 -> bit 11 .. 20)
//
template <typename Index>
class TriangleCodecIndexedBitPackSOA4
{
public:
	class TriangleHeader
	{
	public:
		Float3						mOffset;
		Float3						mScale;
		uint32						mVertexOffset;
	};

	enum { TriangleHeaderSize = sizeof(TriangleHeader) };

	enum { ChangesOffsetOnPack = false }; // If this codec could return a different offset than the current buffer size when calling Pack()

	enum
	{
		COMPONENT_BITS				= 21,
		COMPONENT_MASK				= (1 << COMPONENT_BITS) - 1,
		COMPONENT_X					= 0,
		COMPONENT_Y1				= 21,
		COMPONENT_Y1_BITS			= 32 - 21,
		COMPONENT_Y2				= 21,
		COMPONENT_Y2_BITS			= 31 - 21,
		COMPONENT_Z					= 0,
	};

	class EncodingContext
	{
	public:
									EncodingContext() :
			mNumTriangles(0)
		{
		}

		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return AlignUp(inTriangleCount, 4) * 3 * sizeof(Index) + inTriangleCount * 3 * 2 * sizeof(uint32) + 3;
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Update stats
			uint tri_count = (uint)inTriangles.size();
			mNumTriangles += tri_count;

			// Pack vertices
			uint padded_triangle_count = AlignUp(tri_count, 4);
			Index *vertex = ioBuffer.Allocate<Index>(padded_triangle_count * 3);
			for (uint t = 0; t < padded_triangle_count; t += 4)
				for (uint i = 0; i < 3; ++i)
					for (uint t2 = 0; t2 < 4; ++t2)
					{
						// Fetch vertex. Create degenerate triangles for padding triangles.
						uint32 src_vertex_index = t + t2 < tri_count? inTriangles[t + t2].mIdx[i] : inTriangles[tri_count - 1].mIdx[0];

						// Check if we've seen this vertex before
						Index vertex_index;
						typename map<uint32, Index>::const_iterator found = mVertexMap.find(src_vertex_index);
						if (found == mVertexMap.end())
						{
							// Check overflows
							if (mVertices.size() >= (size_t(1) << (sizeof(Index) * 8)))
								FatalError("TriangleCodecIndexedBitPackSOA4: Index overflow");

							// Add vertex
							vertex_index = (Index)mVertices.size();
							mVertexMap[src_vertex_index] = vertex_index;
							mVertices.push_back(inVertices[src_vertex_index]);
						}
						else
						{
							// Reuse vertex
							vertex_index = found->second;
						}

						// Store vertex index
						*vertex = vertex_index;
						++vertex;
					}

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
			// Align buffer to 4 bytes
			uint vertices_idx = (uint)ioBuffer.Align(4);

			// Calculate bounding box
			AABox bounds;
			for (const Float3 &v : mVertices)
				bounds.Encapsulate(Vec3(v));

			// Compress vertices
			uint32 *vertices = ioBuffer.Allocate<uint32>(mVertices.size() * 2);
			Vec3 compress_scale = Vec3::sSelect(Vec3::sReplicate(COMPONENT_MASK) / (bounds.mMax - bounds.mMin), Vec3::sZero(), Vec3::sLess(bounds.mMax - bounds.mMin, Vec3::sReplicate(1.0e-20f)));
			for (const Float3 &v : mVertices)
			{
				UVec4 c = ((Vec3(v) - bounds.mMin) * compress_scale + Vec3::sReplicate(0.5f)).ToInt();
				assert(c.GetX() <= COMPONENT_MASK);
				assert(c.GetY() <= COMPONENT_MASK);
				assert(c.GetZ() <= COMPONENT_MASK);
				*vertices = c.GetX() + (c.GetY() << COMPONENT_Y1);
				++vertices;
				*vertices = c.GetZ() + ((c.GetY() >> COMPONENT_Y1_BITS) << COMPONENT_Y2);
				++vertices;
			}

			// Store decompression information
			bounds.mMin.StoreFloat3(&ioHeader->mOffset);
			((bounds.mMax - bounds.mMin) / Vec3::sReplicate(COMPONENT_MASK)).StoreFloat3(&ioHeader->mScale);

			// Store offset
			ioHeader->mVertexOffset = vertices_idx;
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Indexed" + (ConvertToString(sizeof(Index) * 8) + "BitPackSOA4");
			outVerticesPerTriangle = (float)mVertices.size() / mNumTriangles;
		}

	private:
		uint						mNumTriangles;
		vector<Float3>				mVertices;				// Output vertices, sorted according to occurance
		map<uint32, Index> 			mVertexMap;				// Maps from the original mesh vertex index (inVertices) to the index in our output vertices (mVertices)
	};

	class DecodingContext
	{
	public:
		f_inline					DecodingContext(const TriangleHeader *inHeader, const ByteBuffer &inBuffer) :
			mOffsetX(Vec4::sReplicate(inHeader->mOffset.x)),
			mOffsetY(Vec4::sReplicate(inHeader->mOffset.y)),
			mOffsetZ(Vec4::sReplicate(inHeader->mOffset.z)),
			mScaleX(Vec4::sReplicate(inHeader->mScale.x)),
			mScaleY(Vec4::sReplicate(inHeader->mScale.y)),
			mScaleZ(Vec4::sReplicate(inHeader->mScale.z)),
			mVertices(reinterpret_cast<const uint32 *>(inBuffer.Get<uint8>(0) + inHeader->mVertexOffset))
		{
		}

		f_inline void				Unpack(UVec4 inIndex, Vec4 &outX, Vec4 &outY, Vec4 &outZ) const
		{
			// Get compressed data
			UVec4 c1 = UVec4::sGatherInt4<8>(mVertices + 0, inIndex);
			UVec4 c2 = UVec4::sGatherInt4<8>(mVertices + 1, inIndex);

			// Unpack the x y and z component
			UVec4 xc = UVec4::sAnd(c1, UVec4::sReplicate(COMPONENT_MASK));
			UVec4 yc = UVec4::sOr(c1.LogicalShiftRight(COMPONENT_Y1), c2.LogicalShiftRight(COMPONENT_Y2).LogicalShiftLeft(COMPONENT_Y1_BITS));
			UVec4 zc = UVec4::sAnd(c2, UVec4::sReplicate(COMPONENT_MASK));

			// Convert to float
			outX = Vec4::sFusedMultiplyAdd(xc.ToFloat(), mScaleX, mOffsetX);
			outY = Vec4::sFusedMultiplyAdd(yc.ToFloat(), mScaleY, mOffsetY);
			outZ = Vec4::sFusedMultiplyAdd(zc.ToFloat(), mScaleZ, mOffsetZ);
		}

		f_inline void				TestRay(const Vec3 &inRayOrigin, const Vec3 &inRayDirection, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, const void *inTriangleStart, uint32 inNumTriangles, float &ioClosest) const
		{
			assert(inNumTriangles > 0);
			const Index *t = reinterpret_cast<const Index *>(inTriangleStart);
			const Index *end = t + inNumTriangles * 3;

			Vec4 closest = Vec4::sReplicate(ioClosest);

			do
			{
				// Get the indices for the three vertices
				UVec4 iv1, iv2, iv3;
				if (sizeof(Index) == 2)
				{
					UVec4 iv1to3pack = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(t)); t += 8;
					iv1 = iv1to3pack.Expand4Uint16Lo();
					iv2 = iv1to3pack.Expand4Uint16Hi();
					UVec4 iv4pack = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(t)); t += 4; // Note this reads 2 uint32's extra that we don't use, but loading 2 floats is more instructions
					iv3 = iv4pack.Expand4Uint16Lo();
				}
				else if (sizeof(Index) == 4)
				{
					iv1 = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(t)); t += 4;
					iv2 = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(t)); t += 4;
					iv3 = UVec4::sLoadInt4(reinterpret_cast<const uint32 *>(t)); t += 4;
				}
				else
					assert(false);

				// Decompress the triangle data
				Vec4 v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
				Unpack(iv1, v1x, v1y, v1z);
				Unpack(iv2, v2x, v2y, v2z);
				Unpack(iv3, v3x, v3y, v3z);

				// Perform ray vs triangle test
				Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z);
				closest = Vec4::sMin(distance, closest);
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
		const uint32 *				mVertices;
	};
};

