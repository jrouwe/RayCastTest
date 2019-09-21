#include <pch.h> // IWYU pragma: keep

#include <AABBTree/AABBTreeBuilder.h>
#include <Core/ProgressIndicator.h>

// Constructor
AABBTreeBuilder::Node::Node() :
	mSplitDimension((uint)-1),
	mSplitValue(0.0f)
{ 
	mChild[0] = nullptr; 
	mChild[1] = nullptr; 
}

// Destructor
AABBTreeBuilder::Node::~Node() 
{ 
	delete mChild[0]; 
	delete mChild[1]; 
}

// Min depth of tree
uint AABBTreeBuilder::Node::GetMinDepth() const
{
	if (HasChildren())
	{
		uint left = mChild[0]->GetMinDepth();
		uint right = mChild[1]->GetMinDepth();
		return min(left, right) + 1;
	}
	else
		return 1;
}

// Max depth of tree
uint AABBTreeBuilder::Node::GetMaxDepth() const
{
	if (HasChildren())
	{
		uint left = mChild[0]->GetMaxDepth();
		uint right = mChild[1]->GetMaxDepth();
		return max(left, right) + 1;
	}
	else
		return 1;
}

// Number of nodes in tree
uint AABBTreeBuilder::Node::GetNodeCount() const
{
	if (HasChildren())
		return mChild[0]->GetNodeCount() + mChild[1]->GetNodeCount() + 1;
	else
		return 1;
}

// Number of leaf nodes in tree
uint AABBTreeBuilder::Node::GetLeafNodeCount() const
{
	if (HasChildren())
		return mChild[0]->GetLeafNodeCount() + mChild[1]->GetLeafNodeCount();
	else
		return 1;
}

// Get triangle count in tree
uint AABBTreeBuilder::Node::GetTriangleCountInTree() const
{
	if (HasChildren())
		return mChild[0]->GetTriangleCountInTree() + mChild[1]->GetTriangleCountInTree();
	else
		return GetTriangleCount();
}

// Calculate min and max triangles per node
void AABBTreeBuilder::Node::GetTriangleCountPerNode(float &outAverage, uint &outMin, uint &outMax) const
{
	outMin = INT_MAX;
	outMax = 0;
	outAverage = 0;
	uint avg_divisor = 0;
	GetTriangleCountPerNodeInternal(outAverage, avg_divisor, outMin, outMax);
	if (avg_divisor > 0)
		outAverage /= avg_divisor;
}

// Calculate the total cost of the tree using the surface area heuristic
float AABBTreeBuilder::Node::CalculateSAHCost(float inCostTraversal, float inCostLeaf) const
{
	float surface_area = mBounds.GetSurfaceArea();
	return CalculateSAHCostInternal(inCostTraversal / surface_area, inCostLeaf / surface_area);
}

// Recursively get children (breadth first) to get in total inN children (or less if there are no more)
void AABBTreeBuilder::Node::GetNChildren(uint inN, vector<const Node *> &outChildren) const
{
	assert(outChildren.empty());

	// Check if there is anything to expand
	if (!HasChildren())
		return;

	// Start with the children of this node
	outChildren.push_back(mChild[0]);
	outChildren.push_back(mChild[1]);

	size_t next = 0;
	bool all_triangles = true;
	while (outChildren.size() < inN)
	{
		// If we have looped over all nodes, start over with the first node again
		if (next >= outChildren.size())
		{
			// If there only triangle nodes left, we have to terminate
			if (all_triangles)
				return; 
			next = 0;
			all_triangles = true;
		}

		// Try to expand this node into its two children
		const Node *to_expand = outChildren[next];
		if (to_expand->HasChildren())
		{
			outChildren.erase(outChildren.begin() + next);
			outChildren.push_back(to_expand->mChild[0]);
			outChildren.push_back(to_expand->mChild[1]);
			all_triangles = false;
		}
		else
		{
			++next;
		}
	}
}

// Recursive helper function to calculate cost of the tree
float AABBTreeBuilder::Node::CalculateSAHCostInternal(float inCostTraversalDivSurfaceArea, float inCostLeafDivSurfaceArea) const
{
	if (HasChildren())
		return inCostTraversalDivSurfaceArea * mBounds.GetSurfaceArea() 
			+ mChild[0]->CalculateSAHCostInternal(inCostTraversalDivSurfaceArea, inCostLeafDivSurfaceArea) 
			+ mChild[1]->CalculateSAHCostInternal(inCostTraversalDivSurfaceArea, inCostLeafDivSurfaceArea);
	else
		return inCostLeafDivSurfaceArea * mBounds.GetSurfaceArea() * GetTriangleCount();
}

