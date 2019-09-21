#include <pch.h>

#include <Utils/Model.h>
#include <Geometry/Indexify.h>

void Model::ReadFromFile(const char *inFileName)
{
	// Read the entire file
	vector<uint8> data = ReadData(inFileName);

	// Check header
	if (data.size() < sizeof(ModelHeaderV1))
		FatalError("File truncated");
	const ModelHeaderV1 *header = reinterpret_cast<const ModelHeaderV1 *>(&data[0]);
	if (header->mVersion != ModelHeaderV1::sVersion)
		FatalError("Invalid header");
	if (header->mNumVertices == 0)
		FatalError("No vertices");
	if (header->mNumTriangles == 0)
		FatalError("No triangles");

	// Copy vertex data
	mTriangleVertices.resize(header->mNumVertices);
	memcpy(&mTriangleVertices[0], &data[sizeof(ModelHeaderV1)], header->mNumVertices * sizeof(Float3));

	// Copy index data
	mIndexedTrianglesNoMaterial.resize(header->mNumTriangles);
	memcpy(&mIndexedTrianglesNoMaterial[0], &data[sizeof(ModelHeaderV1) + header->mNumVertices * sizeof(Float3)], header->mNumTriangles * sizeof(IndexedTriangleNoMaterial));
	
	// Get rid of loaded data
	data.clear();

	// Create index data with material
	mIndexedTriangles.resize(mIndexedTrianglesNoMaterial.size());
	for (size_t i = 0; i < mIndexedTriangles.size(); ++i)
		static_cast<IndexedTriangleNoMaterial &>(mIndexedTriangles[i]) = mIndexedTrianglesNoMaterial[i];

	// Convert to triangle list
	Deindexify(mTriangleVertices, mIndexedTriangles, mTriangles);

	// Update bounding box
	mBounds = AABox();
	for (const Float3 &v : mTriangleVertices)
		mBounds.Encapsulate(Vec3(v));
		
	// Trace result
	Trace("Model '%s' loaded, triangle_count=%d\n", inFileName, GetTriangleCount());
}
