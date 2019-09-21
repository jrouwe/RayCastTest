#include <pch.h> // IWYU pragma: keep

#include <Core/TickCounter.h>

static uint64 sProcessorTicksPerSecond = []() {
#if defined(_WIN32)
	// Open the key where the processor speed is stored
	HKEY hkey;
	RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, 1, &hkey);

	// Query the speed in MHz
	uint mhz = 0;
	DWORD mhz_size = sizeof(uint);
	RegQueryValueExA(hkey, "~MHz", nullptr, nullptr, (LPBYTE)&mhz, &mhz_size);

	// Close key
	RegCloseKey(hkey);

	// Initialize amount of cycles per second
	return (uint64)mhz * 1000000LL;
#elif defined(__linux__)
	// Open /proc/cpuinfo
    ifstream ifs("/proc/cpuinfo");
    if (ifs.is_open())
	{
		// Read all lines
		while (ifs.good())
		{
			// Get next line
			string line;
			getline(ifs, line);
		
			// Check if line starts with 'cpu MHz'
			if (strncmp(line.c_str(), "cpu MHz", 7) == 0)
			{
				// Find ':'
				string::size_type pos = line.find(':', 7);			
				if (pos != string::npos)
				{		
					// Convert to number
					string freq = line.substr(pos + 1);				
					return uint64(stoi(freq)) * 1000000L;
				}
			}
		}
	}

	assert(false);
    return uint64(0);
#else
	#error Undefined
#endif
}();

// Get the amount of ticks per second
uint64 GetProcessorTicksPerSecond()
{
	return sProcessorTicksPerSecond;
}
