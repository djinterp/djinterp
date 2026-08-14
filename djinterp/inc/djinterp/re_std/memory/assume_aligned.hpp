/***********************************************************************
* restd                                                   assume_aligned.hpp
*
* alignment-promise helper:
*   assume_aligned<_N>(_p) is a hint to the compiler that the pointer
* _p is aligned to at least _N bytes. Implementations are free to use
* the hint to emit better code (e.g. wider load/store, vectorisation).
*
* the hint is informational only; if it lies, behaviour is undefined.
* _N must be a power of two.
*
* implementation strategy (in priority order):
*   1. C++20+ with standard intrinsic available  ->  __builtin_assume_aligned
*      (clang, gcc, intel)
*   2. MSVC __assume(...) on the address modulo
*   3. plain return _p (no-op fallback — semantically correct, just
*      no optimization hint)
*
* added in std C++20; restd back-ports the helper unconditionally on
* C++11+.
*
*
* path:      /inc/djinterp/re_std/memory/assume_aligned.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_ASSUME_ALIGNED_
#define RESTD_MEMORY_ASSUME_ALIGNED_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>


namespace restd
{

template<std::size_t _N, typename _T>
D_CONSTEXPR _T* assume_aligned(_T* _p) D_NOEXCEPT
{
    #if defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER)
        // __builtin_assume_aligned returns void*; cast back to _T*.
        return static_cast<_T*>(__builtin_assume_aligned(_p, _N));
    #elif defined(_MSC_VER)
        // MSVC has no equivalent that returns the pointer; __assume
        // is a hint-only intrinsic. Emit it with a reinterpret to
        // uintptr to communicate the alignment, then return the
        // unmodified pointer.
        __assume(reinterpret_cast<std::uintptr_t>(_p) % _N == 0);
        return _p;
    #else
        // Unknown compiler: degrade to identity.
        return _p;
    #endif
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_ASSUME_ALIGNED_
