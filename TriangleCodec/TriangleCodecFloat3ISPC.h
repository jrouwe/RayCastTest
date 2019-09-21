#pragma once

// Exported function from ISPC to test a ray against a list of triangles in SOA format
extern "C" { float __cdecl RayVsTriangleList(const float origin[3], const float direction[3], const float triangles[], int num_triangles); }

// Uncompressed triangle codec, vertices are stored as structure of array and triangles are tested using ISPC (Intel SPMD Program Compiler)
class TriangleCodecFloat3ISPC
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

			// Pack vertices
			uint triangle_count = (uint)inTriangles.size();
			float *vertex = ioBuffer.Allocate<float>(triangle_count * 3 * 3);
			for (int v = 0; v < 3; ++v)
				for (int c = 0; c < 3; ++c)
					for (uint t = 0; t < triangle_count; ++t)
						*vertex++ = inVertices[inTriangles[t].mIdx[v]][c];

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Float3ISPC";
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
			float distance = RayVsTriangleList((const float *)&inRayOrigin, (const float *)&inRayDirection, (const float *)inTriangleStart, inNumTriangles);
			ioClosest = min(ioClosest, distance);
		}
	};
};

