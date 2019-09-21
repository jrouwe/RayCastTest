#pragma once

#include <Core/Memory.h>

// STL allocator that takes care that memory is aligned to N bytes
template <typename T, size_t N>
class AlignedAllocator
{
public:
	using value_type = T;

	// Pointer to type
	using pointer = T *;
	using const_pointer = const T *;

	// Reference to type
	// Can be removed in C++20
	using reference = T &;
	using const_reference = const T &;

	using size_type = size_t;
	using difference_type = ptrdiff_t;

	// Constructor
	inline					AlignedAllocator() { }

	// Constructor from other allocator
	template <typename T2>
	inline					AlignedAllocator(const AlignedAllocator<T2, N> &) { }

	// Destructor
	inline					~AlignedAllocator() { }

	// Convert reference to pointer
	// Can be removed in C++20
	inline pointer			adress(reference r)
	{
		return &r;
	}

	// Convert reference to pointer
	// Can be removed in C++20
	inline const_pointer	adress(const_reference r) const
	{
		return &r;
	}

	// Allocate memory
	inline pointer			allocate(size_type n)
	{
		return (pointer)AlignedAlloc(n * sizeof(value_type), N);
	}

	// Free memory
	inline void				deallocate(pointer p, size_type)
	{
		AlignedFree(p);
	}

	// Construct object
	// Can be removed in C++20
	inline void				construct(pointer p, const value_type & value)
	{
		new (p) value_type(value);
	}

	// Destroy object
	// Can be removed in C++20
	inline void				destroy(pointer p)
	{
		p->~value_type();
	}

	// Max allocation size
	// Can be removed in C++20
	inline size_type		max_size() const 
	{
		return size_type(-1) / sizeof(value_type);
	}

	// Allocators are stateless so assumed to be equal
	inline bool				operator == (const AlignedAllocator<T, N>& other) const
	{
		return true;
	}

	inline bool				operator != (const AlignedAllocator<T, N>& other) const
	{
		return false;
	}

	// Converting to allocator for other type
	template <typename T2>
	struct rebind
	{
		using other = AlignedAllocator<T2, N>;
	};
};