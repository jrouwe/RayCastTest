#include <pch.h> // IWYU pragma: keep

#include <Core/Memory.h>

// Allocate a block of memory aligned to inAlignment bytes of size inSize
void *AlignedAlloc(size_t inSize, size_t inAlignment)
{
#if defined(_WIN32)
	return _aligned_malloc(inSize, inAlignment);
#elif defined(__linux__)
	return aligned_alloc(inAlignment, inSize);
#else
	#error Undefined
#endif
}

// Free memory block allocated with AlignedAlloc
void AlignedFree(void *inBlock)
{
#if defined(_WIN32)
	_aligned_free(inBlock);
#elif defined(__linux__)
	free(inBlock);
#else
	#error Undefined
#endif
}
