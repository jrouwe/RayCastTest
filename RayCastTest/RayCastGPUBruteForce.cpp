#include <pch.h>

#ifdef _WIN32

#include <RayCastTest/RayCastGPUBruteForce.h>
#include <Renderer/Renderer.h>
#include <Utils/Model.h>

const uint max_rays = 16384;		// Maximum supported number of raycasts

void RayCastGPUBruteForce::Initialize()
{
	// Set defines
	Renderer::Defines defines;
	defines.push_back(Renderer::Define("variant", ConvertToString(mVariant)));
	defines.push_back(Renderer::Define("group_size", ConvertToString(mGroupSize)));
	defines.push_back(Renderer::Define("num_triangles_per_thread", ConvertToString(mTrianglesPerThread)));

	// Create shaders
	mShaderStep1 = mRenderer->CreateComputeShader("Shaders/RayCastGPUBruteForceClear.hlsl", &defines);
	mShaderStep2 = mRenderer->CreateComputeShader("Shaders/RayCastGPUBruteForce.hlsl", &defines);

	const uint triangle_count = mModel->GetTriangleCount();
	const uint triangles_per_thread = mTrianglesPerThread < 0? -mTrianglesPerThread : mTrianglesPerThread;
	const uint num_threads = (triangle_count + triangles_per_thread - 1) / triangles_per_thread;
	const uint num_groups = (num_threads + mGroupSize - 1) / mGroupSize;
	
	switch (mVariant)
	{
	case STORE_PER_TRIANGLE:
		{
			vector<Float3> vertices;
			vertices.resize(num_groups * mGroupSize * triangles_per_thread * 3);

			// Vertices are stored:
			// V1 0, V2 0, V3 0
			// V1 1, V2 1, V3 1
			// ...
			for (uint t = 0, t_end = num_groups * mGroupSize * triangles_per_thread; t < t_end; ++t)
				for (uint v = 0; v < 3; ++v)
					vertices[t * 3 + v] = mModel->GetTriangle(t < triangle_count? t : 0).mV[v];

			// Create vertex buffer
			mVertexBuffer = mRenderer->CreateStructuredBuffer(CPU_ACCESS_NONE, sizeof(Float3), (uint)vertices.size(), &vertices[0]);
			break;
		}

	case STORE_PER_VECTOR:
		{
			vector<Float3> vertices;
			vertices.resize(num_groups * mGroupSize * triangles_per_thread * 3);

			// Vertices are stored:
			// V1 0 .. V1 group_size - 1
			// V2 0 .. V2 group_size - 1
			// V3 0 .. V3 group_size - 1
			// V1 group_size .. V1 2 * group_size - 1
			// ...
			for (uint t = 0, t_end = num_groups * mGroupSize * triangles_per_thread; t < t_end; ++t)
			{
				int group_id = t / mGroupSize;
				int thread_id = t % mGroupSize;
				for (uint j = 0; j < 3; ++j)
					vertices[(group_id * 3 + j) * mGroupSize + thread_id] = mModel->GetTriangle(t < triangle_count? t : 0).mV[j];
			}

			// Create vertex buffer
			mVertexBuffer = mRenderer->CreateStructuredBuffer(CPU_ACCESS_NONE, sizeof(Float3), (uint)vertices.size(), &vertices[0]);
			break;
		}

	case STORE_PER_COMPONENT:
		{
			vector<float> vertices;
			vertices.resize(num_groups * mGroupSize * triangles_per_thread * 9);
	
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
			for (uint t = 0, t_end = num_groups * mGroupSize * triangles_per_thread; t < t_end; ++t)
			{
				int group_id = t / mGroupSize;
				int thread_id = t % mGroupSize;
				for (uint j = 0; j < 3; ++j)
					for (uint c = 0; c < 3; ++c)
						vertices[(group_id * 9 + j * 3 + c) * mGroupSize + thread_id] = mModel->GetTriangle(t < triangle_count? t : 0).mV[j][c];
			}
	
			// Create vertex buffer
			mVertexBuffer = mRenderer->CreateStructuredBuffer(CPU_ACCESS_NONE, sizeof(float), (uint)vertices.size(), &vertices[0]);
			break;
		}
	}

	// Create buffer for rays
	mRays = mRenderer->CreateStructuredBuffer(CPU_ACCESS_WRITE, sizeof(RayCastTestIn), max_rays);

	// Create output buffer
	mOutputBuffer = mRenderer->CreateRWStructuredBuffer(CPU_ACCESS_READ, sizeof(RayCastTestOut), max_rays);
}

void RayCastGPUBruteForce::CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts)
{
	const uint ray_cast_count = (uint)(inRayCastsEnd - inRayCastsBegin);
	const uint num_groups_step1 = (ray_cast_count + mGroupSize - 1) / mGroupSize;

	const uint triangle_count = mModel->GetTriangleCount();
	const uint triangles_per_thread = mTrianglesPerThread < 0? -mTrianglesPerThread : mTrianglesPerThread;
	const uint num_threads = (triangle_count + triangles_per_thread - 1) / triangles_per_thread;
	const uint num_groups_step2 = (num_threads + mGroupSize - 1) / mGroupSize;

	// Copy rays to buffer
	RayCastTestIn *rays = mRays->Map<RayCastTestIn>(CPU_ACCESS_WRITE);
	memcpy(rays, inRayCastsBegin, ray_cast_count * sizeof(RayCastTestIn));
	mRays->Unmap(CPU_ACCESS_WRITE);

	// Bind stuff
	mRenderer->CSBindRBuffers(mVertexBuffer.get(), mRays.get());
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