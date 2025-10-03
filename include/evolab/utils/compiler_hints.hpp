#pragma once

/// @file compiler_hints.hpp
/// @brief Compiler hints for branch prediction and optimization
///
/// Provides portable macros for compiler optimization hints including
/// branch prediction (likely/unlikely), alignment assumptions, and
/// restrict pointers. These hints help the compiler generate more
/// efficient code for performance-critical paths.

/// Branch prediction hint: condition is likely to be true
/// Use when a branch is taken in >90% of cases
#if defined(__GNUC__) || defined(__clang__)
#define EVOLAB_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define EVOLAB_LIKELY(x) (x)
#endif

/// Branch prediction hint: condition is unlikely to be true
/// Use when a branch is taken in <10% of cases
#if defined(__GNUC__) || defined(__clang__)
#define EVOLAB_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define EVOLAB_UNLIKELY(x) (x)
#endif

/// Pointer aliasing hint (GCC/Clang specific)
/// Indicates that a pointer does not alias with other pointers
#if defined(__GNUC__) || defined(__clang__)
#define EVOLAB_RESTRICT __restrict__
#else
#define EVOLAB_RESTRICT
#endif

/// Alignment assumption hint
/// Tells the compiler that a pointer is aligned to N bytes
#if defined(__GNUC__) || defined(__clang__)
#define EVOLAB_ASSUME_ALIGNED(ptr, align) __builtin_assume_aligned(ptr, align)
#elif defined(_MSC_VER)
#define EVOLAB_ASSUME_ALIGNED(ptr, align) __assume((((uintptr_t)(ptr)) & ((align) - 1)) == 0)
#else
#define EVOLAB_ASSUME_ALIGNED(ptr, align) (ptr)
#endif

/// Force inline hint (stronger than inline keyword)
#if defined(__GNUC__) || defined(__clang__)
#define EVOLAB_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define EVOLAB_FORCE_INLINE __forceinline
#else
#define EVOLAB_FORCE_INLINE inline
#endif

/// No inline hint (prevent inlining)
#if defined(__GNUC__) || defined(__clang__)
#define EVOLAB_NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define EVOLAB_NO_INLINE __declspec(noinline)
#else
#define EVOLAB_NO_INLINE
#endif

namespace evolab::utils {} // namespace evolab::utils
