#pragma once

#ifdef _WIN32

#include <RayCastTest/RayCastTest.h>
#include <AABBTree/AABBTreeToBuffer.h>
#include <TriangleCodec/TriangleCodecFloat3.h>
#include <NodeCodec/NodeCodecAABBTree.h>

// Raycast against AABB tree on CPU, does a full tree walk using ISPC compiler
class RayCastCPUAABBTreeISPC : public RayCastTest
{
public:
									RayCastCPUAABBTreeISPC(const VertexList &inVertices, const AABBTreeBuilder::Node *inRoot) : mVertices(inVertices), mRoot(inRoot) { }

	virtual void					GetStats(StatsRow &ioRow) const override
	{
		ioRow.Set(StatsColumn::TestName, "RayCastCPUAABBTreeISPC");
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
	AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree> mBuffer;
	StatsRow						mStats;
};

#endif