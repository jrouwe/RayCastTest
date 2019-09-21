#include <pch.h>

#ifdef _WIN32

#include <RayCastTest/RayCastGPUBruteForceBitPack.h>
#include <Renderer/Renderer.h>
#include <Utils/Model.h>
#include <Geometry/AABox.h>

const uint max_rays = 16384;		// Maximum supported number of raycasts
const uint component_bits = 21;
const uint component_max = (1 << component_bits) - 1;

static inline void sCompress(const Float3 &inV, const AABox &inBounds, uint32 *outCompressedData)
{
	Vec3 scaled = (Vec3(inV) - inBounds.mMin) / (inBounds.mMax - inBounds.mMin) * Vec3::sReplicate(float(component_max));
	uint32 x = int(scaled.GetX() + 0.5f);
	uint32 y = int(scaled.GetY() + 0.5f);
	uint32 z = int(scaled.GetZ() + 0.5f);
	assert(x <= component_max);
	assert(y <= component_max);
	assert(z <= component_max);

	outCompressedData[0] = x | (y << component_bits);
	outCompressedData[1] = z | ((y >> (32 - component_bits)) << component_bits);
}

void RayCastGPUBruteForceBitPack::Initialize()
{
	// Set defines
	Renderer::Defines defines;
	defines.push_back(Renderer::Define("group_size", ConvertToString(mGroupSize)));

	// Create shaders
	mShaderStep1 = mRenderer->CreateComputeShader("Shaders/RayCastGPUBruteForceClear.hlsl", &defines);
	mShaderStep2 = mRenderer->CreateComputeShader("Shaders/RayCastGPUBruteForceBitPack.hlsl", &defines);

	const uint triangle_count = mModel->GetTriangleCount();
	const uint num_groups = (triangle_count + mGroupSize - 1) / mGroupSize;

	// Calculate bounds of all triangles
	AABox bounds;
	for (uint t = 0; t < triangle_count; ++t)
		bounds.Encapsulate(mModel->GetTriangle(t));

	// Make sure box is not degenerate
	bounds.EnsureMinimalEdgeLength(1.0e-5f);

	vector<uint32> vertices;
	vertices.resize(num_groups * mGroupSize * 6);
	
	// Vertices are stored (a = first uint32 of compressed data, b = 2nd uint32):
	// V1a 0 .. V1a group_size - 1
	// V1b 0 .. V1b group_size - 1
	// V2a 0 .. V2a group_size - 1
	// V2b 0 .. V2b group_size - 1
	// V3a 0 .. V3a group_size - 1
	// V3b 0 .. V3b group_size - 1
	// V1a group_size .. V1a 2 * group_size - 1
	// ...
	for (uint t = 0, t_end = num_groups * mGroupSize; t < t_end; ++t)
	{
		int group_id = t / mGroupSize;
		int thread_id = t % mGroupSize;
		for (uint j = 0; j < 3; ++j)
		{
			uint32 data[2];
			sCompress(mModel->GetTriangle(t < triangle_count? t : 0).mV[j], bounds, data);
			for (uint k = 0; k < 2; ++k)
				vertices[(group_id * 6 + j * 2 + k) * mGroupSize + thread_id] = data[k];
		}
	}

	// Create constant buffer
	mConstantBuffer = mRenderer->CreateConstantBuffer(sizeof(DecompressionInfo));
	DecompressionInfo *info = mConstantBuffer->Map<DecompressionInfo>();
	bounds.mMin.StoreFloat3(&info->mOffset);
	Vec3 scale = (bounds.mMax - bounds.mMin) / Vec3::sReplicate(float(component_max));
	scale.StoreFloat3(&info->mScale);
	mConstantBuffer->Unmap();

	// Create triangle buffer
	mTriangleBuffer = mRenderer->CreateStructuredBuffer(CPU_ACCESS_NONE, sizeof(uint32), (uint)vertices.size(), &vertices[0]);

	// Create buffer for rays
	mRays = mRenderer->CreateStructuredBuffer(CPU_ACCESS_WRITE, sizeof(RayCastTestIn), max_rays);

	// Create output buffer
	mOutputBuffer = mRenderer->CreateRWStructuredBuffer(CPU_ACCESS_READ, sizeof(RayCastTestOut), max_rays);
}

void RayCastGPUBruteForceBitPack::CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts)
{
	const uint ray_cast_count = (uint)(inRayCastsEnd - inRayCastsBegin);
	const uint triangle_count = mModel->GetTriangleCount();
	const uint num_groups_step1 = (ray_cast_count + mGroupSize - 1) / mGroupSize;
	const uint num_groups_step2 = (triangle_count + mGroupSize - 1) / mGroupSize;

	// Copy rays to buffer
	RayCastTestIn *rays = mRays->Map<RayCastTestIn>(CPU_ACCESS_WRITE);
	memcpy(rays, inRayCastsBegin, ray_cast_count * sizeof(RayCastTestIn));
	mRays->Unmap(CPU_ACCESS_WRITE);

	// Bind stuff
	mRenderer->CSBindConstantBuffers(mConstantBuffer.get());
	mRenderer->CSBindRBuffers(mTriangleBuffer.get(), mRays.get());
	mRenderer->CSBindRWBuffers(mOutputBuffer.get());

	// Step 1 - clear output buffer
	mRenderer->CSBindShader(mShaderStep1.Get());
	mRenderer->Dispatch(num_groups_step1, 1, 1);

	// Step 2 - perform ray tests
	mRenderer->CSBindShader(mShaderStep2.Get());
	mRenderer->Dispatch(num_groups_step2, ray_cast_count, 1);

	// Write back output
	RayCastTestOut *output = mOutputBuffer->Map<RayCastTestOut>(CPU_ACCESS_READ);
	memcpy(&outRayCasts[0], output, ray_cast_count * sizeof(RayCastTestOut));
	mOutputBuffer->Unmap(CPU_ACCESS_READ);
}

#endif