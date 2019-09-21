#pragma once

// Pack vertices in 64 bits
class TriangleCodecBitPack
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
			return inTriangleCount * 3 * sizeof(Vertex);
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Compress vertices
			Vec3 compress_scale = sGetCompressScale(inBoundsMin, inBoundsMax);
		#ifdef _DEBUG
			Vec3 decompress_scale = sGetDecompressScale(inBoundsMin, inBoundsMax);
		#endif
			Vertex *vertex_out = ioBuffer.Allocate<Vertex>(inTriangles.size() * 3);
			for (const IndexedTriangle &triangle : inTriangles)
				for (uint32 vertex_in_idx : triangle.mIdx)
				{
					Vec3 vertex_in(inVertices[vertex_in_idx]);
					*vertex_out = sPackVertex(vertex_in, inBoundsMin, compress_scale);
				#ifdef _DEBUG
					Vec3 decompressed = sUnpackVertex(*vertex_out, inBoundsMin, decompress_scale);
					assert((decompressed - vertex_in).Length() < 1.0e-7f);
				#endif
					++vertex_out;
				}

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "BitPack";
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
			Vec3 decompress_scale = sGetDecompressScale(inBoundsMin, inBoundsMax);
			const Vertex *t = reinterpret_cast<const Vertex *>(inTriangleStart);
			const Vertex *end = t + inNumTriangles * 3;
			do
			{
				Vec3 v1 = sUnpackVertex(*t++, inBoundsMin, decompress_scale);
				Vec3 v2 = sUnpackVertex(*t++, inBoundsMin, decompress_scale);
				Vec3 v3 = sUnpackVertex(*t++, inBoundsMin, decompress_scale);

				RayTriangle(inRayOrigin, inRayDirection, v1, v2, v3, ioClosest);
			}
			while (t < end);
		}
	};

private:
	enum
	{
		COMPONENT_BITS				= 21,
		COMPONENT_MASK				= (1 << COMPONENT_BITS) - 1,
		COMPONENT_X					= 0,
		COMPONENT_Y					= COMPONENT_BITS,
		COMPONENT_Z					= 2 * COMPONENT_BITS,
	};

	typedef uint64					Vertex;

	static Vec3						sGetCompressScale(const Vec3 &inBoundsMin, const Vec3 &inBoundsMax)
	{
		return Vec3::sSelect(Vec3::sReplicate(COMPONENT_MASK) / (inBoundsMax - inBoundsMin), Vec3::sZero(), Vec3::sLess(inBoundsMax - inBoundsMin, Vec3::sReplicate(1.0e-20f)));
	}
	
	static Vertex					sPackVertex(const Vec3 &inVertex, const Vec3 &inOffset, const Vec3 &inCompressScale)
	{
		UVec4 v = ((inVertex - inOffset) * inCompressScale + Vec3::sReplicate(0.5f)).ToInt();
		assert(v.GetX() <= COMPONENT_MASK);
		assert(v.GetY() <= COMPONENT_MASK);
		assert(v.GetZ() <= COMPONENT_MASK);
		return (uint64(v.GetX()) << COMPONENT_X) | (uint64(v.GetY()) << COMPONENT_Y) | (uint64(v.GetZ()) << (COMPONENT_Z));
	}

	static f_inline Vec3		sGetDecompressScale(const Vec3 &inBoundsMin, const Vec3 &inBoundsMax)
	{
		return (inBoundsMax - inBoundsMin) * (1.0f / COMPONENT_MASK);
	}
	
	static f_inline Vec3		sUnpackVertex(const Vertex &inVertex, Vec3 inOffset, Vec3 inDecompressScale)
	{
		Vec4 compressed = UVec4::sAnd(UVec4(uint32(inVertex >> COMPONENT_X), uint32(inVertex >> COMPONENT_Y), uint32(inVertex >> COMPONENT_Z), 0), UVec4::sReplicate(COMPONENT_MASK)).ToFloat();
		return Vec3::sFusedMultiplyAdd(Vec3(compressed), inDecompressScale, inOffset);
	}
};

