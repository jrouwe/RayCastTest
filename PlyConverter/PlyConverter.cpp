// Converts a .ply file (https://en.wikipedia.org/wiki/PLY_(file_format)) to a .model format that is compatible with our Model class
#include <pch.h>
#include <Geometry/Triangle.h>
#include <Geometry/IndexedTriangle.h>
#include <Geometry/Indexify.h>
#include <Utils/Model.h>
#include <Core/Utils.h>
#include <fstream>

static void sBigEndianToLittleEndian(uint32 &ioValue)
{
	uint8 *tmp = reinterpret_cast<uint8 *>(&ioValue);
	swap(tmp[0], tmp[3]);
	swap(tmp[1], tmp[2]);
}

static void sBigEndianToLittleEndian(float &ioValue)
{
	uint8 *tmp = reinterpret_cast<uint8 *>(&ioValue);
	swap(tmp[0], tmp[3]);
	swap(tmp[1], tmp[2]);
}

int __cdecl main(int argc, const char *argv[])
{
	try
	{
		// Check parameters
		if (argc != 3)
			FatalError("Usage: PlyConverter <input> <output>");

		// Read the entire file
		vector<uint8> data = ReadData(argv[1]);
		data.push_back(0); // Make sure it is zero terminated
		char *cur = reinterpret_cast<char *>(&data[0]);
		const char *end = cur + data.size();

		bool is_binary = false;
		int vertex_count = 0;
		int triangle_count = 0;

		// Read header
		for (;;)
		{
			// Read line
			const char *str = cur;
			cur = strchr(cur, '\n');
			if (cur == nullptr)
				FatalError("Unexpected end of file!");
			*cur = 0;
			++cur;

			// Check header element type
			if (strncmp(str, "element vertex ", 15) == 0)
				vertex_count = atoi(str + 15);
			else if (strncmp(str, "element face ", 13) == 0)
				triangle_count = atoi(str + 13);
			else if (strncmp(str, "format binary_big_endian", 24) == 0)
				is_binary = true;
			else if (strncmp(str, "end_header", 10) == 0)
				break;
		}

		// Check header validity
		if (vertex_count <= 0)
			FatalError("No vertices found in model");
		if (triangle_count <= 0)
			FatalError("No triangles found in model");

		// Make room for data
		VertexList vertices;
		IndexedTriangleList indexed_triangles;
		vertices.resize(vertex_count);
		indexed_triangles.resize(triangle_count);

		if (!is_binary)
		{
			for (int i = 0; i < vertex_count; ++i)
			{
				// Read line
				const char *str = cur;
				cur = strchr(cur, '\n');
				if (cur == nullptr)
					FatalError("Cannot parse vertex");
				*cur = 0;
				++cur;

				// Parse vertex
				float x, y, z;
				if (sscanf(str, "%f %f %f", &x, &y, &z) != 3)
					FatalError("Cannot parse vertex");

				// Set vertex
				vertices[i] = Float3(x, y, z);
			}

			for (int i = 0; i < triangle_count; ++i)
			{
				IndexedTriangle &tri = indexed_triangles[i];

				// Read line
				const char *str = cur;
				cur = strchr(cur, '\n');
				if (cur == nullptr)
					FatalError("Cannot parse triangle");
				*cur = 0;
				++cur;

				// Read indexed_triangles of face
				int c;
				if (sscanf(str, "%d %d %d %d", &c, &tri.mIdx[0], &tri.mIdx[1], &tri.mIdx[2]) != 4)
					FatalError("Cannot parse triangle");
				if (c != 3)
					FatalError("Cannot parse triangle");
			}
		}
		else
		{
			for (int i = 0; i < vertex_count; ++i)
			{
				// Read vertex position
				if (end - cur < (int)sizeof(Float3))
					FatalError("Cannot parse vertex");
				Float3 pos = *(const Float3 *)cur;
				cur += sizeof(Float3);

				// Convert endianness
				sBigEndianToLittleEndian(pos.x);
				sBigEndianToLittleEndian(pos.y);
				sBigEndianToLittleEndian(pos.z);

				// Set vertex
				vertices[i] = pos;
			}

			for (int i = 0; i < triangle_count; ++i)
			{
				IndexedTriangle &tri = indexed_triangles[i];

				// Read vertex count
				if (end - cur < (int)sizeof(uint8))
					FatalError("Cannot parse triangle");
				uint8 c = *(const uint8 *)cur;
				cur += sizeof(uint8);
				if (c != 3)
					FatalError("Cannot parse triangle");

				// Read indexed triangles of face
				if (end - cur < (int)sizeof(IndexedTriangleNoMaterial))
					FatalError("Cannot parse triangle");
				static_cast<IndexedTriangleNoMaterial &>(tri) = *reinterpret_cast<const IndexedTriangleNoMaterial *>(cur);
				cur += sizeof(IndexedTriangleNoMaterial);

				// Convert endianness
				for (int j = 0; j < 3; ++j)
					sBigEndianToLittleEndian(tri.mIdx[j]);
			}
		}

		// Throw away temporary data
		data.clear();

		// Deindex triangles
		TriangleList triangles;
		triangles.reserve(indexed_triangles.size());
		for (const IndexedTriangle &t : indexed_triangles)
		{
			Triangle out_t;
			for (int v = 0; v < 3; ++v)
				out_t.mV[v] = vertices[t.mIdx[v]];
			triangles.push_back(out_t);
		}

		// Re-index triangles
		vertices.clear();
		indexed_triangles.clear();
		Indexify(triangles, vertices, indexed_triangles);

		// Open output
		FILE *f = fopen(argv[2], "wb");
		if (f == nullptr)
			FatalError("Unable to open output file");
		
		// Write out header
		Model::ModelHeaderV1 header;
		header.mNumTriangles = (uint32)indexed_triangles.size();
		header.mNumVertices = (uint32)vertices.size();
		fwrite(&header, sizeof(header), 1, f);

		// Write vertices
		fwrite(&vertices[0], sizeof(Float3) * vertices.size(), 1, f);

		// Write indices without material
		for (uint32 t = 0; t < header.mNumTriangles; ++t)
			fwrite(&indexed_triangles[t], sizeof(IndexedTriangleNoMaterial), 1, f);
		
		// Close file
		fclose(f);

		return 0;
	}
	catch (string e)
	{
		printf("Exception caught: %s\n", e.c_str());

		return -1;
	}
}
