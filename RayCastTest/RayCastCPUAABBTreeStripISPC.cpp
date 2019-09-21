#include <pch.h>

#ifdef _WIN32

#include <RayCastTest/RayCastCPUAABBTreeStripISPC.h>

extern "C" { void __cdecl RayVsAABBTreeStripped(const RayCastTestIn raycasts[], const uint num_rays, const uint tree[], float distances[]); }

void RayCastCPUAABBTreeStripISPC::Initialize()
{
	// Check if stack is big enough
	if (mRoot->GetMaxDepth() >= 64)
		FatalError("RayCastCPUAABBTreeStripISPC: Tree too deep");

	AABBTreeToBufferStats stats;
	mBuffer.Convert(mVertices, mRoot, stats);
	mStats.Set(stats);
}

void RayCastCPUAABBTreeStripISPC::CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts)
{
	RayVsAABBTreeStripped(inRayCastsBegin, (uint)(inRayCastsEnd - inRayCastsBegin), (const uint *)&mBuffer.GetBuffer()[0], (float *)outRayCasts);
}

#endif