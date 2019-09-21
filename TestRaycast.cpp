#include <pch.h>

#ifdef _WIN32
#include <Renderer/Renderer.h>
#include <Renderer/LineRenderer.h>
#include <RayCastTest/RayCastGPUBruteForce.h>
#include <RayCastTest/RayCastGPUBruteForceBitPack.h>
#include <RayCastTest/RayCastGPUTree.h>
#include <RayCastTest/RayCastGPUAABBList.h>
#include <RayCastTest/RayCastCPUAABBTreeISPC.h>
#include <RayCastTest/RayCastCPUAABBTreeStripISPC.h>
#include <TriangleCodec/TriangleCodecFloat3ISPC.h>
#endif
#include <RayCastTest/RayCastCPUBruteForce.h>
#include <RayCastTest/RayCastCPUAABBList.h>
#include <RayCastTest/RayCastCPUAABBTree1.h>
#include <RayCastTest/RayCastCPUAABBTree2.h>
#include <RayCastTest/RayCastCPUAABBTree3.h>
#include <RayCastTest/RayCastCPUAABBTree4.h>
#include <RayCastTest/RayCastCPUAABBTree5.h>
#include <RayCastTest/RayCastCPUAABBTreePNS.h>
#include <RayCastTest/RayCastCPUAABBTreeSplitAxis.h>
#include <RayCastTest/RayCastCPUAABBTreeCompressed.h>
#include <RayCastTest/RayCastCPUSKDTree.h>
#include <RayCastTest/RayCastCPUQuadTree.h>
#include <RayCastTest/RayCastCPUQuadTreeHalfFloat.h>
#include <RayCastTest/RayCastCPUQuadTreeHalfFloat2.h>
#include <TriangleSplitter/TriangleSplitterBinning.h>
#include <TriangleSplitter/TriangleSplitterMean.h>
#include <TriangleSplitter/TriangleSplitterMorton.h>
#include <TriangleSplitter/TriangleSplitterLongestAxis.h>
#include <TriangleSplitter/TriangleSplitterFixedLeafSize.h>
#include <TriangleCodec/TriangleCodecIndexed.h>
#include <TriangleCodec/TriangleCodecIndexedSOA4.h>
#include <TriangleCodec/TriangleCodecIndexedBitPackSOA4.h>
#include <TriangleCodec/TriangleCodecFloat3Original.h>
#include <TriangleCodec/TriangleCodecFloat3.h>
#include <TriangleCodec/TriangleCodecFloat3SOA4.h>
#include <TriangleCodec/TriangleCodecFloat3SOA4Packed.h>
#include <TriangleCodec/TriangleCodecFloat3SOA8.h>
#include <TriangleCodec/TriangleCodecStrip.h>
#include <TriangleCodec/TriangleCodecBitPack.h>
#include <TriangleCodec/TriangleCodecBitPackSOA4.h>
#include <TriangleCodec/TriangleCodecIndexed8BitPackSOA4.h>
#include <Application/Application.h>
#include <Application/EntryPoint.h>
#include <Utils/CacheTrasher.h>
#include <Utils/StatsWriter.h>
#include <Utils/PerfTimer.h>
#include <Utils/Model.h>
#include <random>

//-----------------------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------------------
#define TEST_FILE "Assets/mothers_heart.model"
//#define TEST_FILE "Assets/bun_zipper.model"
//#define TEST_FILE "Assets/triangle.model"

//#define TEST_CODEC TriangleCodecFloat3Original
//#define TEST_CODEC TriangleCodecFloat3
//#define TEST_CODEC TriangleCodecFloat3SOA4<16>
//#define TEST_CODEC TriangleCodecFloat3SOA4Packed
//#define TEST_CODEC TriangleCodecFloat3SOA8<32>
//#define TEST_CODEC TriangleCodecFloat3ISPC
//#define TEST_CODEC TriangleCodecStripUncompressed
//#define TEST_CODEC TriangleCodecStripCompressed
//#define TEST_CODEC TriangleCodecIndexed<uint16>
//#define TEST_CODEC TriangleCodecIndexedSOA4<uint16>
//#define TEST_CODEC TriangleCodecIndexedBitPackSOA4<uint16>
#define TEST_CODEC TriangleCodecIndexed8BitPackSOA4
//#define TEST_CODEC TriangleCodecBitPack
//#define TEST_CODEC TriangleCodecBitPackSOA4<1, true>