// Recursive helper function to calculate min and max triangles per node
void AABBTreeBuilder::Node::GetTriangleCountPerNodeInternal(float &outAverage, uint &outAverageDivisor, uint &outMin, uint &outMax) const
{
	if (HasChildren())
	{
		mChild[0]->GetTriangleCountPerNodeInternal(outAverage, outAverageDivisor, outMin, outMax);
		mChild[1]->GetTriangleCountPerNodeInternal(outAverage, outAverageDivisor, outMin, outMax);
	}
	else
	{
		outAverage += GetTriangleCount();
		outAverageDivisor++;
		outMin = min(outMin, GetTriangleCount());
		outMax = max(outMax, GetTriangleCount());
	}
}

// Constructor
AABBTreeBuilder::AABBTreeBuilder(TriangleSplitter &inSplitter, uint inMaxTrianglesPerLeaf) : 
	mTriangleSplitter(inSplitter),
	mMaxTrianglesPerLeaf(inMaxTrianglesPerLeaf) 
{ 
}

// Build the tree
AABBTreeBuilder::Node *AABBTreeBuilder::Build(AABBTreeBuilderStats &outStats)
{
	TriangleSplitter::Range initial = mTriangleSplitter.GetInitialRange();

	ProgressIndicator progress("AABBTreeBuilder", initial.Count());
	Node *root = BuildInternal(initial, progress);

	float avg_triangles_per_leaf;
	uint min_triangles_per_leaf, max_triangles_per_leaf;
	root->GetTriangleCountPerNode(avg_triangles_per_leaf, min_triangles_per_leaf, max_triangles_per_leaf);

	mTriangleSplitter.GetStats(outStats.mSplitterStats);

	outStats.mSAHCost = root->CalculateSAHCost(1.0f, 1.0f);
	outStats.mMinDepth = root->GetMinDepth();
	outStats.mMaxDepth = root->GetMaxDepth();
	outStats.mNodeCount = root->GetNodeCount();
	outStats.mLeafNodeCount = root->GetLeafNodeCount();
	outStats.mMaxTrianglesPerLeaf = mMaxTrianglesPerLeaf;
	outStats.mTreeMinTrianglesPerLeaf = min_triangles_per_leaf;
	outStats.mTreeMaxTrianglesPerLeaf = max_triangles_per_leaf;
	outStats.mTreeAvgTrianglesPerLeaf = avg_triangles_per_leaf;

	return root;
}

// Recursive helper function to build the tree
AABBTreeBuilder::Node *AABBTreeBuilder::BuildInternal(TriangleSplitter::Range &inTriangles, ProgressIndicator &inProgress)
{
	// Check if there are too many triangles left
	if (inTriangles.Count() > mMaxTrianglesPerLeaf)
	{
		// Split triangles in two batches
		uint dimension = (uint)-1;
		float split = 0.0f;
		TriangleSplitter::Range left, right;
		if (!mTriangleSplitter.Split(inTriangles, left, right, dimension, split))
		{
			Trace("AABBTreeBuilder: Doing random split for %d triangles (max per node: %d)!\n", (int)inTriangles.Count(), mMaxTrianglesPerLeaf);
			int half = inTriangles.Count() / 2;
			assert(half > 0);
			left = TriangleSplitter::Range(inTriangles.mBegin, inTriangles.mBegin + half);
			right = TriangleSplitter::Range(inTriangles.mBegin + half, inTriangles.mEnd);
		}

		// Recursively build
		Node *node = new Node();
		node->mChild[0] = BuildInternal(left, inProgress);
		node->mChild[1] = BuildInternal(right, inProgress);
		node->mBounds = node->mChild[0]->mBounds;
		node->mBounds.Encapsulate(node->mChild[1]->mBounds);
		node->mSplitDimension = dimension;
		node->mSplitValue = split;
		return node;
	}

	// Create leaf node
	Node *node = new Node();
	node->mTriangles.reserve(inTriangles.Count());
	for (uint i = inTriangles.mBegin; i < inTriangles.mEnd; ++i)
	{
		const IndexedTriangle &t = mTriangleSplitter.GetTriangle(i);
		const VertexList &v = mTriangleSplitter.GetVertices();
		node->mTriangles.push_back(t);
		node->mBounds.Encapsulate(v, t);
	}

	// Update progress
	inProgress.Update(inTriangles.Count());

	return node;
}
