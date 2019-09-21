#pragma once

#include <Geometry/RayTriangle8.h>

// Uncompressed triangle codec, vertices are stored as structure of array, padded to a multiple of 8 and tested 8 at a time
template <int Alignment>
class TriangleCodecFloat3SOA8
{
public:
	class TriangleHeader
	{
	};

	enum { TriangleHeaderSize = 0 };

	enum { ChangesOffsetOnPack = (Alignment != 1) }; // If this codec could return a different offset than the current buffer size when calling Pack()

	class EncodingContext
	{
	public:
		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return inTriangleCount * (8 * 3 * sizeof(Float3) + Alignment - 1); // Worst case every triangle goes into a group of 8 triangles
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Align buffer
			ioBuffer.Align(Alignment);

			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Pack vertices
			uint triangle_count = (uint)inTriangles.size();
			uint padded_triangle_count = AlignUp(triangle_count, 8);
			float *vertex = ioBuffer.Allocate<float>(padded_triangle_count * 3 * 3);
			for (uint b = 0; b < padded_triangle_count; b += 8)
				for (int v = 0; v < 3; ++v)
					for (int c = 0; c < 3; ++c)
						for (uint t = 0; t < 8; ++t)
							*vertex++ = inVertices[b + t < triangle_count? inTriangles[b + t].mIdx[v] : inTriangles[triangle_count - 1].mIdx[0]][c]; // Pad with degenerate triangles

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Float3SOA8Align" + ConvertToString(Alignment);
			outVerticesPerTriangle = 3;
		}
	};

	class DecodingContext
	{
	public:
		f_inline					DecodingContext(const TriangleHeader *inHeader, const ByteBuffer &inBuffer)
		{
		}		
		
		f_inline void				TestRay(const Vec3 &inRayOrigin, const Vec3 &inRayDirection, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, const void *inTriangleStart, uint32 inNumTriangles, float &ioClosest) const
		{
			Vec8 closest = Vec8::sReplicate(ioClosest);
			
			const float *vertex = reinterpret_cast<const float *>(inTriangleStart);
			assert(IsAligned(vertex, Alignment));

			for (uint b = 0; b < inNumTriangles; b += 8)
			{
				Vec8 v0x = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v0y = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v0z = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v1x = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v1y = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v1z = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v2x = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v2y = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;
				Vec8 v2z = Vec8LoadFloat8ConditionallyAligned<Alignment % 32 == 0>(vertex); vertex += 8;

				Vec8 distance = RayTriangle8(inRayOrigin, inRayDirection, v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
				closest = Vec8::sMin(distance, closest);
			}

			ioClosest = closest.ReduceMin();
		}
	};
};

