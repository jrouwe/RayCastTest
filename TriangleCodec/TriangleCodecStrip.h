#pragma once

#include <Stripify/Stripify.h>

#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
#endif

// Store vertices as triangle strip with each vertex encoded according to the vertex codec
template <class VertexCodec>
class TriangleCodecStrip
{
public:
	class TriangleHeader
	{
		static const uint Size = 0;
	};

	enum { TriangleHeaderSize = 0 };

	enum { ChangesOffsetOnPack = false }; // If this codec could return a different offset than the current buffer size when calling Pack()

	class EncodingContext
	{
	public:
									EncodingContext() : 
			mNumTriangles(0),
			mNumVertices(0)
		{ 
		}

		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return inTriangleCount * 3 * sizeof(typename VertexCodec::Vertex);
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Strip the triangles
			Stripify::TriangleStrip strips;
			Stripify::sStripify(inTriangles, strips);

			// Update stats
			mNumTriangles += (uint)inTriangles.size();
			mNumVertices += (uint)strips.size();

			// Convert to stripped vertex list
			typename VertexCodec::Vertex *vertex = ioBuffer.Allocate<typename VertexCodec::Vertex>(strips.size());
			const typename VertexCodec::EncodingContext codec(inBoundsMin, inBoundsMax);
			for (auto &s : strips)
				*vertex++ = codec.PackVertex(inVertices[s.mIndex], s.mFlags);

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = VertexCodec::sGetName();
			outVerticesPerTriangle = (float)mNumVertices / mNumTriangles;
		}

	private:
		uint						mNumTriangles;
		uint						mNumVertices;
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

			const typename VertexCodec::Vertex *vertex = reinterpret_cast<const typename VertexCodec::Vertex *>(inTriangleStart);
			const typename VertexCodec::DecodingContext codec(inBoundsMin, inBoundsMax);
			Vec3 v[3];

			// Read first 2 vertices
			assert(codec.GetFlags(*vertex) == STRIPIFY_FLAG_START_STRIP_V1);
			v[0] = codec.UnpackVertex(*vertex++);

			assert(codec.GetFlags(*vertex) == STRIPIFY_FLAG_START_STRIP_V2);
			v[1] = codec.UnpackVertex(*vertex++);

			// Assume vertices are already in the right order
			assert(codec.GetFlags(*vertex) == (STRIPIFY_FLAG_A_IS_V1 | STRIPIFY_FLAG_B_IS_A_PLUS_1));
			v[2] = codec.UnpackVertex(*vertex++);

			// Test intersection
			RayTriangle(inRayOrigin, inRayDirection, v[0], v[1], v[2], ioClosest);

			for (uint triangles_left = inNumTriangles - 1; triangles_left > 0; --triangles_left)
			{
				uint32 flags = codec.GetFlags(*vertex);
				if (flags == STRIPIFY_FLAG_START_STRIP_V1)
				{
					// Start of new strip, load first 2 vertices
					v[0] = codec.UnpackVertex(*vertex++);

					assert(codec.GetFlags(*vertex) == STRIPIFY_FLAG_START_STRIP_V2);
					v[1] = codec.UnpackVertex(*vertex++);

					// Assume vertices are already in the right order
					assert(codec.GetFlags(*vertex) == (STRIPIFY_FLAG_A_IS_V1 | STRIPIFY_FLAG_B_IS_A_PLUS_1));
				}
				else
				{
					// Continuation of strip, determine first 2 vertices
					uint32 a = flags & STRIPIFY_FLAG_A_MASK;
					uint32 b = (a + ((flags & STRIPIFY_FLAG_B_MASK) >> STRIPIFY_FLAG_B_SHIFT) + 1) % 3;
					Vec3 va = v[a];
					Vec3 vb = v[b];
					v[0] = va;
					v[1] = vb;
				}

				// Load third vertex
				v[2] = codec.UnpackVertex(*vertex++);

				// Test intersection
				RayTriangle(inRayOrigin, inRayDirection, v[0], v[1], v[2], ioClosest);
			}
		}
	};
};

// Codec that doesn't compress vertices but stores the strip flags in the LSB of each component
class UncompressedStripCodec
{
public:
	typedef Float3 				Vertex;

	// Name of codec
	static const char *			sGetName()
	{
		return "UncompressedStrip";
	}

	class EncodingContext
	{
	public:
		// Constructor
								EncodingContext(const Vec3 &inBoundsMin, const Vec3 &inBoundsMax)
		{
		}

