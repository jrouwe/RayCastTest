#pragma once

#ifdef _WIN32

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <TriangleCodec/TriangleCodecStrip.h>
#include <NodeCodec/NodeCodecAABBTree.h>

// Raycast against AABB tree on CPU, using ISPC compiler
class RayCastCPUAABBTreeStripISPC : public RayCastTest
{
public:
									RayCastCPUAABBTreeStripISPC(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTreeStripISPC");
		ioRow += mStats;
	}

	virtual void					Initialize() override;

	virtual void					TrashCache() override
	{
		CacheTrasher::sTrash(mBuffer.GetBuffer());
	}

	virtual void					CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) override;

private:
	const VertexList &				mVertices;
	const AABBTreeBuilder::Node *	mRoot;
	AABBTreeToBuffer<TriangleCodecStripUncompressed, NodeCodecAABBTree> mBuffer;
	StatsRow						mStats;
};

#endif