//#define TEST_TYPE RayCastCPUBruteForce<TEST_CODEC>
//#define TEST_TYPE RayCastCPUBruteForce<TriangleCodecIndexed8BitPackSOA4, 16>
//#define TEST_TYPE RayCastGPUBruteForce(RayCastGPUBruteForce::STORE_PER_TRIANGLE, 128, 1)
//#define TEST_TYPE RayCastGPUBruteForceBitPack(128)
//#define TEST_TYPE RayCastCPUAABBList<TEST_CODEC, 1>(ERayCastCPUAABBListVariant::BOUNDS_PLAIN, ERayCastCPUAABBGrouper::GROUPER_MORTON, 64)
//#define TEST_TYPE RayCastCPUAABBList<TEST_CODEC, 16>(ERayCastCPUAABBListVariant::BOUNDS_SOA4, ERayCastCPUAABBGrouper::GROUPER_MORTON, 64)
//#define TEST_TYPE RayCastCPUAABBList<TEST_CODEC, 32>(ERayCastCPUAABBListVariant::BOUNDS_SOA8, ERayCastCPUAABBGrouper::GROUPER_MORTON, 64)
//#define TEST_TYPE RayCastCPUAABBList<TEST_CODEC, 16>(ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4, ERayCastCPUAABBGrouper::GROUPER_MORTON, 64)
//#define TEST_TYPE RayCastGPUAABBList(RayCastGPUAABBList::GROUPER_MORTON)
//#define TEST_TYPE RayCastCPUAABBTree1<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTree2<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTree3<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTree4<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTree5<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTreePNS<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTreeSplitAxis<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTreeISPC(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTreeStripISPC(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUAABBTreeCompressed<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>>(mModel->mTriangleVertices, mAABBTreeRoot, "RayCastGPUAABBTree1.hlsl")
//#define TEST_TYPE RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>>(mModel->mTriangleVertices, mAABBTreeRoot, "RayCastGPUAABBTree2.hlsl")
//#define TEST_TYPE RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>>(mModel->mTriangleVertices, mAABBTreeRoot, "RayCastGPUAABBTree3.hlsl")
//#define TEST_TYPE RayCastGPUTree<AABBTreeToBuffer<TriangleCodecStripUncompressed, NodeCodecAABBTree>>(mModel->mTriangleVertices, mAABBTreeRoot, "RayCastGPUAABBTreeStrip.hlsl")
//#define TEST_TYPE RayCastCPUSKDTree<TEST_CODEC>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecSKDTree>>(mModel->mTriangleVertices, mAABBTreeRoot, "RayCastGPUSKDTree.hlsl"); 
//#define TEST_TYPE RayCastCPUQuadTree<TEST_CODEC, 1>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUQuadTree<TEST_CODEC, 128>(mModel->mTriangleVertices, mAABBTreeRoot)
//#define TEST_TYPE RayCastCPUQuadTreeHalfFloat<TEST_CODEC, 1>(mModel->mTriangleVertices, mAABBTreeRoot)
#define TEST_TYPE RayCastCPUQuadTreeHalfFloat<TEST_CODEC, 16>(mModel->mTriangleVertices, mAABBTreeRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST)
//#define TEST_TYPE RayCastCPUQuadTreeHalfFloat2<TEST_CODEC, 16>(mModel->mTriangleVertices, mAABBTreeRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST)

//#define RUN_ALL_TESTS
#define DRAW_MODEL
//#define DRAW_RAYS
#ifdef RUN_ALL_TESTS
	#define TEST_ITERATIONS_FAST 10
	#define TEST_ITERATIONS_SLOW 3
#else
	#define TEST_ITERATIONS_FAST 0
	#define TEST_ITERATIONS_SLOW 0
#endif
//#define QUICK_TEST
#define RANDOM_RAYS
#define NUM_RAYS_PER_AXIS 32
//#define TEST_SPLITTERS
//#define FLUSH_CACHE_AFTER_EVERY_RAY

//-----------------------------------------------------------------------------
// Class declaration
//-----------------------------------------------------------------------------
class TestRaycast : public Application
{
private:
	// Render modules
	TriangleRenderer::Batch 	mModelBatch;

	// Loaded model
	Model *						mModel;
#ifdef TEST_TYPE
	AABBTreeBuilder::Node *		mAABBTreeRoot;
#endif

	// Raycasts to perform
	RayCasts 					mRayCasts;
	RayCastTest *				mRayCastTest;

