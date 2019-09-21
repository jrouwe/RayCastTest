#pragma once

// Store vertices in structure of array format, 16 bits per vertex component followed by decompression scale and offset:
//
// -----------64b------------
// t1v1x, t2v1x, t3v1x, t4v1x
// t1v1y, t2v1y, t3v1y, t4v1y
// t1v1z, t2v1z, t3v1z, t4v1z
// t1v2x, t2v2x, t3v2x, t4v2x
// t1v2y, t2v2y, t3v2y, t4v2y
// t1v2z, t2v2z, t3v2z, t4v2z
// t1v3x, t2v3x, t3v3x, t4v3x
// t1v3y, t2v3y, t3v3y, t4v3y
// t1v3z, t2v3z, t3v3z, t4v3z
// -----------32b------------
// scalex
// scaley
// scalez
// offsetx
// offsety
// offsetz
//
template <int Alignment, bool IntToFloatTrick>
class TriangleCodecBitPackSOA4
{
public:
	class TriangleHeader
	{
	};

	enum { TriangleHeaderSize = 0 };

	enum { ChangesOffsetOnPack = (Alignment != 1) }; // If this codec could return a different offset than the current buffer size when calling Pack()

	enum
	{
		COMPONENT_BITS				= 16,
		COMPONENT_MASK				= (1 << COMPONENT_BITS) - 1,
	};

	class EncodingContext
	{
	public:
		uint						GetPessimisticMemoryEstimate(uint inTriangleCount) const
		{
			return inTriangleCount * (4 * 3 * 3 * sizeof(uint16) + 2 * sizeof(Float3) + Alignment - 1); // Worst case every triangle goes into a group of 4 triangles
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Realign buffer
			ioBuffer.Align(Alignment);

			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Sort triangles so that we put triangles that are close in one block
			vector<uint> sorted_triangle_idx;
			TriangleGrouperMorton grouper;
			grouper.Group(inVertices, inTriangles, 4, sorted_triangle_idx);

			// Pack vertices
			uint triangle_count = (uint)inTriangles.size();
			for (uint b = 0; b < triangle_count; b += 4)
			{
				// Calculate bounding box for this batch of 4 triangles
				AABox bounds;
				for (uint t = 0; t < 4; ++t)
					bounds.Encapsulate(inVertices, inTriangles[sorted_triangle_idx[b + t < triangle_count ? b + t : triangle_count - 1]]);

				// Determine scale based on bounding box
				Vec3 compress_scale = Vec3::sSelect(Vec3::sReplicate(COMPONENT_MASK) / (bounds.mMax - bounds.mMin), Vec3::sZero(), Vec3::sLess(bounds.mMax - bounds.mMin, Vec3::sReplicate(1.0e-20f)));

				// Allocate vertices
				uint16 *vertex = ioBuffer.Allocate<uint16>(4 * 3 * 3);

				for (int v = 0; v < 3; ++v)
					for (int c = 0; c < 3; ++c)
						for (uint t = 0; t < 4; ++t)
						{
							float component = inVertices[b + t < triangle_count? inTriangles[sorted_triangle_idx[b + t]].mIdx[v] : inTriangles[sorted_triangle_idx[triangle_count - 1]].mIdx[0]][c];
							uint32 compressed = (uint32)((component - bounds.mMin[c]) * compress_scale[c] + 0.5f);
							assert(compressed <= COMPONENT_MASK);
							*vertex++ = uint16(compressed);
						}

				// Decompression scale
				Vec3 decompress_scale;
				if (IntToFloatTrick)
				{
					union { float f; uint32 i; } hi, lo;
					hi.i = 0x3f8fffff;
					lo.i = 0x3f8f0000;
					decompress_scale = (bounds.mMax - bounds.mMin) / Vec3::sReplicate(hi.f - lo.f);
				}
				else
				{
					decompress_scale = (bounds.mMax - bounds.mMin) / Vec3::sReplicate(COMPONENT_MASK);
				}

				// Store decompression scale and offset
				Float3 *scale_and_offset = ioBuffer.Allocate<Float3>(2);
				decompress_scale.StoreFloat3(scale_and_offset);
				++scale_and_offset;
				bounds.mMin.StoreFloat3(scale_and_offset);
			}

			return offset;
		}

		void						Finalize(TriangleHeader *ioHeader, ByteBuffer &ioBuffer) const
		{
		}

