#pragma once

class CacheTrasher
{
public:
	// Initialize
	static void					sInit();

	// Remove memory starting from inStart with size inSize from all CPU caches
	static void					sTrash(const void *inStart, size_t inSize)
	{
		const uint8 *cur = reinterpret_cast<const uint8 *>(inStart);
		const uint8 *end = cur + inSize;
		while (cur < end)
		{
			_mm_clflush(cur);
			cur += sCacheLineSize;
		}
	}

	// Remove memory associated with inVector from all CPU caches
	template <class T, class A>
	static void					sTrash(const vector<T, A> &inVector)
	{
		if (!inVector.empty())
			sTrash(&inVector[0], inVector.size() * sizeof(T));
	}

	// Remove memory associated with inT from all CPU caches
	template <class T>
	static void					sTrash(const T *inT)
	{
		sTrash(inT, sizeof(T));
	}
		
private:
	static int					sCacheLineSize;
};