	// Stats
	StatsWriter					mStatsWriter;

public:
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
TestRaycast() :
	mModel(nullptr),
#ifdef TEST_TYPE
	mAABBTreeRoot(nullptr),
#endif
	mRayCastTest(nullptr)
{
	// Initialize cache trasher
	CacheTrasher::sInit();
	
	// Read model
	mModel = new Model;
	mModel->ReadFromFile(TEST_FILE);

	// Init model renderer
	mModelBatch = TriangleRenderer::sInstance->CreateTriangleBatch(mModel->mTriangles);

#ifdef TEST_TYPE
	// Create AABB tree
	TriangleSplitterBinning splitter(mModel->mTriangleVertices, mModel->mIndexedTriangles);
	StatsRow stats;

	AABBTreeBuilderStats builder_stats;
	mAABBTreeRoot = AABBTreeBuilder(splitter, 8).Build(builder_stats);
	stats.Set(builder_stats);
#endif
	
	// Make a bit bigger so we'll do some raycasts around the edges
	Vec3 delta = mModel->mBounds.mMax - mModel->mBounds.mMin;
	Vec3 min = mModel->mBounds.mMin - 0.1f * delta;
	Vec3 max = mModel->mBounds.mMax + 0.1f * delta;
	delta = max - min;

	// Calculate rays to cast
	mRayCasts.reserve(NUM_RAYS_PER_AXIS * NUM_RAYS_PER_AXIS);

#ifdef RANDOM_RAYS
	default_random_engine random(0x1ee7c0de);
	float radius = 0.5f * delta.Length();
	Vec3 mid = 0.5f * (min + max);
	for (int i = 0; i < NUM_RAYS_PER_AXIS * NUM_RAYS_PER_AXIS; ++i)
	{
		Vec3 origin = radius * Vec3::sRandom(random);
		Vec3 destination = 0.25f * radius * Vec3::sRandom(random);
		Vec3 direction = (destination - origin).Normalized();
		RayCastTestIn ray;
		(mid + origin).StoreFloat3(&ray.mOrigin);
		direction.StoreFloat3(&ray.mDirection);
		mRayCasts.push_back(ray);
	}
#else
	float xs = min.GetX(), dx = delta.GetX() / NUM_RAYS_PER_AXIS;
	float ys = min.GetY(), dy = delta.GetY() / NUM_RAYS_PER_AXIS;
	for (int i = 0; i < NUM_RAYS_PER_AXIS; ++i)
		for (int j = 0; j < NUM_RAYS_PER_AXIS; ++j)
		{
			RayCastTestIn ray;
			ray.mOrigin = Float3(xs + dx * i, ys + dy * j, 2.0f);
			ray.mDirection = Float3(0.0f, 0.0f, -1.0f);
			mRayCasts.push_back(ray);
		}
#endif

#if TEST_ITERATIONS_SLOW > 0 || TEST_ITERATIONS_FAST > 0
	// Validate all algorithms
	RunTests();
#endif

#ifdef TEST_TYPE
	// Initialize test
	mRayCastTest = new TEST_TYPE;
	mRayCastTest->SetSubSystems(mModel, mRenderer);
	mRayCastTest->Initialize();
#endif
}

//-----------------------------------------------------------------------------
// This method is called after the application terminates.
//-----------------------------------------------------------------------------
virtual ~TestRaycast() override
{
	delete mRayCastTest;
#ifdef TEST_TYPE
	delete mAABBTreeRoot;
#endif
	delete mModel;
	mModelBatch = nullptr;
}

#ifdef TEST_TYPE

//-----------------------------------------------------------------------------
// Render the frame.
//-----------------------------------------------------------------------------
virtual bool RenderFrame(float inDeltaTime) override
{
	// Allocate space for raycast output
	RayCastsOut out;
	out.resize(mRayCasts.size());

	static PerfTimer timer("CastRays");

#ifdef FLUSH_CACHE_AFTER_EVERY_RAY
	for (size_t j = 0; j < mRayCasts.size(); ++j)
	{
		// Trash the cache
		CacheTrasher::sTrash(&mRayCasts[j]);
		CacheTrasher::sTrash(&out[j]);
		mRayCastTest->TrashCache();

		// Do raycasts
		timer.Start();
		mRayCastTest->CastRays(&mRayCasts[j], &mRayCasts[j] + 1, &out[j]);
		timer.Stop(1);
	}
#else
	// Trash the cache
	CacheTrasher::sTrash(mRayCasts);
	CacheTrasher::sTrash(out);
	mRayCastTest->TrashCache();

	timer.Start();
	mRayCastTest->CastRays(&mRayCasts[0], &mRayCasts[0] + mRayCasts.size(), &out[0]);
	timer.Stop((int)mRayCasts.size());
#endif

	timer.Output();

	float model_size = 2.0f * mModel->mBounds.GetExtent().Length();
	float marker_size = 0.002f * model_size;

	// Draw raycast results
	for (uint r = 0; r < mRayCasts.size(); ++r)
		if (out[r].mDistance < FLT_MAX)
		{
			Vec3 hit_pos = Vec3(mRayCasts[r].mOrigin) + Vec3(mRayCasts[r].mDirection) * out[r].mDistance;
			LineRenderer::sInstance->DrawMarker(hit_pos, Color::sGreen, marker_size);
		}

#ifdef DRAW_RAYS
	// Draw rays
	for (uint r = 0; r < mRayCasts.size(); ++r)
	{
		Vec3 origin(mRayCasts[r].mOrigin);
		Vec3 direction(mRayCasts[r].mDirection);
		LineRenderer::sInstance->DrawMarker(origin, Color::sRed, marker_size);
		if (out[r].mDistance < FLT_MAX)
			LineRenderer::sInstance->DrawLine(origin, origin + out[r].mDistance * direction, Color::sGreen);
		else
			LineRenderer::sInstance->DrawLine(origin, origin + model_size * direction, Color::sWhite);
	}
#endif

	// Draw model
#ifdef DRAW_MODEL
	TriangleRenderer::sInstance->DrawTriangleBatch(Mat44::sIdentity(), Color(90, 90, 90, 255), mModelBatch);
#endif

	return true;
}

#endif

#if TEST_ITERATIONS_SLOW > 0 || TEST_ITERATIONS_FAST > 0

//-----------------------------------------------------------------------------
// Validate output of various algorithms
//-----------------------------------------------------------------------------
void RunTests()
{
	StatsRow row;
	row.Set(StatsColumn::ModelName, TEST_FILE);

	RayCastsOut reference_data;
	reference_data.resize(mRayCasts.size());

	// Calculate reference output
	{
		RayCastCPUBruteForce<TriangleCodecFloat3Original> reference_test;
		reference_test.SetSubSystems(mModel, mRenderer);
		reference_test.Initialize();
		reference_test.CastRays(&mRayCasts[0], &mRayCasts[0] + mRayCasts.size(), &reference_data[0]);
	}

	// Trace how many rays hit the target
	uint num_hits = 0;
	for (RayCastTestOut &o : reference_data)
		if (o.mDistance < FLT_MAX)
			++num_hits;
	Trace("Hit percentage: %.1f%%\n", 100.0f * num_hits / reference_data.size());

#ifndef QUICK_TEST
	// CPU brute force
	{
		RayCastCPUBruteForce<TriangleCodecFloat3Original> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecFloat3> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}	
	{
		RayCastCPUBruteForce<TriangleCodecFloat3SOA4<1>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecFloat3SOA4<16>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecFloat3SOA8<1>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecFloat3SOA8<32>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
#ifdef _WIN32
	{
		RayCastCPUBruteForce<TriangleCodecFloat3ISPC> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
#endif
	if (mModel->mTriangleVertices.size() <= 0xffff)
	{
		RayCastCPUBruteForce<TriangleCodecIndexed<uint16>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecIndexed<uint32>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	if (mModel->mTriangleVertices.size() <= 0xffff)
	{
		RayCastCPUBruteForce<TriangleCodecIndexedSOA4<uint16>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecIndexedSOA4<uint32>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	if (mModel->mTriangleVertices.size() <= 0xffff)
	{
		RayCastCPUBruteForce<TriangleCodecIndexedBitPackSOA4<uint16>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}	
	{
		RayCastCPUBruteForce<TriangleCodecIndexedBitPackSOA4<uint32>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecStripUncompressed> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecStripCompressed> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecBitPack> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecBitPackSOA4<1, false>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecBitPackSOA4<16, false>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecBitPackSOA4<1, true>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecBitPackSOA4<16, true>> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastCPUBruteForce<TriangleCodecIndexed8BitPackSOA4, 16> test;
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
#endif

	// CPU AABB list
#ifndef QUICK_TEST
	for (int tri_per_batch = 4; tri_per_batch <= 64; tri_per_batch <<= 1)
#else
	uint tri_per_batch = 64;
#endif
	{
		{
			RayCastCPUAABBList<TriangleCodecFloat3, 1> test(ERayCastCPUAABBListVariant::BOUNDS_PLAIN, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}
		{
			RayCastCPUAABBList<TriangleCodecFloat3SOA4<1>, 1> test(ERayCastCPUAABBListVariant::BOUNDS_SOA4, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}
		{
			RayCastCPUAABBList<TriangleCodecFloat3SOA4<16>, 16> test(ERayCastCPUAABBListVariant::BOUNDS_SOA4, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}
		{
			RayCastCPUAABBList<TriangleCodecFloat3SOA8<1>, 1> test(ERayCastCPUAABBListVariant::BOUNDS_SOA8, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}
		{
			RayCastCPUAABBList<TriangleCodecFloat3SOA8<32>, 32> test(ERayCastCPUAABBListVariant::BOUNDS_SOA8, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}
		{
			RayCastCPUAABBList<TriangleCodecFloat3SOA4<1>, 1> test(ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}
		{
			RayCastCPUAABBList<TriangleCodecFloat3SOA4<16>, 16> test(ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4, ERayCastCPUAABBGrouper::GROUPER_MORTON, tri_per_batch);
			RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
		}

		// Above 100,000 triangles the closest centroid grouper (O(N^2)) becomes too slow
		if (mModel->GetTriangleCount() < 100000)
		{
			{
				RayCastCPUAABBList<TriangleCodecFloat3, 1> test(ERayCastCPUAABBListVariant::BOUNDS_PLAIN, ERayCastCPUAABBGrouper::GROUPER_CLOSEST_CENTROID, tri_per_batch);
				RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
			}
			{
				RayCastCPUAABBList<TriangleCodecFloat3SOA4<16>, 16> test(ERayCastCPUAABBListVariant::BOUNDS_SOA4, ERayCastCPUAABBGrouper::GROUPER_CLOSEST_CENTROID, tri_per_batch);
				RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
			}
			{
				RayCastCPUAABBList<TriangleCodecFloat3SOA8<32>, 32> test(ERayCastCPUAABBListVariant::BOUNDS_SOA8, ERayCastCPUAABBGrouper::GROUPER_CLOSEST_CENTROID, tri_per_batch);
				RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
			}
			{
				RayCastCPUAABBList<TriangleCodecFloat3SOA4<16>, 16> test(ERayCastCPUAABBListVariant::BOUNDS_HALFFLOAT_SOA4, ERayCastCPUAABBGrouper::GROUPER_CLOSEST_CENTROID, tri_per_batch);
				RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
			}
		}
	}

	// AABBTree
#ifndef QUICK_TEST
	for (uint triangles_per_leaf = 4; triangles_per_leaf <= 16; triangles_per_leaf <<= 1)
#else
	uint triangles_per_leaf = 8;
#endif
	{
		{
			TriangleSplitterBinning splitter(mModel->mTriangleVertices, mModel->mIndexedTriangles);
			RunAABBTreeTest(splitter, triangles_per_leaf, reference_data, row);
		}
#ifdef TEST_SPLITTERS
		{
			TriangleSplitterFixedLeafSize splitter(mModel->mTriangleVertices, mModel->mIndexedTriangles, triangles_per_leaf);
			RunAABBTreeTest(splitter, triangles_per_leaf, reference_data, row);
		}
		{
			TriangleSplitterMean splitter(mModel->mTriangleVertices, mModel->mIndexedTriangles);
			RunAABBTreeTest(splitter, triangles_per_leaf, reference_data, row);
		}
		{
			TriangleSplitterLongestAxis splitter(mModel->mTriangleVertices, mModel->mIndexedTriangles);
			RunAABBTreeTest(splitter, triangles_per_leaf, reference_data, row);
		}
		{
			TriangleSplitterMorton splitter(mModel->mTriangleVertices, mModel->mIndexedTriangles);
			RunAABBTreeTest(splitter, triangles_per_leaf, reference_data, row);
		}
#endif
	}

#ifdef _WIN32
	// GPU brute force
	RunGPUBruteForceTest(RayCastGPUBruteForce::STORE_PER_TRIANGLE, reference_data, row);
	RunGPUBruteForceTest(RayCastGPUBruteForce::STORE_PER_VECTOR, reference_data, row);
	RunGPUBruteForceTest(RayCastGPUBruteForce::STORE_PER_COMPONENT, reference_data, row);

	// GPU Bit Pack
	{
		RayCastGPUBruteForceBitPack test(128);
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}

	// GPU AABB list
	{
		RayCastGPUAABBList test(RayCastGPUAABBList::GROUPER_MORTON);
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}

	// Above 100,000 triangles the closest centroid grouper (O(N^2)) becomes too slow
	if (mModel->GetTriangleCount() < 100000)
	{
		RayCastGPUAABBList test(RayCastGPUAABBList::GROUPER_CLOSEST_CENTROID);
		RunTest(test, reference_data, row, TEST_ITERATIONS_SLOW);
	}
#endif
}

#ifdef _WIN32

void RunGPUBruteForceTest(RayCastGPUBruteForce::EVariant inVariant, const RayCastsOut &inReference, const StatsRow &inRow)
{
#ifndef QUICK_TEST
	// Different thread group sizes
	{
		RayCastGPUBruteForce test(inVariant, 32, 1);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastGPUBruteForce test(inVariant, 64, 1);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastGPUBruteForce test(inVariant, 128, 1);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastGPUBruteForce test(inVariant, 256, 1);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}

	// Different amount of triangles per thread
	{
		RayCastGPUBruteForce test(inVariant, 128, 1);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastGPUBruteForce test(inVariant, 128, 2);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
#endif
	{
		RayCastGPUBruteForce test(inVariant, 128, 4);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
#ifndef QUICK_TEST
	{
		RayCastGPUBruteForce test(inVariant, 128, -4);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastGPUBruteForce test(inVariant, 128, 8);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
	{
		RayCastGPUBruteForce test(inVariant, 128, 16);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_SLOW);
	}
#endif
}

#endif

template <class TriangleCodec>
void RunAABBTreeTestWithCodec(TriangleSplitter &inSplitter, const AABBTreeBuilder::Node *inRoot, const RayCastsOut &inReference, const StatsRow &inRow)
{
	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Variant 1: Tests bounds at each level of the tree and recurses to left and right child if it intersects
		RayCastCPUAABBTree1<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}
	
	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Variant 2: Tests bounds at each level of the tree, then checks left and right subtrees to decide if / which child to visit first
		RayCastCPUAABBTree2<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Variant 3: Never check root, only check left and right subtrees to decide if / which child to visit first
		RayCastCPUAABBTree3<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Variant 4: Never check root, only check left and right subtrees to decide if / which child to visit first. Only check current nodes bounds if testing against triangles.
		RayCastCPUAABBTree4<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Variant 5: Test bounding box for each entry from the stack, recurse to leaf node without retesting bounding box
		RayCastCPUAABBTree5<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Compressed AABB Tree
		RayCastCPUAABBTreeCompressed<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// Test with Precomputed Node Sorting
		RayCastCPUAABBTreePNS<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);	
	}

	if (!TriangleCodec::ChangesOffsetOnPack && inSplitter.CalculatesSplitDimension())
	{
		// Includes split axis test
		RayCastCPUAABBTreeSplitAxis<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	if (!TriangleCodec::ChangesOffsetOnPack)
	{
		// SKDTree test on CPU
		RayCastCPUSKDTree<TriangleCodec> test(inSplitter.GetVertices(), inRoot);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree, not aligned
		RayCastCPUQuadTree<TriangleCodec, 1> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree, nodes aligned to 16 bytes
		RayCastCPUQuadTree<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree, nodes aligned to 16 bytes
		RayCastCPUQuadTree<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_BREADTH_FIRST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree, nodes aligned to 16 bytes
		RayCastCPUQuadTree<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree, nodes aligned to 16 bytes
		RayCastCPUQuadTree<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_BREADTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree with half floats, not aligned
		RayCastCPUQuadTreeHalfFloat<TriangleCodec, 1> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree with half floats, nodes aligned to 16 bytes
		RayCastCPUQuadTreeHalfFloat<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}
	
	{
		// Quad tree with half floats, nodes aligned to 16 bytes
		RayCastCPUQuadTreeHalfFloat<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_BREADTH_FIRST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree with half floats, nodes aligned to 16 bytes
		RayCastCPUQuadTreeHalfFloat<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree with half floats, nodes aligned to 16 bytes
		// different data loading strategy
		RayCastCPUQuadTreeHalfFloat2<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_DEPTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}

	{
		// Quad tree with half floats, nodes aligned to 16 bytes
		RayCastCPUQuadTreeHalfFloat<TriangleCodec, 16> test(inSplitter.GetVertices(), inRoot, EAABBTreeToBufferConvertMode::CONVERT_BREADTH_FIRST_TRIANGLES_LAST);
		RunTest(test, inReference, inRow, TEST_ITERATIONS_FAST);
	}	
}

void RunAABBTreeTest(TriangleSplitter &inSplitter, int inMaxTrianglesPerLeaf, const RayCastsOut &inReference, const StatsRow &inRow)
{
	// Build tree
	StatsRow row = inRow;
	AABBTreeBuilder builder(inSplitter, inMaxTrianglesPerLeaf);
	AABBTreeBuilderStats builder_stats;
	AABBTreeBuilder::Node *root = builder.Build(builder_stats);
	row.Set(builder_stats);

	// Run tests that can handle multiple codecs
	RunAABBTreeTestWithCodec<TriangleCodecFloat3Original>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecFloat3>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecFloat3SOA4<1>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecFloat3SOA4<16>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecFloat3SOA4Packed>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecFloat3SOA8<1>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecFloat3SOA8<32>>(inSplitter, root, inReference, row);
#ifdef _WIN32
	RunAABBTreeTestWithCodec<TriangleCodecFloat3ISPC>(inSplitter, root, inReference, row);
#endif
	RunAABBTreeTestWithCodec<TriangleCodecStripUncompressed>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecStripCompressed>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecIndexed8BitPackSOA4>(inSplitter, root, inReference, row);
	if (mModel->mTriangleVertices.size() <= 0xffff)
	{
		RunAABBTreeTestWithCodec<TriangleCodecIndexed<uint16>>(inSplitter, root, inReference, row);
		RunAABBTreeTestWithCodec<TriangleCodecIndexedSOA4<uint16>>(inSplitter, root, inReference, row);
		RunAABBTreeTestWithCodec<TriangleCodecIndexedBitPackSOA4<uint16>>(inSplitter, root, inReference, row);
	}
	RunAABBTreeTestWithCodec<TriangleCodecIndexed<uint32>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecIndexedSOA4<uint32>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecIndexedBitPackSOA4<uint32>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecBitPack>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecBitPackSOA4<1, false>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecBitPackSOA4<16, false>>(inSplitter, root, inReference, row);
	RunAABBTreeTestWithCodec<TriangleCodecBitPackSOA4<16, true>>(inSplitter, root, inReference, row);

#ifdef _WIN32
	{
		//// Entire tree traversal is done using ISPC
		RayCastCPUAABBTreeISPC test(mModel->mTriangleVertices, root);
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}

	{
		//// Entire tree traversal is done using ISPC, stripped version
		RayCastCPUAABBTreeStripISPC test(mModel->mTriangleVertices, root);
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}

	{
		// Variant 1: Tests bounds at each level of the tree and recurses to left and right child if it intersects
		RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>> test(mModel->mTriangleVertices, root, "RayCastGPUAABBTree1.hlsl");
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}
	
	{
		// Variant 2: Tests bounds at each level of the tree, then checks left and right subtrees to decide if / which child to visit first
		RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>> test(mModel->mTriangleVertices, root, "RayCastGPUAABBTree2.hlsl");
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}
	
	{
		// Variant 3: Never check root, only check left and right subtrees to decide if / which child to visit first
		RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>> test(mModel->mTriangleVertices, root, "RayCastGPUAABBTree3.hlsl");
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}

	{
		// Variant 4: Never check root, only check left and right subtrees to decide if / which child to visit first. Only check current nodes bounds if testing against triangles.
		RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecAABBTree>> test(mModel->mTriangleVertices, root, "RayCastGPUAABBTree4.hlsl");
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}

	{
		// Triangles are stripped
		RayCastGPUTree<AABBTreeToBuffer<TriangleCodecStripUncompressed, NodeCodecAABBTree>> test(mModel->mTriangleVertices, root, "RayCastGPUAABBTreeStrip.hlsl");
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}

	{
		// SKDTree test on GPU
		RayCastGPUTree<AABBTreeToBuffer<TriangleCodecFloat3, NodeCodecSKDTree>> test(mModel->mTriangleVertices, root, "RayCastGPUSKDTree.hlsl");
		RunTest(test, inReference, row, TEST_ITERATIONS_FAST);
	}
#endif

	delete root;
}

//-----------------------------------------------------------------------------
// Validate output of single algorithm
//-----------------------------------------------------------------------------
void RunTest(RayCastTest &inTest, const RayCastsOut &inReference, const StatsRow &inRow, int inTestIterations)
{
	if (inTestIterations <= 0)
		return;

	StatsRow row = inRow;

	try
	{
		// Initialize
		inTest.SetSubSystems(mModel, mRenderer);
		inTest.Initialize();

		// Fetch stats (they're available after initialization)
		inTest.GetStats(row);

		// Store description
		string description = row.Get(StatsColumn::TestName);
		Trace("Starting '%s'\n", description.c_str());

		// Create timer to measure performance
		PerfTimer timer(description.c_str());

		// Allow shaders to be compiled, buffers to be uploaded etc.
		RayCastsOut dummy;
		dummy.resize(mRayCasts.size());
		inTest.CastRays(&mRayCasts[0], &mRayCasts[0] + mRayCasts.size(), &dummy[0]);

		// Get max size of model
		const AABox &bounds = mModel->mBounds;
		Vec3 bounds_size = 2.0f * bounds.GetExtent();
		float max_size = bounds_size.ReduceMax();

		// Repeat tests a number of times
		float max_error = 0.0f;
		for (int i = 0; i < inTestIterations; ++i)
		{
			// Prepare output
			RayCastsOut out;
			out.resize(mRayCasts.size());

	#ifdef FLUSH_CACHE_AFTER_EVERY_RAY
			for (size_t j = 0; j < mRayCasts.size(); ++j)
			{
				// Trash the cache
				CacheTrasher::sTrash(&mRayCasts[j]);
				CacheTrasher::sTrash(&out[j]);
				inTest.TrashCache();

				// Do raycasts
				timer.Start();
				inTest.CastRays(&mRayCasts[j], &mRayCasts[j] + 1, &out[j]);
				timer.Stop(1);
			}
	#else
			// Trash the cache
			CacheTrasher::sTrash(mRayCasts);
			CacheTrasher::sTrash(out);
			inTest.TrashCache();

			// Do raycasts
			timer.Start();
			inTest.CastRays(&mRayCasts[0], &mRayCasts[0] + mRayCasts.size(), &out[0]);
			timer.Stop((uint)mRayCasts.size());
	#endif

			// Validate that there is no difference
			assert(out.size() == mRayCasts.size());
			for (uint j = 0; j < out.size(); ++j)
			{
				float diff = abs(out[j].mDistance - inReference[j].mDistance);
				max_error = max(max_error, diff);
				if (diff / max_size > 1e-5f)
					Trace("%s: Mismatch for raycast %d, result: %g should be: %g, diff: %g\n", description.c_str(), j, out[j].mDistance, inReference[j].mDistance, diff);
			}
		}

		// Output timings
		timer.Output();

		// Collect stats
		PerfTimer::Stats stats;
		timer.GetStats(stats);
		row.Set(StatsColumn::TimeAvg, stats.mTimeAvgUs);
		row.Set(StatsColumn::TimeMin, stats.mTimeMinUs);
		row.Set(StatsColumn::TimeMax, stats.mTimeMaxUs);
		row.Set(StatsColumn::NumSamples, stats.mNumSamples);

		// Add stats
		row.Set(StatsColumn::MaxError, max_error / max_size);
	}
	catch (string e)
	{
		// If no stats have been collected yet, fetch as much as possible
		if (row.Get(StatsColumn::TestName).empty())
			inTest.GetStats(row);

		// Store the error
		row.Set(StatsColumn::MaxError, e);
	}

	mStatsWriter.Add(row);
}

#endif

//-----------------------------------------------------------------------------
// Override to specify the initial camera state (world space)
//-----------------------------------------------------------------------------
virtual void GetInitialCamera(CameraState &ioState) const override
{
	// Determine initial camera position
	ioState.mPos = Vec3(0.0f, max(0.2f, mModel->mBounds.mMax.GetY()), max(0.2f, 2.0f * mModel->mBounds.mMax.GetZ()));
	ioState.mForward = (mModel->mBounds.GetCenter() - ioState.mPos).Normalized();
	ioState.mFarPlane = max(100.0f, (ioState.mPos - mModel->mBounds.GetCenter()).Length() * 2.0f);
}
};

ENTRY_POINT(TestRaycast)
