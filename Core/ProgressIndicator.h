#pragma once

#include <Core/TickCounter.h>

// Simple class to show progress
class ProgressIndicator
{
public:
	// Constructor
						ProgressIndicator(const char *inName, uint inTotalCount) : mName(inName), mCurrentCount(0), mTotalCount(inTotalCount)
	{ 
		// Get current time
		mLastReportTime = GetProcessorTickCount();

		// Report every 5 seconds
		mReportDelta = 5 * GetProcessorTicksPerSecond();
	}

	// Update progress
	void				Update(uint inDeltaCount)
	{
		mCurrentCount += inDeltaCount;

		uint64 current_time = GetProcessorTickCount();

		if (mCurrentCount >= mTotalCount)
		{
			Trace("%s: Done.\n", mName);
		}
		else if (current_time - mLastReportTime > mReportDelta)
		{
			Trace("%s: Progress - %.1f%%\n", mName, 100.0f * mCurrentCount / mTotalCount);
			mLastReportTime = current_time;
		}
	}
	
private:
	const char *		mName;
	uint				mCurrentCount;
	uint				mTotalCount;
	uint64				mLastReportTime;
	uint64				mReportDelta;
};
