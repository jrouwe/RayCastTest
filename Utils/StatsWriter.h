#pragma once

#include <Core/StringTools.h>
#include <fstream>

struct AABBTreeBuilderStats;
struct AABBTreeToBufferStats;

enum class StatsColumn : int
{
	ModelName,
	TestName,
	TriangleCodec,
	TestVariant,
	TrianglesPerLeaf,
	TimeAvg,
	TimeMin,
	TimeMax,
	MaxError,
	NumSamples,
	SplitterName,
	SplitterLeafSize,
	BufferTotalSize,
	BufferNodesSize,
	BufferTrianglesSize,
	BufferVerticesPerTriangle,
	BufferBytesPerTriangle,
	TreeSAHCost,
	TreeSurfaceArea,
	TreeMinDepth,
	TreeMaxDepth,
	TreeNodeCount,
	TreeLeafNodeCount,
	TreeMinTrianglesPerLeaf,
	TreeMaxTrianglesPerLeaf,
	TreeAvgTrianglesPerLeaf,
	NumColumns
};

inline const char *ConvertToString(StatsColumn inValue)
{
	switch (inValue)
	{
	#define VALUE(x) case StatsColumn::x: return #x
	VALUE(ModelName);
	VALUE(TestName);
	VALUE(TriangleCodec);
	VALUE(TestVariant);
	VALUE(TrianglesPerLeaf);
	VALUE(TimeAvg);
	VALUE(TimeMin);
	VALUE(TimeMax);
	VALUE(MaxError);
	VALUE(NumSamples);
	VALUE(SplitterName);
	VALUE(SplitterLeafSize);
	VALUE(BufferTotalSize);
	VALUE(BufferNodesSize);
	VALUE(BufferTrianglesSize);
	VALUE(BufferVerticesPerTriangle);
	VALUE(BufferBytesPerTriangle);
	VALUE(TreeSAHCost);
	VALUE(TreeSurfaceArea);
	VALUE(TreeMinDepth);
	VALUE(TreeMaxDepth);
	VALUE(TreeNodeCount);
	VALUE(TreeLeafNodeCount);
	VALUE(TreeMinTrianglesPerLeaf);
	VALUE(TreeMaxTrianglesPerLeaf);
	VALUE(TreeAvgTrianglesPerLeaf);
	VALUE(NumColumns);
	#undef VALUE
	}

	assert(false);
	return nullptr;
}

class StatsRow
{
public:
	void				Set(StatsColumn inColumn, const char *inValue)
	{
		mColumns[(int)inColumn] = inValue;
	}

	void				Set(const AABBTreeBuilderStats &inStats);
	void				Set(const AABBTreeToBufferStats &inStats);

	template <typename T>
	void				Set(StatsColumn inColumn, T inValue)
	{
		mColumns[(int)inColumn] = ConvertToString(inValue);
	}

	void				operator += (const StatsRow &inRow);

	const string &		Get(StatsColumn inColumn) const;

private:
	friend class StatsWriter;

	string				mColumns[(int)StatsColumn::NumColumns];
};

// Write out timings and other statistics in a formatted way for easy processing in Excel
class StatsWriter
{
public:
						StatsWriter();

	void				Add(const StatsRow &inRow);

private:
	ofstream			mFile;
};