		// Pack a vertex
		Vertex					PackVertex(const Float3 &inVertex, uint inFlags) const
		{
			// Encode 3 bits of flags into lowest bit of x, y and z components
			assert(inFlags < 8);
			Vertex v = inVertex;
			if ((inFlags & 1) != 0)
				reinterpret_cast<uint32 &>(v.x) |= 0x00000001;
			else
				reinterpret_cast<uint32 &>(v.x) &= 0xfffffffe;
			if ((inFlags & 2) != 0)
				reinterpret_cast<uint32 &>(v.y) |= 0x00000001;
			else
				reinterpret_cast<uint32 &>(v.y) &= 0xfffffffe;
			if ((inFlags & 4) != 0)
				reinterpret_cast<uint32 &>(v.z) |= 0x00000001;
			else
				reinterpret_cast<uint32 &>(v.z) &= 0xfffffffe;
			return v;
		}
	};

	class DecodingContext
	{
	public:
		// Constructor
								DecodingContext(const Vec3 &inBoundsMin, const Vec3 &inBoundsMax)
		{
		}

		// Get flags for current vertex
		f_inline uint			GetFlags(const Vertex &inVertex) const
		{
			return (reinterpret_cast<const uint32 &>(inVertex.x) & 1) + ((reinterpret_cast<const uint32 &>(inVertex.y) & 1) << 1) + ((reinterpret_cast<const uint32 &>(inVertex.z) & 1) << 2);
		}

		// Unpack current vertex
		f_inline Vec3			UnpackVertex(const Vertex &inVertex) const
		{
			return Vec3(UVec4::sAnd(Vec3(inVertex).ReinterpretAsInt(), UVec4::sReplicate(0xfffffffe)).ReinterpretAsFloat());
		}
	};
};

// Codec that compresses the stripped triangle in 64-bits per vertex (relative to bounding box of node)
class CompressedStripCodec
{
public:
	typedef uint64				Vertex;

	// Name of codec
	static const char *			sGetName()
	{
		return "CompressedStrip";
	}

	class EncodingContext
	{
	public:
		// Constructor
								EncodingContext(const Vec3 &inBoundsMin, const Vec3 &inBoundsMax) :
			mOffset(inBoundsMin),
			mScale(Vec3::sSelect(Vec3::sReplicate(VertexMask) / (inBoundsMax - inBoundsMin), Vec3::sZero(), Vec3::sLess(inBoundsMax - inBoundsMin, Vec3::sReplicate(1.0e-20f))))
		{
		}

		// Pack a vertex
		Vertex					PackVertex(const Float3 &inVertex, uint inFlags) const
		{
			// Encode 3 bits of flags into lowest bit of x, y and z components
			assert(inFlags < 8);
			UVec4 packed = ((Vec3(inVertex) - mOffset) * mScale + Vec3::sReplicate(0.5f)).ToInt();
			assert(packed.GetX() <= VertexMask);
			assert(packed.GetY() <= VertexMask);
			assert(packed.GetZ() <= VertexMask);
			return (uint64(packed.GetX()) << VertexShiftX) + (uint64(packed.GetY()) << VertexShiftY) + (uint64(packed.GetZ()) << VertexShiftZ) + inFlags;
		}

	private:
		Vec3					mOffset;
		Vec3					mScale;
	};

	class DecodingContext
	{
	public:
		// Constructor
		f_inline				DecodingContext(const Vec3 &inBoundsMin, const Vec3 &inBoundsMax) :
			mOffset(inBoundsMin),
			mScale((inBoundsMax - inBoundsMin) * (1.0f / VertexMask))
		{
		}

		// Get flags for current vertex
		f_inline uint			GetFlags(const Vertex &inVertex) const
		{
			return inVertex & StripifyFlagsMask;
		}

		// Unpack current vertex
		f_inline Vec3			UnpackVertex(const Vertex &inVertex) const
		{
			Vec4 compressed = UVec4::sAnd(UVec4(uint32(inVertex >> VertexShiftX), uint32(inVertex >> VertexShiftY), uint32(inVertex >> VertexShiftZ), 0), UVec4::sReplicate(VertexMask)).ToFloat();
			return Vec3::sFusedMultiplyAdd(Vec3(compressed), mScale, mOffset);
		}
	
	private:
		Vec3					mOffset;
		Vec3					mScale;
	};

private:
	enum EVertexData
	{
		VertexBits				= 20,
		VertexMask				= (1 << VertexBits) - 1,
		StripifyFlagsMask		= 7,
		VertexShiftX			= 4,
		VertexShiftY			= 24,
		VertexShiftZ			= 44,
	};
};

typedef TriangleCodecStrip<UncompressedStripCodec> TriangleCodecStripUncompressed;
typedef TriangleCodecStrip<CompressedStripCodec> TriangleCodecStripCompressed;