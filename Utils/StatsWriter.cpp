#include <pch.h>

#include <Utils/StatsWriter.h>
#include <AABBTree/AABBTreeBuilder.h>
#include <AABBTree/AABBTreeToBuffer.h>

void StatsRow::StatsRow::Set(const AABBTreeBuilderStats &inStats)
{
	Set(StatsColumn::SplitterName, inStats.mSplitterStats.mSplitterName);
	Set(StatsColumn::SplitterLeafSize, inStats.mSplitterStats.mLeafSize);
	Set(StatsColumn::TreeSAHCost, inStats.mSAHCost);
	Set(StatsColumn::TreeMinDepth, inStats.mMinDepth);
	Set(StatsColumn::TreeMaxDepth, inStats.mMaxDepth);
	Set(StatsColumn::TreeNodeCount, inStats.mNodeCount);
	Set(StatsColumn::TreeLeafNodeCount, inStats.mLeafNodeCount);
	Set(StatsColumn::TrianglesPerLeaf, inStats.mMaxTrianglesPerLeaf);
	Set(StatsColumn::TreeMinTrianglesPerLeaf, inStats.mTreeMinTrianglesPerLeaf);
	Set(StatsColumn::TreeMaxTrianglesPerLeaf, inStats.mTreeMaxTrianglesPerLeaf);
	Set(StatsColumn::TreeAvgTrianglesPerLeaf, inStats.mTreeAvgTrianglesPerLeaf);
}

void StatsRow::StatsRow::Set(const AABBTreeToBufferStats &inStats)
{
	Set(StatsColumn::BufferTotalSize, inStats.mTotalSize);
	Set(StatsColumn::BufferNodesSize, inStats.mNodesSize);
	Set(StatsColumn::BufferTrianglesSize, inStats.mTrianglesSize);
	Set(StatsColumn::BufferBytesPerTriangle, inStats.mBytesPerTriangle);
	Set(StatsColumn::TriangleCodec, inStats.mTriangleCodecName);
	Set(StatsColumn::BufferVerticesPerTriangle, inStats.mVerticesPerTriangle);
}

void StatsRow::operator += (const StatsRow &inRow)
{
	for (int i = 0; i < (int)StatsColumn::NumColumns; ++i)
		if (!inRow.mColumns[i].empty())
			mColumns[i] = inRow.mColumns[i];
}

const string & StatsRow::Get(StatsColumn inColumn) const
{
	return mColumns[(int)inColumn];
}

StatsWriter::StatsWriter()
{
	// Create header line
	string header;
	for (int i = 0; i < (int)StatsColumn::NumColumns; ++i)
	{
		if (i != 0)
			header += ", ";
		header += ConvertToString((StatsColumn)i);
	}
	header += "\n";

	// Open file
	mFile.open("Timings.csv", ofstream::out | ofstream::trunc);
	if (!mFile)
		FatalError("Unable to open Timings.csv file");

	// Write header
	mFile << header;

	mFile.flush();
}

void StatsWriter::Add(const StatsRow &inRow)
{
	// Construct line
	for (int i = 0; i < (int)StatsColumn::NumColumns; ++i)
	{
		if (i != 0)
			mFile << ", ";
		mFile << inRow.mColumns[i];
	}
	mFile << "\n";

	mFile.flush();
}
