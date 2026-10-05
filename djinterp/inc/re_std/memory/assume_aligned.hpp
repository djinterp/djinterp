/*******************************************************************************
* djinterp [re_std]                                           assume_aligned.hpp
*
* alignment-promise helper:
*   assume_aligned<N>(_p) is a hint to the compiler that the pointer
* _p is aligned to at least N bytes. Implementations are free to use
* the hint to emit better code (e.g. wider load/store, vectorisation).
*
* the hint is informational only; if it lies, behaviour is undefined.
* N must be a power of two.
*
* implementation strategy (in priority order):
*   1. C++20+ with standard intrinsic available  ->  __builtin_assume_aligned
*      (clang, gcc, intel)
*   2. MSVC __assume(...) on the address modulo
*   3. plain return _p (no-op fallback — semantically correct, just
*      no optimization hint)
*
* added in std C++20; re_std back-ports the helper unconditionally on
* C++11+.
*
*
* path:      /inc/re_std/memory/assume_aligned.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_MEMORY_ASSUME_ALIGNED_HPP
#define RE_STD_MEMORY_ASSUME_ALIGNED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>

    // re_std
    #include "../cstdint/cstdint.hpp"  // uintptr_t


namespace re_std
{

template<std::size_t N, typename T>
RE_STD_CONSTEXPR T* assume_aligned(T* _p) RE_STD_NOEXCEPT
{
    #if defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER)
        // __builtin_assume_aligned returns void*; cast back to T*.
        return static_cast<T*>(__builtin_assume_aligned(_p, N));
    #elif defined(_MSC_VER)
        // MSVC has no equivalent that returns the pointer; __assume
        // is a hint-only intrinsic. Emit it with a reinterpret to
        // uintptr to communicate the alignment, then return the
        // unmodified pointer.
        __assume(reinterpret_cast<uintptr_t>(_p) % N == 0);
        return _p;
    #else
        // Unknown compiler: degrade to identity.
        return _p;
    #endif
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_ASSUME_ALIGNED_HPP
