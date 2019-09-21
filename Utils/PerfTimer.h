#pragma once

// Very simple class to measure the time a piece of code takes
class PerfTimer
{
public:
	// Constructor
						PerfTimer(const char *inName) : 
		mName(inName), 
		mStartTime(0), 
		mMinTime(FLT_MAX), 
		mMaxTime(0), 
		mTotalTime(0), 
		mSamples(0) 
	{ 
	}

	// Start the timer
	void				Start()
	{
		assert(mStartTime == 0);

		// Store start time
		mStartTime = GetProcessorTickCount();
	}

	// Stop the timer
	void				Stop(int inSamples)
	{
		// Store stop time
		uint64 stop_time = GetProcessorTickCount();

		// Calculate delta
		assert(mStartTime != 0);
		double time = double(stop_time - mStartTime);
		mStartTime = 0;

		// Accumulate total
		mTotalTime += time;

		// Store min / max
		time /= inSamples;
		if (time < mMinTime)
			mMinTime = time;
		if (time > mMaxTime)
			mMaxTime = time;
		
		// Increase sample count
		mSamples += inSamples;
	}

	// Output timing results to TTY
	void				Output()
	{
		double us_per_ticks = 1.0e6 / GetProcessorTicksPerSecond();

		Trace("%s: avg = %g, min = %g us\n", mName, us_per_ticks * mTotalTime / mSamples, us_per_ticks * mMinTime);
	}

	struct Stats
	{
		float			mTimeAvgUs = 0;
		float			mTimeMinUs = 0;
		float			mTimeMaxUs = 0;
		int				mNumSamples = 0;
	};

	// Get statistics
	void				GetStats(Stats &outStats)
	{
		double us_per_ticks = 1.0e6 / GetProcessorTicksPerSecond();

		outStats.mTimeAvgUs = float(us_per_ticks * mTotalTime / mSamples);
		outStats.mTimeMinUs = float(us_per_ticks * mMinTime);
		outStats.mTimeMaxUs = float(us_per_ticks * mMaxTime);
		outStats.mNumSamples = mSamples;
	}

private:
	const char *		mName;
	uint64				mStartTime;
	double				mMinTime;
	double				mMaxTime;
	double				mTotalTime;
	int					mSamples;
};