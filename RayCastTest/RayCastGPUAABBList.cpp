#include <pch.h>

#ifdef _WIN32

#include <RayCastTest/RayCastGPUAABBList.h>
#include <Renderer/Renderer.h>
#include <Utils/Model.h>
#include <TriangleGrouper/TriangleGrouperMorton.h>
#include <TriangleGrouper/TriangleGrouperClosestCentroid.h>

const uint max_rays = 16384;		// Maximum supported number of raycasts
const uint group_size = 64;
const uint triangles_per_batch = 256;

void RayCastGPUAABBList::Initialize()
{
	// Create shaders
	mShader1 = mRenderer->CreateComputeShader("Shaders/RayCastGPUAABBListStage1.hlsl");
	mShader2 = mRenderer->CreateComputeShader("Shaders/RayCastGPUAABBListStage2.hlsl");

	const vector<Triangle> &triangle_list = mModel->mTriangles;
	const uint triangle_count = (uint)triangle_list.size();
	const uint num_batches = (triangle_count + triangles_per_batch - 1) / triangles_per_batch;
	const uint num_groups = (num_batches + group_size - 1) / group_size;

	// Group triangles according to locality
	vector<uint> sorted_triangle_idx;
	if (mVariant == GROUPER_MORTON)
	{
		TriangleGrouperMorton grouper;
		grouper.Group(mModel->mTriangleVertices, mModel->mIndexedTriangles, triangles_per_batch, sorted_triangle_idx);
	}
	else if (mVariant == GROUPER_CLOSEST_CENTROID)
	{
		TriangleGrouperClosestCentroid grouper;
		grouper.Group(mModel->mTriangleVertices, mModel->mIndexedTriangles, triangles_per_batch, sorted_triangle_idx);
	}
	
	// Calculate bounds for each group
	vector<AABox> bounds;
	bounds.resize(num_batches);
	for (uint t = 0; t < triangle_count; ++t)
		bounds[t / triangles_per_batch].Encapsulate(triangle_list[sorted_triangle_idx[t]]);

	// Calculate total surface area
	mSurfaceArea = 0.0f;
	for (uint b = 0; b < (uint)bounds.size(); ++b)
		mSurfaceArea += bounds[b].GetSurfaceArea();

	vector<Float3> optimized_bounds;
	optimized_bounds.resize(num_groups * group_size * 2);

	// Boxes are stored:
	// MIN 0 .. MIN group_size - 1
	// MAX 0 .. MAX group_size - 1
	// MIN group_size .. MIN 2 * group_size - 1
	// ...
	for (uint b = 0, b_end = num_groups * group_size; b < b_end; ++b)
	{
		int group_id = b / group_size;
		int thread_id = b % group_size;
		(b < num_batches? bounds[b].mMin : Vec3::sReplicate(FLT_MAX)).StoreFloat3(&optimized_bounds[(group_id * 2 + 0) * group_size + thread_id]);
		(b < num_batches? bounds[b].mMax : Vec3::sReplicate(-FLT_MAX)).StoreFloat3(&optimized_bounds[(group_id * 2 + 1) * group_size + thread_id]);
	}

	vector<float> vertices;
	vertices.resize(num_batches * triangles_per_batch * 9);
	
	// Vertices are stored:
	// V1x 0 .. V1x group_size - 1
	// V1y 0 .. V1y group_size - 1
	// V1z 0 .. V1z group_size - 1
	// V2x 0 .. V2x group_size - 1
	// V2y 0 .. V2y group_size - 1
	// V2z 0 .. V2z group_size - 1
	// V3x 0 .. V3x group_size - 1
	// V3y 0 .. V3y group_size - 1
	// V3z 0 .. V3z group_size - 1
	// V1x group_size .. V1 2 * group_size - 1
	// ...
	for (uint t = 0, t_end = num_batches * triangles_per_batch; t < t_end; ++t)
	{
		int group_id = t / group_size;
		int thread_id = t % group_size;
		for (uint j = 0; j < 3; ++j)
			for (uint c = 0; c < 3; ++c)
				vertices[(group_id * 9 + j * 3 + c) * group_size + thread_id] = triangle_list[sorted_triangle_idx[t < triangle_count? t : 0]].mV[j][c];
	}

	// Create vertex buffer
	mVertexBuffer = mRenderer->CreateStructuredBuffer(CPU_ACCESS_NONE, sizeof(float), (uint)vertices.size(), &vertices[0]);

	// Create bounding box buffer
	mBoundingBoxBuffer = mRenderer->CreateStructuredBuffer(CPU_ACCESS_NONE, sizeof(Float3), (uint)optimized_bounds.size(), &optimized_bounds[0]);

	// Create buffer for rays
	mRays = mRenderer->CreateStructuredBuffer(CPU_ACCESS_WRITE, sizeof(RayCastTestIn), max_rays);

	// Create output buffer
	mOutputBuffer = mRenderer->CreateRWStructuredBuffer(CPU_ACCESS_READ, sizeof(RayCastTestOut), max_rays);

	// Create job buffer
	mJobBuffer = mRenderer->CreateAppendConsumeStructuredBuffer(CPU_ACCESS_NONE, sizeof(JobItem), num_batches * max_rays);
	
	// Create dispatch indirect buffer
	DispatchIndirectParams params;
	mDispatchBuffer = mRenderer->CreateRWByteAddressBufferDispatchIndirect(CPU_ACCESS_NONE, sizeof(DispatchIndirectParams), &params);
}

void RayCastGPUAABBList::CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts)
{
	const uint ray_cast_count = (uint)(inRayCastsEnd - inRayCastsBegin);
	const uint triangle_count = mModel->GetTriangleCount();
	const uint num_batches = (triangle_count + triangles_per_batch - 1) / triangles_per_batch;
	const uint num_groups = (num_batches + group_size - 1) / group_size;

	// Copy rays to buffer
	RayCastTestIn *rays = mRays->Map<RayCastTestIn>(CPU_ACCESS_WRITE);
	memcpy(rays, inRayCastsBegin, ray_cast_count * sizeof(RayCastTestIn));
	mRays->Unmap(CPU_ACCESS_WRITE);

	// Bind stuff
	mRenderer->CSBindRBuffers(mBoundingBoxBuffer.get(), mVertexBuffer.get(), mRays.get());
	mRenderer->CSBindRWBuffers(mOutputBuffer.get(), mJobBuffer.get());

	// Dispatch step 1 - aabb vs ray tests
	mRenderer->CSBindShader(mShader1.Get());
	mRenderer->Dispatch(num_groups, ray_cast_count, 1);

	// Copy size of job buffer to dispatch buffer
	mJobBuffer->CopyCounterTo(mDispatchBuffer.get(), 4);

	// Dispatch step 2 - ray vs triangle tests
	mRenderer->CSBindShader(mShader2.Get());
	mRenderer->DispatchIndirect(mDispatchBuffer.get(), 0);

	// Write back output
	RayCastTestOut *output = mOutputBuffer->Map<RayCastTestOut>(CPU_ACCESS_READ);
	memcpy(outRayCasts, output, ray_cast_count * sizeof(RayCastTestOut));
	mOutputBuffer->Unmap(CPU_ACCESS_READ);
}

#endif