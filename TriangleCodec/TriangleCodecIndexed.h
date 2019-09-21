#pragma once

#include <map>

// Store vertices as Float3 and indices as specified by template parameter Index
template <typename Index>
class TriangleCodecIndexed
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
			return inTriangleCount * 3 * (sizeof(Index) + sizeof(Float3)) + 3;
		}

		uint						Pack(const VertexList &inVertices, const IndexedTriangleList &inTriangles, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, ByteBuffer &ioBuffer)
		{
			// Determine position of triangles start
			uint offset = (uint)ioBuffer.size();

			// Update stats
			mNumTriangles += (uint)inTriangles.size();

			// Fill in vertices
			Index *vertex = ioBuffer.Allocate<Index>(inTriangles.size() * 3);
			for (const IndexedTriangle &t : inTriangles)
				for (int i = 0; i < 3; ++i)
				{
					uint32 src_vertex_index = t.mIdx[i];

					// Check if we've seen this vertex before
					Index vertex_index;
					typename map<uint32, Index>::const_iterator found = mVertexMap.find(src_vertex_index);
					if (found == mVertexMap.end())
					{
						// Check overflows
						if (mVertices.size() >= (size_t(1) << (sizeof(Index) * 8)))
							FatalError("TriangleCodecIndexed: Index overflow");

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
			outTriangleCodecName = "Indexed" + ConvertToString(sizeof(Index) * 8);
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
			mVertices(reinterpret_cast<const Float3 *>(inBuffer.Get<uint8>(0) + inHeader->mVertexOffset))
		{
		}

		f_inline void				TestRay(const Vec3 &inRayOrigin, const Vec3 &inRayDirection, const Vec3 &inBoundsMin, const Vec3 &inBoundsMax, const void *inTriangleStart, uint32 inNumTriangles, float &ioClosest) const
		{
			assert(inNumTriangles > 0);
			const Index *t = reinterpret_cast<const Index *>(inTriangleStart);
			const Index *end = t + inNumTriangles * 3;
			do
			{
				Vec3 v1(mVertices[*(t++)]);
				Vec3 v2(mVertices[*(t++)]);
				Vec3 v3(mVertices[*(t++)]);

				RayTriangle(inRayOrigin, inRayDirection, v1, v2, v3, ioClosest);
			}
			while (t < end);
		}

	private:
		const Float3 *				mVertices;
	};
};

