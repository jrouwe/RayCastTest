#include <pch.h>

#ifdef _WIN32

#include <RayCastTest/RayCastCPUAABBTreeISPC.h>

extern "C" { void __cdecl RayVsAABBTree(const RayCastTestIn raycasts[], const uint num_rays, const uint tree[], float distances[]); }

void RayCastCPUAABBTreeISPC::Initialize()
{
	// Check if stack is big enough
	if (mRoot->GetMaxDepth() >= 64)
		FatalError("RayCastCPUAABBTreeISPC: Tree too deep");

	AABBTreeToBufferStats stats;
	mBuffer.Convert(mVertices, mRoot, stats);
	mStats.Set(stats);
}

void RayCastCPUAABBTreeISPC::CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts)
{
	RayVsAABBTree(inRayCastsBegin, (uint)(inRayCastsEnd - inRayCastsBegin), (const uint *)&mBuffer.GetBuffer()[0], (float *)outRayCasts);
}

#endif