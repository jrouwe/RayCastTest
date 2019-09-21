#pragma once

// Hash combiner to use a custom struct in an unordered map or set
// Taken from: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
//
// Usage:
//
// struct SomeHashKey 
// {
//     std::string key1;
//     std::string key2;
//     bool key3;
// };
// 
// MAKE_HASHABLE(SomeHashKey, t.key1, t.key2, t.key3)

inline void hash_combine(std::size_t &ioSeed) 
{ 
}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t &ioSeed, const T &inValue, Rest... inRest) 
{
	std::hash<T> hasher;
    ioSeed ^= hasher(inValue) + 0x9e3779b9 + (ioSeed << 6) + (ioSeed >> 2);
    hash_combine(ioSeed, inRest...);
}

#define MAKE_HASH_STRUCT(type, name, ...)					\
	struct name												\
	{														\
        std::size_t operator()(const type &t) const			\
		{													\
            std::size_t ret = 0;							\
            hash_combine(ret, __VA_ARGS__);					\
            return ret;										\
        }													\
    };

#define MAKE_HASHABLE(type, ...)							\
    namespace std											\
	{														\
        template<>											\
		MAKE_HASH_STRUCT(type, hash<type>, __VA_ARGS__)		\
    }
