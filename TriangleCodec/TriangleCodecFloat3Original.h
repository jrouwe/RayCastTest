#pragma once

// Uncompressed triangle codec, using non-optimized raycast
class TriangleCodecFloat3Original
{
public:
	class TriangleHeader
	{
	};

	enum { TriangleHeaderSize = 0 };

	enum { ChangesOffsetOnPack = false }; // If this codec could return a different offset than the current buffer size when calling Pack()

	class EncodingContext
	{
	public:
		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return inTriangleCount * 3 * sizeof(Float3);
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Copy vertices
			Float3 *vertices = ioBuffer.Allocate<Float3>(inTriangles.size() * 3);
			for (const IndexedTriangle &t : inTriangles)
				for (int v = 0; v < 3; ++v)
					*vertices++ = inVertices[t.mIdx[v]];

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Float3Original";
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
			assert(inNumTriangles > 0);
			const Float3 *t = reinterpret_cast<const Float3 *>(inTriangleStart);
			const Float3 *end = t + inNumTriangles * 3;
			do
			{
				Vec3 v1(*(t++));
				Vec3 v2(*(t++));
				Vec3 v3(*(t++));

				float distance = RayTriangleOriginal(inRayOrigin, inRayDirection, v1, v2, v3);
				ioClosest = min(distance, ioClosest);
			}
			while (t < end);
		}
	};
};

