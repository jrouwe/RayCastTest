#pragma once

#include <Utils/CacheTrasher.h>
#include <Utils/StatsWriter.h>

class Model;
class Renderer;

// Structure that holds a single ray cast
struct RayCastTestIn
{
	Float3				mOrigin;
	Float3				mDirection;
};

typedef vector<RayCastTestIn> RayCasts;

// Structure that holds a single ray cast test output
struct RayCastTestOut
{
	float				mDistance;
};

typedef vector<RayCastTestOut> RayCastsOut;

class RayCastTest
{
public:
	// Virtual destructor
						RayCastTest() : mModel(nullptr), mRenderer(nullptr) { }
	virtual				~RayCastTest() { }

	// Get statistics relevant for this test
	virtual void		GetStats(StatsRow &ioRow) const = 0;

	// Setup pointers to subsystems
	virtual void		SetSubSystems(Model *inModel, Renderer *inRenderer) { mModel = inModel; mRenderer = inRenderer; }

	// Remove all internal data structures from the cache
	virtual void		TrashCache() { }

	// Initialize the test
	virtual	void		Initialize() { }

	// Cast a number of rays and get the closest collision point
	virtual void		CastRays(const RayCastTestIn *inRayCastsBegin, const RayCastTestIn *inRayCastsEnd, RayCastTestOut *outRayCasts) = 0;

protected:
	Model *				mModel;
	Renderer *			mRenderer;
};
