#pragma once

// Allocate a block of memory aligned to inAlignment bytes of size inSize
void *AlignedAlloc(size_t inSize, size_t inAlignment);

// Free memory block allocated with AlignedAlloc
void AlignedFree(void *inBlock);
