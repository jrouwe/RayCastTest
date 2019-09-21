#pragma once

// Uncompressed triangle codec, vertices are stored as structure of array, padded to a multiple of 4 and tested 4 at a time
template <int Alignment>
class TriangleCodecFloat3SOA4
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
			return inTriangleCount * (4 * 3 * sizeof(Float3) + Alignment - 1); // Worst case every triangle goes into a group of 4 triangles
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Align buffer
			ioBuffer.Align(Alignment);

			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Pack vertices
			uint triangle_count = (uint)inTriangles.size();
			uint padded_triangle_count = AlignUp(triangle_count, 4);
			float *vertex = ioBuffer.Allocate<float>(padded_triangle_count * 3 * 3);
			for (uint b = 0; b < padded_triangle_count; b += 4)
				for (int v = 0; v < 3; ++v)
					for (int c = 0; c < 3; ++c)
						for (uint t = 0; t < 4; ++t)
							*vertex++ = inVertices[b + t < triangle_count? inTriangles[b + t].mIdx[v] : inTriangles[triangle_count - 1].mIdx[0]][c]; // Pad with degenerate triangles

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Float3SOA4Align" + ConvertToString(Alignment);
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
			Vec4 closest = Vec4::sReplicate(ioClosest);
			
			const Float4 *vertex = reinterpret_cast<const Float4 *>(inTriangleStart);
			assert(IsAligned(vertex, Alignment));

			for (uint b = 0; b < inNumTriangles; b += 4)
			{
				Vec4 v0x = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v0y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v0z = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v1x = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v1y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v1z = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v2x = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v2y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
				Vec4 v2z = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);

				Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
				closest = Vec4::sMin(distance, closest);
			}

			ioClosest = closest.ReduceMin();
		}
	};
};

