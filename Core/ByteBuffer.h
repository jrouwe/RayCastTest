#pragma once

#include <Core/AlignedAllocator.h>

// Simple byte buffer, aligned to a cache line
class ByteBuffer : public vector<uint8, AlignedAllocator<uint8, CACHE_LINE_SIZE>>
{
public:
	// Align the size to a multiple of <inSize>, returns the length after alignment
	size_t			Align(size_t inSize)
	{
		// Assert power of 2
		assert(IsPowerOf2(inSize));

		// Calculate new size and resize buffer
		size_t s = AlignUp(size(), inSize);
		resize(s);

		return s;
	}

	// Allocate block of data of inSize elements and return the pointer
	template <class Type>
	Type *			Allocate(size_t inSize = 1)
	{
		// Reserve space
		size_t s = size();
		resize(s + inSize * sizeof(Type));

		// Get data pointer
		Type *data = reinterpret_cast<Type *>(&at(s));

		// Construct elements
		for (Type *d = data, *d_end = data + inSize; d < d_end; ++d)
			new (d) Type;

		// Return pointer
		return data;
	}

	// Append data in vector to the buffer
	template <class Type>
	void			AppendVector(const vector<Type> &inData)
	{
		size_t size = inData.size() * sizeof(Type);
		uint8 *data = Allocate<uint8>(size);
		memcpy(data, &inData[0], size);
	}

	// Get object at inPosition
	template <class Type>
	const Type *	Get(size_t inPosition) const
	{
		return reinterpret_cast<const Type *>(&at(inPosition));
	}

	// Get object at inPosition
	template <class Type>
	Type *			Get(size_t inPosition)
	{
		return reinterpret_cast<Type *>(&at(inPosition));
	}
};