		void						GetStats(string &outTriangleCodecName, float &outVerticesPerTriangle)
		{
			// Store stats
			outTriangleCodecName = "BitPackSOA4Align" + ConvertToString(Alignment) + "IntToFloat" + ConvertToString(IntToFloatTrick);
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
			const Float4 *vertex = reinterpret_cast<const Float4 *>(inTriangleStart);
			assert(IsAligned(vertex, Alignment));

			Vec4 closest = Vec4::sReplicate(ioClosest);

			if (IntToFloatTrick)
			{
				UVec4 mask(0x3f8f0000, 0x3f8f0000, 0x3f8f0000, 0x3f8f0000);

				for (uint b = 0; b < inNumTriangles; b += 4)
				{
					Vec4 v0xv0y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v0zv1x = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v1yv1z = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v2xv2y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v2zsxsy = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 szoxoyoz = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);

					Vec4 v0x = (UVec4::sOr(v0xv0y.ReinterpretAsInt().Expand4Uint16Lo(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * v2zsxsy.SplatZ() + szoxoyoz.SplatY();
					Vec4 v0y = (UVec4::sOr(v0xv0y.ReinterpretAsInt().Expand4Uint16Hi(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * v2zsxsy.SplatW() + szoxoyoz.SplatZ();
					Vec4 v0z = (UVec4::sOr(v0zv1x.ReinterpretAsInt().Expand4Uint16Lo(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * szoxoyoz.SplatX() + szoxoyoz.SplatW();
					Vec4 v1x = (UVec4::sOr(v0zv1x.ReinterpretAsInt().Expand4Uint16Hi(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * v2zsxsy.SplatZ() + szoxoyoz.SplatY();
					Vec4 v1y = (UVec4::sOr(v1yv1z.ReinterpretAsInt().Expand4Uint16Lo(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * v2zsxsy.SplatW() + szoxoyoz.SplatZ();
					Vec4 v1z = (UVec4::sOr(v1yv1z.ReinterpretAsInt().Expand4Uint16Hi(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * szoxoyoz.SplatX() + szoxoyoz.SplatW();
					Vec4 v2x = (UVec4::sOr(v2xv2y.ReinterpretAsInt().Expand4Uint16Lo(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * v2zsxsy.SplatZ() + szoxoyoz.SplatY();
					Vec4 v2y = (UVec4::sOr(v2xv2y.ReinterpretAsInt().Expand4Uint16Hi(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * v2zsxsy.SplatW() + szoxoyoz.SplatZ();
					Vec4 v2z = (UVec4::sOr(v2zsxsy.ReinterpretAsInt().Expand4Uint16Lo(), mask).ReinterpretAsFloat() - mask.ReinterpretAsFloat()) * szoxoyoz.SplatX() + szoxoyoz.SplatW();

					Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
					closest = Vec4::sMin(distance, closest);
				}
			}
			else
			{
				for (uint b = 0; b < inNumTriangles; b += 4)
				{
					Vec4 v0xv0y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v0zv1x = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v1yv1z = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v2xv2y = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 v2zsxsy = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);
					Vec4 szoxoyoz = Vec4LoadFloat4ConditionallyAligned<Alignment % 16 == 0>(vertex++);

					Vec4 v0x = v0xv0y.ReinterpretAsInt().Expand4Uint16Lo().ToFloat() * v2zsxsy.SplatZ() + szoxoyoz.SplatY();
					Vec4 v0y = v0xv0y.ReinterpretAsInt().Expand4Uint16Hi().ToFloat() * v2zsxsy.SplatW() + szoxoyoz.SplatZ();
					Vec4 v0z = v0zv1x.ReinterpretAsInt().Expand4Uint16Lo().ToFloat() * szoxoyoz.SplatX() + szoxoyoz.SplatW();
					Vec4 v1x = v0zv1x.ReinterpretAsInt().Expand4Uint16Hi().ToFloat() * v2zsxsy.SplatZ() + szoxoyoz.SplatY();
					Vec4 v1y = v1yv1z.ReinterpretAsInt().Expand4Uint16Lo().ToFloat() * v2zsxsy.SplatW() + szoxoyoz.SplatZ();
					Vec4 v1z = v1yv1z.ReinterpretAsInt().Expand4Uint16Hi().ToFloat() * szoxoyoz.SplatX() + szoxoyoz.SplatW();
					Vec4 v2x = v2xv2y.ReinterpretAsInt().Expand4Uint16Lo().ToFloat() * v2zsxsy.SplatZ() + szoxoyoz.SplatY();
					Vec4 v2y = v2xv2y.ReinterpretAsInt().Expand4Uint16Hi().ToFloat() * v2zsxsy.SplatW() + szoxoyoz.SplatZ();
					Vec4 v2z = v2zsxsy.ReinterpretAsInt().Expand4Uint16Lo().ToFloat() * szoxoyoz.SplatX() + szoxoyoz.SplatW();

					Vec4 distance = RayTriangle4(inRayOrigin, inRayDirection, v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
					closest = Vec4::sMin(distance, closest);
				}
			}

			ioClosest = closest.ReduceMin();
		}
	};
};

