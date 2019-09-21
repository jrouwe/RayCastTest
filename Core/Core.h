#pragma once

#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wc++98-compat"
	#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
	#pragma clang diagnostic ignored "-Wfloat-equal"
	#pragma clang diagnostic ignored "-Wnewline-eof"
	#pragma clang diagnostic ignored "-Wsign-conversion"
	#pragma clang diagnostic ignored "-Wold-style-cast"
	#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
	#pragma clang diagnostic ignored "-Wnested-anon-types"
	#pragma clang diagnostic ignored "-Wglobal-constructors"
	#pragma clang diagnostic ignored "-Wexit-time-destructors"
	#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
	#pragma clang diagnostic ignored "-Wlanguage-extension-token"
	#pragma clang diagnostic ignored "-Wunused-parameter"
	#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#pragma clang diagnostic ignored "-Wswitch-enum"
	#pragma clang diagnostic ignored "-Wcovered-switch-default"
	#pragma clang diagnostic ignored "-Wcast-align"
	#pragma clang diagnostic ignored "-Wformat-non-iso"
	#pragma clang diagnostic ignored "-Wdouble-promotion"
	#pragma clang diagnostic ignored "-Winvalid-offsetof"
	#pragma clang diagnostic ignored "-Wnonportable-include-path"
	#pragma clang diagnostic ignored "-Wheader-hygiene"
#elif defined(_MSC_VER)
	#pragma warning (disable : 4514) // 'X' : unreferenced inline function has been removed
	#pragma warning (disable : 4710) // 'X' : function not inlined
	#pragma warning (disable : 4711) // function 'X' selected for automatic inline expansion
	#pragma warning (disable : 4820) // 'X': 'Y' bytes padding added after data member 'Z'
	#pragma warning (disable : 4100) // 'X' : unreferenced formal parameter
	#pragma warning (disable : 4626) // 'X' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
	#pragma warning (disable : 5027) // 'X' : move assignment operator was implicitly defined as deleted because a base class move assignment operator is inaccessible or deleted
	#pragma warning (disable : 4365) // 'argument' : conversion from 'X' to 'Y', signed / unsigned mismatch
	#pragma warning (disable : 4324) // 'X' : structure was padded due to alignment specifier
	#pragma warning (disable : 4127) // conditional expression is constant
	#pragma warning (disable : 4625) // 'X' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
	#pragma warning (disable : 5026) // 'X': move constructor was implicitly defined as deleted because a base class move constructor is inaccessible or deleted
	#pragma warning (disable : 4464) // relative include path contains '..'
	#pragma warning (disable : 4061) // enumerator 'X' in switch of enum 'Y' is not explicitly handled by a case label
	#pragma warning (disable : 4987) // nonstandard extension used : 'throw (...)'
	#pragma warning (disable : 4623) // 'X' : default constructor was implicitly defined as deleted
	#pragma warning (disable : 4571) // Informational: catch (...) semantics changed since Visual C++ 7.1; structured exceptions(SEH) are no longer caught
	#pragma warning (disable : 4774) // 'X' : format string expected in argument N is not a string literal
	#pragma warning (disable : 4201) // nonstandard extension used: nameless struct/union
	#pragma warning (disable : 4371) // 'X': layout of class may have changed from a previous version of the compiler due to better packing of member 'Y'
	#pragma warning (disable : 5039) // 'X': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#pragma warning (disable : 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#endif

// OS-specific includes
#if defined(_WIN32)
	#pragma warning (push, 0)
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#undef min // We'd like to use std::min and max instead of the ones defined in windows.h
	#undef max
	#include <d3d11.h>
	#include <wrl.h> // for ComPtr
	#pragma warning (pop)
	using Microsoft::WRL::ComPtr;

	#define ALIGN(n)			__declspec(align(n))

	#define BREAKPOINT			__debugbreak()
#elif defined(__linux__)
	#include <popcntintrin.h>	// IWYU pragma: export
	#include <float.h>			// IWYU pragma: export
	#include <limits.h>			// IWYU pragma: export
	#include <string.h>			// IWYU pragma: export

	#define ALIGN(n)			__attribute__((aligned(n)))

	#define BREAKPOINT			__asm volatile ("int $0x3")
#else
	#error Unknown platform
#endif

// Standard C++ includes
#include <vector>
#include <algorithm>
#include <assert.h>
#include <immintrin.h>
#include <utility>
#include <cmath>
#include <sstream>
#include <functional>

using namespace std;

// Standard types
using uint = unsigned int;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

#if defined(_WIN32)
	#define UINT64_FORMAT_SPECIFIER "%I64u"

	#define f_inline __forceinline

	#define CACHE_LINE_SIZE 64

	// Which instructions to use
	#define USE_AVX2
	#define USE_FMADD
#elif defined(__linux__)
	#define UINT64_FORMAT_SPECIFIER "%lu"

	#define f_inline __inline__ __attribute__((always_inline))

	#define CACHE_LINE_SIZE 64

	// Which instructions to use
	#define USE_AVX2
	#define USE_FMADD
#else
	#error Undefined
#endif

// Assert sizes of types
static_assert(sizeof(uint) >= 4, "Invalid size of uint");
static_assert(sizeof(uint8) == 1, "Invalid size of uint8");
static_assert(sizeof(uint16) == 2, "Invalid size of uint16");
static_assert(sizeof(uint32) == 4, "Invalid size of uint32");
static_assert(sizeof(uint64) == 8, "Invalid size of uint64");

// Shorthand for #ifdef _DEBUG / #endif
#ifdef _DEBUG
	#define IF_DEBUG(x) x
#else
	#define IF_DEBUG(x)
#endif