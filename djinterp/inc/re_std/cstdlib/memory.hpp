/******************************************************************************
* djinterp [re_std]                                                   memory.hpp
*
* the C allocation functions (re-exports):
*   malloc / calloc / realloc / free, plus aligned_alloc where the
* platform has it. Re-exported because they are the runtime's allocator:
* a block from one malloc must be returned to the matching free, so
* substituting an implementation here would corrupt every allocation that
* crossed a library boundary.
*
*   THIS IS NOT THE C++ ALLOCATION PATH:
*   malloc returns raw storage and runs no constructor; free runs no
* destructor. Mixing them with new / delete on the same pointer is
* undefined behaviour. C++ code wants ::operator new (surfaced by re_std's
* <new>) or a smart pointer from <memory>; these four are here for
* interoperating with C APIs that hand ownership across the boundary.
*
*   aligned_alloc IS GATED, NOT ASSUMED:
*   C++17 adopted it from C11, but MSVC's runtime has never provided it --
* its equivalent is _aligned_malloc, which must be released with
* _aligned_free rather than free, so it is NOT a drop-in and is not
* aliased to this name. Apple's libc gained it only in macOS 10.15.
* Where it is absent the name is simply not declared: an omission the
* caller can test for with the detection macro, rather than a redirect to
* a function with different release semantics.
*
*   Two further constraints on aligned_alloc that survive the re-export,
* because they are the C standard's and not the platform's: the alignment
* must be one the implementation supports, and the size must be a
* multiple of it.
*
*
* path:      /inc/djinterp/re_std/cstdlib/memory.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDLIB_MEMORY_
#define DJINTERP_RE_STD_CSTDLIB_MEMORY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


// D_RE_STD_HAS_ALIGNED_ALLOC
//   constant: 1 if std::aligned_alloc is declared. C++17 and later only,
// and never on MSVC. Overridable by the user for a runtime the checks
// below do not know about.
#ifndef D_RE_STD_HAS_ALIGNED_ALLOC
    #if !D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_RE_STD_HAS_ALIGNED_ALLOC  0
    #elif defined(D_ENV_COMPILER_MSVC) || defined(_MSC_VER)
        #define D_RE_STD_HAS_ALIGNED_ALLOC  0
    #elif defined(__APPLE__)
        // Apple's libc declares it from the macOS 10.15 / iOS 13 SDKs on.
        #if defined(__MAC_OS_X_VERSION_MIN_REQUIRED) &&                       \
            __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
            #define D_RE_STD_HAS_ALIGNED_ALLOC  0
        #else
            #define D_RE_STD_HAS_ALIGNED_ALLOC  1
        #endif
    #else
        #define D_RE_STD_HAS_ALIGNED_ALLOC  1
    #endif
#endif


NS_RESTD

    // malloc
    //   function: allocate uninitialised storage, or null on failure.
    using ::std::malloc;

    // calloc
    //   function: allocate zero-initialised storage for n objects.
    using ::std::calloc;

    // realloc
    //   function: resize a block, possibly moving it. Returns null on
    // failure WITHOUT freeing the original, so assigning the result over
    // the only surviving pointer leaks the block.
    using ::std::realloc;

    // free
    //   function: release a block obtained from malloc / calloc /
    // realloc / aligned_alloc. Never from new.
    using ::std::free;

#if D_RE_STD_HAS_ALIGNED_ALLOC

    // aligned_alloc
    //   function: allocate storage at the requested alignment. Released
    // with free, unlike the Windows _aligned_malloc it is often confused
    // with. Absent where D_RE_STD_HAS_ALIGNED_ALLOC is 0.
    using ::std::aligned_alloc;

#endif  // D_RE_STD_HAS_ALIGNED_ALLOC

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDLIB_MEMORY_
