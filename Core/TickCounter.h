#pragma once

#ifdef _WIN32
	#include <intrin.h> // for __rdtsc
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Functionality to get the processors cycle counter 
//////////////////////////////////////////////////////////////////////////////////////////

// Get the high frequency tick count
f_inline uint64 GetProcessorTickCount()
{
#if defined(_WIN32)
	return __rdtsc();
#elif defined(__linux__)
	return __rdtsc();
#else
	#error Undefined
#endif
}

// Get the amount of ticks per second
uint64 GetProcessorTicksPerSecond();
