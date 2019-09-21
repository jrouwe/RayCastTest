#pragma once

#include <map>

// Store vertices as Float3 and indices as type Index in structure of array with size 4 format, indices are stored like this:
//
// i1v1, i2v1, i3v1, i4v1
// i1v2, 12v2, i3v2, i4v2
// i1v3, i2v3, i3v3, i4v3
//
// where iXvY is the index for triangle X and vertex Y
template <typename Index>
class TriangleCodecIndexedSOA4
{
public:
	class TriangleHeader
	{
	public:
		uint32						mVertexOffset;
	};

	enum { TriangleHeaderSize = sizeof(TriangleHeader) };

	enum { ChangesOffsetOnPack = false }; // If this codec could return a different offset than the current buffer size when calling Pack()

	class EncodingContext
	{
	public:
									EncodingContext() :
			mNumTriangles(0)
		{
		}

		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return AlignUp(inTriangleCount, 4) * 3 * sizeof(Index) + inTriangleCount * 3 * sizeof(Float3) + 3;
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Update stats
			uint tri_count = (uint)inTriangles.size();
			mNumTriangles += tri_count;

			// Fill in vertices
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
								FatalError("TriangleCodecIndexedSOA4: Index overflow");

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

			// Copy vertices
			ioBuffer.AppendVector(mVertices);

			// Store offset
			ioHeader->mVertexOffset = vertices_idx;
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Indexed" + (ConvertToString(sizeof(Index) * 8) + "SOA4");
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
			mVertices(reinterpret_cast<const float *>(inBuffer.Get<uint8>(0) + inHeader->mVertexOffset))
		{
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

				// Multiply by 3 to get the offset in the mVertices array (because 1 vertex = 3 floats)
				UVec4 three = UVec4::sReplicate(3);
				iv1 = iv1 * three;
				iv2 = iv2 * three;
				iv3 = iv3 * three;

				// Load the vertices of the 4 triangles
				Vec4 v1x = Vec4::sGatherFloat4<4>(mVertices + 0, iv1);
				Vec4 v1y = Vec4::sGatherFloat4<4>(mVertices + 1, iv1);
				Vec4 v1z = Vec4::sGatherFloat4<4>(mVertices + 2, iv1);
				Vec4 v2x = Vec4::sGatherFloat4<4>(mVertices + 0, iv2);
				Vec4 v2y = Vec4::sGatherFloat4<4>(mVertices + 1, iv2);
				Vec4 v2z = Vec4::sGatherFloat4<4>(mVertices + 2, iv2);
				Vec4 v3x = Vec4::sGatherFloat4<4>(mVertices + 0, iv3);
				Vec4 v3y = Vec4::sGatherFloat4<4>(mVertices + 1, iv3);
				Vec4 v3z = Vec4::sGatherFloat4<4>(mVertices + 2, iv3);

				Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z);
				closest = Vec4::sMin(distance, closest);
			}
			while (t < end);

			ioClosest = closest.ReduceMin();
		}

	private:
		const float *				mVertices;
	};
};

