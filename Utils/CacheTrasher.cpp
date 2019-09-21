#include <pch.h>

#include <Utils/CacheTrasher.h>

int CacheTrasher::sCacheLineSize = 0;

void CacheTrasher::sInit()
{
#if defined(_WIN32)
	// Output cache configuration
	DWORD length = 0;
	BOOL success = GetLogicalProcessorInformation(nullptr, &length);
	assert(!success);

	vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> info;
	info.resize(length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
	success = GetLogicalProcessorInformation(&info[0], &length);
	assert(success);
	if (success)
	{
		sCacheLineSize = INT_MAX;

		for (SYSTEM_LOGICAL_PROCESSOR_INFORMATION &i : info)
			if (i.Relationship == RelationCache)
			{
				const char *type;
				switch (i.Cache.Type)
				{
				case CacheData:
					type = "Data";
					break;
				case CacheInstruction:
					type = "Instruction";
					break;
				case CacheUnified:
					type = "Unified";
					break;
				default:
					assert(false);
					type = "Undefined";
					break;
				}
				Trace("Cache: ProcessorMask=0x%08x, Level=%d, Size=%d, LineSize=%d, Type=%s\n", (uint32)i.ProcessorMask, (int)i.Cache.Level, (int)i.Cache.Size, (int)i.Cache.LineSize, type);
				sCacheLineSize = min<int>(sCacheLineSize, i.Cache.LineSize);
			}
	}
#else
	#error Undefined
#endif
}
