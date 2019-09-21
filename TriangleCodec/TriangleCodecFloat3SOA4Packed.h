#pragma once

// As TriangleCodecFloat3SOA4 but we don't add padding vertices to round up batches of triangles to the next 4
// this adds a little bit of extra overhead to the testing since we can only test multiples of 4 and since
// the first triangle in a batch can start halfway a block of 4
class TriangleCodecFloat3SOA4Packed
{
public:
	class TriangleHeader
	{
	};

	enum { TriangleHeaderSize = 0 };

	enum { ChangesOffsetOnPack = true }; // If this codec could return a different offset than the current buffer size when calling Pack()

	class EncodingContext
	{
	public:
									EncodingContext() :
			mTrianglesStart(0),
			mCurTriangle(4)
		{
		}

		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return inTriangleCount * (4 * 3 * sizeof(Float3) + 15); // Worst case every triangle goes into a group of 4 triangles
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Align buffer
			ioBuffer.Align(16);

			// Tracks which triangle we're writing from the container
			uint container_tri = 0;

			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Check if we can continue triangles in the previous block
			if (mCurTriangle < 4 && mTrianglesStart + 3 * 3 * 4 * sizeof(float) == ioBuffer.size())
			{
				// Override offset, we're starting in the previous block
				offset = mTrianglesStart + mCurTriangle * sizeof(float);

				// Fill the remaining triangle in the block
				float *vertex = reinterpret_cast<float *>(&ioBuffer[mTrianglesStart]);
				while (mCurTriangle < 4 && container_tri < inTriangles.size())
				{
					for (int v = 0; v < 3; ++v)
						for (int c = 0; c < 3; ++c)
							vertex[mCurTriangle + c * 4 + v * 3 * 4] = inVertices[inTriangles[container_tri].mIdx[v]][c];
					++mCurTriangle;
					++container_tri;
				}
			}

			// Add remaining triangles in new blocks
			while (container_tri < inTriangles.size())
			{
				// Calculate triangle count for this block
				uint triangle_count = min<uint>((uint)inTriangles.size() - container_tri, 4);

				// Allocate new block
				mTrianglesStart = (uint)ioBuffer.size();
				mCurTriangle = triangle_count;
				float *vertex = ioBuffer.Allocate<float>(3 * 3 * 4);

				// Fill new block (and ensure that padding is also filled in)
				for (int v = 0; v < 3; ++v)
					for (int c = 0; c < 3; ++c)
						for (uint t = 0; t < 4; ++t)
							*vertex++ = inVertices[t < triangle_count? inTriangles[container_tri + t].mIdx[v] : inTriangles[container_tri + triangle_count - 1].mIdx[0]][c]; // Pad with degenerate triangles

				// Next batch of triangles
				container_tri += 4;
			}

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "Float3SOA4Packed";
			outVerticesPerTriangle = 3;
		}

	private:
		int							mTrianglesStart;
		int							mCurTriangle;
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

			// We potentially have some extra triangles in the block that don't belong to this node, add those triangles to the number of triangles to test
			uint32 triangle_in_batch = (reinterpret_cast<uint64>(inTriangleStart) >> 2) & 3;
			uint32 triangles_to_test = inNumTriangles + triangle_in_batch;

			// Get block start
			const Float4 *vertex = reinterpret_cast<const Float4 *>(reinterpret_cast<uint64>(inTriangleStart) & 0xfffffffffffffff0LL);

			for (uint b = 0; b < triangles_to_test; b += 4)
			{
				Vec4 v0x = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v0y = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v0z = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v1x = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v1y = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v1z = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v2x = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v2y = Vec4::sLoadFloat4Aligned(vertex++);
				Vec4 v2z = Vec4::sLoadFloat4Aligned(vertex++);

				Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
				closest = Vec4::sMin(distance, closest);
			}

			ioClosest = closest.ReduceMin();
		}
	};
};

