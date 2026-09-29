/*******************************************************************************
* djinterp [env]                                         env_vendor_attributes.h
*
* djinterp portable wrappers for vendor-specific attributes.
*   D_* macros for compiler attributes and builtins that have no standard
* [[...]] form, or whose standard form arrived too recently to rely on
* everywhere. Each expands to the spelling the detected compiler and language
* standard accept -- the __attribute__ family GCC and Clang share, MSVC's
* __declspec, or a language keyword -- and otherwise to a fallback that is safe
* to use.
*   It does not redefine macros that live in other headers: D_INLINE,
* D_NOINLINE, and D_RESTRICT belong to djinterp.h, and the standard attributes
* with vendor fallbacks (D_NORETURN, D_DEPRECATED, D_NODISCARD, and the rest) to
* env_attributes.h.
*   Every macro is pre-definable: #define it before including this header to
* override the detected value. Every macro is always defined except
* D_THREAD_LOCAL, which is left undefined where the compiler has no
* thread-local storage; D_THREAD_LOCAL_AVAILABLE says which.
*   It requires env.h, for the D_ENV_LANG_* and D_ENV_COMPILER_* families it
* reads, and includes it itself.
*
*
* path:      /inc/djinterp/env/c/env_vendor_attributes.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.11.12
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  COMPILER FAMILY
    ---------------
    1.  GCC-compatible compilers
         1.  D_INTERNAL_ENV_GCC_COMPAT
2.  FUNCTION PURITY AND OPTIMISATION
    --------------------------------
    1.  Purity
         1.  D_PURE
         2.  D_CONST
    2.  Optimisation hints
         1.  D_HOT
         2.  D_COLD
         3.  D_FLATTEN
3.  MEMORY AND ALLOCATION
    ---------------------
    1.  Allocation functions
         1.  D_MALLOC
         2.  D_ALLOC_SIZE
         3.  D_ALLOC_ALIGN
    2.  Layout
         1.  D_ALIGNED
         2.  D_PACKED
4.  NULL AND PARAMETER CONTRACTS
    ----------------------------
    1.  Pointer contracts
         1.  D_NONNULL
         2.  D_NONNULL_ALL
         3.  D_RETURNS_NONNULL
5.  FORMAT-STRING CHECKING
    ----------------------
    1.  Format strings
         1.  D_FORMAT_PRINTF
         2.  D_FORMAT_SCANF
6.  SYMBOL VISIBILITY AND LINKAGE
    -----------------------------
    1.  Visibility
         1.  D_EXPORT
         2.  D_IMPORT
         3.  D_HIDDEN
    2.  Linkage
         1.  D_WEAK
7.  SECTIONS AND LIFETIME
    ---------------------
    1.  Linker sections
         1.  D_SECTION
         2.  D_USED
    2.  Startup and shutdown
         1.  D_CONSTRUCTOR / D_DESTRUCTOR
8.  BRANCH PREDICTION
    -----------------
    1.  Expression-level hints
         1.  D_EXPECT
         2.  D_EXPECT_TRUE / D_EXPECT_FALSE
9.  MISCELLANEOUS
    -------------
    1.  Code generation
         1.  D_UNREACHABLE
         2.  D_PREFETCH
    2.  Storage duration
         1.  D_THREAD_LOCAL
         2.  D_THREAD_LOCAL_AVAILABLE
    3.  Function prologues
         1.  D_NAKED
*/

#ifndef DJINTERP_ENV_C_ENV_VENDOR_ATTRIBUTES_H
#define DJINTERP_ENV_C_ENV_VENDOR_ATTRIBUTES_H 1

// djinterp
#include "../env.h"  // D_ENV_LANG_*, D_ENV_COMPILER_*


//==============================================================================
// 1.  COMPILER FAMILY
//==============================================================================
// Most attributes below have a GNU spelling, which GCC and Clang share, and
// several also have an MSVC __declspec spelling. The helper here identifies the
// GNU family; it is file-local, and #undef'd at the end of this header.


// 1.1    GCC-compatible compilers
//------------------------------------------------------------------------------
// 1.1.1
// D_INTERNAL_ENV_GCC_COMPAT
//   macro (internal): defined, to 1, when the compiler accepts GCC's
// __attribute__ and __builtin_* spellings: GCC, and Clang on every target.
// clang-cl counts as Clang (see D_ENV_COMPILER_MSVC_FAMILY), so it takes these
// spellings rather than MSVC's.
#if ( (defined(D_ENV_COMPILER_GCC)) ||                                         \
      (defined(D_ENV_COMPILER_CLANG)) )
    #define D_INTERNAL_ENV_GCC_COMPAT 1
#endif


//==============================================================================
// 2.  FUNCTION PURITY AND OPTIMISATION
//==============================================================================


// 2.1    Purity
//------------------------------------------------------------------------------
// 2.1.1
// D_PURE
//   macro: declares that a function has no side effects and that its return
// value depends only on its parameters and on global state. The compiler may
// eliminate redundant calls while that state is unchanged.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((pure)).
//     2. No-op fallback.
#ifndef D_PURE
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_PURE __attribute__((pure))
    #else
        #define D_PURE
    #endif
#endif  // D_PURE

// 2.1.2
// D_CONST
//   macro: stricter than D_PURE: the function depends only on its parameters,
// reading neither global memory nor anything through a pointer. Calls with
// identical arguments may be merged, or hoisted out of loops.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((const)).
//     2. No-op fallback.
#ifndef D_CONST
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_CONST __attribute__((const))
    #else
        #define D_CONST
    #endif
#endif  // D_CONST

// 2.2    Optimisation hints
//------------------------------------------------------------------------------
// 2.2.1
// D_HOT
//   macro: hints that the function is on a hot path. The compiler may place it
// in a dedicated section and optimise it more aggressively.
//
//   resolution order:
//     1. GCC 4.3+ / Clang - __attribute__((hot)).
//     2. No-op fallback.
#ifndef D_HOT
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_HOT __attribute__((hot))
    #else
        #define D_HOT
    #endif
#endif  // D_HOT

// 2.2.2
// D_COLD
//   macro: hints that the function is rarely executed, such as an error
// handler or an initialisation path. The compiler may place it in a cold
// section and optimise it for size rather than speed.
//
//   resolution order:
//     1. GCC 4.3+ / Clang - __attribute__((cold)).
//     2. No-op fallback.
#ifndef D_COLD
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_COLD __attribute__((cold))
    #else
        #define D_COLD
    #endif
#endif  // D_COLD

// 2.2.3
// D_FLATTEN
//   macro: requests that every call inside the annotated function be inlined,
// whatever the callees' own inline hints. Useful for hot dispatch wrappers, and
// for critical loops that call many small helpers.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((flatten)).
//     2. No-op fallback.
#ifndef D_FLATTEN
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_FLATTEN __attribute__((flatten))
    #else
        #define D_FLATTEN
    #endif
#endif  // D_FLATTEN


//==============================================================================
// 3.  MEMORY AND ALLOCATION
//==============================================================================


// 3.1    Allocation functions
//------------------------------------------------------------------------------
// 3.1.1
// D_MALLOC
//   macro: declares that the function returns a pointer to newly allocated
// memory that aliases no other pointer visible to the caller, enabling the
// alias-analysis optimisations the compiler applies to malloc(3).
//
//   resolution order:
//     1. GCC / Clang - __attribute__((malloc)).
//     2. MSVC - __declspec(restrict), which makes the same no-alias promise
//        about the result.
//     3. No-op fallback.
#ifndef D_MALLOC
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_MALLOC __attribute__((malloc))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_MALLOC __declspec(restrict)
    #else
        #define D_MALLOC
    #endif
#endif  // D_MALLOC

// 3.1.2
// D_ALLOC_SIZE
//   macro: names, by 1-based position, the parameters of an allocation
// function that give the size of the returned block. With one argument, that
// parameter is the byte count; with two, the byte count is the product of the
// two parameters, as for calloc.
//
//   usage:
//     void* my_malloc(size_t _size) D_ALLOC_SIZE(1);
//     void* my_calloc(size_t _count,
//                     size_t _size) D_ALLOC_SIZE(1, 2);
//
//   resolution order:
//     1. GCC 4.3+ / Clang - __attribute__((alloc_size(…))).
//     2. No-op fallback.
#ifndef D_ALLOC_SIZE
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_ALLOC_SIZE(...) __attribute__((alloc_size(__VA_ARGS__)))
    #else
        #define D_ALLOC_SIZE(...)
    #endif
#endif  // D_ALLOC_SIZE

// 3.1.3
// D_ALLOC_ALIGN
//   macro: names, by 1-based position, the parameter of an allocation function
// that gives the alignment of the returned block.
//
//   usage:
//     void* my_aligned_alloc(size_t _align,
//                            size_t _size) D_ALLOC_ALIGN(1);
//
//   resolution order:
//     1. GCC 4.9+ / Clang - __attribute__((alloc_align(…))).
//     2. No-op fallback.
#ifndef D_ALLOC_ALIGN
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_ALLOC_ALIGN(n) __attribute__((alloc_align(n)))
    #else
        #define D_ALLOC_ALIGN(n)
    #endif
#endif  // D_ALLOC_ALIGN

// 3.2    Layout
//------------------------------------------------------------------------------
// 3.2.1
// D_ALIGNED
//   macro: specifies a minimum alignment, in bytes, for a type, a variable, or
// a struct member.
//
//   usage:
//     D_ALIGNED(16) float vec[4];
//     struct D_ALIGNED(64) cache_line { ... };
//
//   resolution order:
//     1. GCC / Clang - __attribute__((aligned(n))).
//     2. MSVC - __declspec(align(n)).
//     3. No-op fallback (natural alignment only).
//
//   note: MSVC's __declspec(align(…)) requires a compile-time constant, and
// cannot take a template parameter or a constexpr value.
#ifndef D_ALIGNED
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_ALIGNED(n) __attribute__((aligned(n)))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_ALIGNED(n) __declspec(align(n))
    #else
        #define D_ALIGNED(n)
    #endif
#endif  // D_ALIGNED

// 3.2.2
// D_PACKED
//   macro: removes the padding between struct members, so the struct occupies
// the minimum number of bytes.
//
//   usage:
//     struct D_PACKED wire_header
//     {
//         uint8_t  kind;
//         uint32_t length;
//     };
//
//   resolution order:
//     1. GCC / Clang - __attribute__((packed)).
//     2. No-op fallback.
//
//   note: MSVC packs with #pragma pack(push, 1) and #pragma pack(pop) rather
// than a per-type attribute; wrap the struct declaration in those pragmas by
// hand.
#ifndef D_PACKED
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_PACKED __attribute__((packed))
    #else
        #define D_PACKED
    #endif
#endif  // D_PACKED


//==============================================================================
// 4.  NULL AND PARAMETER CONTRACTS
//==============================================================================


// 4.1    Pointer contracts
//------------------------------------------------------------------------------
// 4.1.1
// D_NONNULL
//   macro: declares that the listed parameters, by 1-based position, must not
// be NULL. The compiler may warn when a provably null argument is passed, and
// may optimise on the assumption that those pointers are non-null.
//
//   usage:
//     void copy(void*       _dst,
//               const void* _src,
//               size_t      _count) D_NONNULL(1, 2);
//
//   resolution order:
//     1. GCC / Clang - __attribute__((nonnull(…))).
//     2. No-op fallback.
#ifndef D_NONNULL
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
    #else
        #define D_NONNULL(...)
    #endif
#endif  // D_NONNULL

// 4.1.2
// D_NONNULL_ALL
//   macro: shorthand declaring that no pointer parameter may be NULL.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((nonnull)).
//     2. No-op fallback.
#ifndef D_NONNULL_ALL
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_NONNULL_ALL __attribute__((nonnull))
    #else
        #define D_NONNULL_ALL
    #endif
#endif  // D_NONNULL_ALL

// 4.1.3
// D_RETURNS_NONNULL
//   macro: declares that the function never returns NULL, so null checks on
// its result can be elided at the call site.
//
//   resolution order:
//     1. GCC 4.9+ / Clang - __attribute__((returns_nonnull)).
//     2. No-op fallback.
#ifndef D_RETURNS_NONNULL
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_RETURNS_NONNULL __attribute__((returns_nonnull))
    #else
        #define D_RETURNS_NONNULL
    #endif
#endif  // D_RETURNS_NONNULL


//==============================================================================
// 5.  FORMAT-STRING CHECKING
//==============================================================================


// 5.1    Format strings
//------------------------------------------------------------------------------
// 5.1.1
// D_FORMAT_PRINTF
//   macro: enables compile-time checking of a printf-style format string
// against its arguments. `fmt_idx` is the 1-based position of the format
// parameter, and `first_arg` that of the first variadic argument, or 0 for a
// vprintf-style function, which takes a va_list.
//
//   usage:
//     void my_printf(const char* _format,
//                    ...) D_FORMAT_PRINTF(1, 2);
//     void my_vprintf(const char* _format,
//                     va_list     _args) D_FORMAT_PRINTF(1, 0);
//
//   resolution order:
//     1. GCC / Clang - __attribute__((format(printf, …, …))).
//     2. No-op fallback.
//
//   note: in a C++ non-static member function the implicit `this` occupies
// position 1, so each index is one higher than for a free function.
#ifndef D_FORMAT_PRINTF
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_FORMAT_PRINTF(fmt_idx, first_arg)                            \
            __attribute__((format(printf, fmt_idx, first_arg)))
    #else
        #define D_FORMAT_PRINTF(fmt_idx, first_arg)
    #endif
#endif  // D_FORMAT_PRINTF

// 5.1.2
// D_FORMAT_SCANF
//   macro: as D_FORMAT_PRINTF, for a scanf-style format string.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((format(scanf, …, …))).
//     2. No-op fallback.
#ifndef D_FORMAT_SCANF
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_FORMAT_SCANF(fmt_idx, first_arg)                             \
            __attribute__((format(scanf, fmt_idx, first_arg)))
    #else
        #define D_FORMAT_SCANF(fmt_idx, first_arg)
    #endif
#endif  // D_FORMAT_SCANF


//==============================================================================
// 6.  SYMBOL VISIBILITY AND LINKAGE
//==============================================================================


// 6.1    Visibility
//------------------------------------------------------------------------------
// 6.1.1
// D_EXPORT
//   macro: marks a symbol as exported from a shared library or DLL.
//
//   resolution order:
//     1. MSVC, or any compiler targeting Windows (_WIN32) -
//        __declspec(dllexport).
//     2. GCC 4+ / Clang - __attribute__((visibility("default"))).
//     3. No-op fallback.
#ifndef D_EXPORT
    #if ( (defined(D_ENV_COMPILER_MSVC)) ||                                    \
          (defined(_WIN32)) )
        #define D_EXPORT __declspec(dllexport)
    #elif defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_EXPORT __attribute__((visibility("default")))
    #else
        #define D_EXPORT
    #endif
#endif  // D_EXPORT

// 6.1.2
// D_IMPORT
//   macro: marks a symbol as imported from a shared library or DLL.
//
//   resolution order:
//     1. MSVC, or any compiler targeting Windows (_WIN32) -
//        __declspec(dllimport).
//     2. GCC / Clang - __attribute__((visibility("default"))), since ELF does
//        not distinguish import from export at the symbol level.
//     3. No-op fallback.
#ifndef D_IMPORT
    #if ( (defined(D_ENV_COMPILER_MSVC)) ||                                    \
          (defined(_WIN32)) )
        #define D_IMPORT __declspec(dllimport)
    #elif defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_IMPORT __attribute__((visibility("default")))
    #else
        #define D_IMPORT
    #endif
#endif  // D_IMPORT

// 6.1.3
// D_HIDDEN
//   macro: marks a symbol as internal to its library, and not exported. On ELF
// platforms this makes shared objects smaller and faster.
//
//   resolution order:
//     1. GCC 4+ / Clang - __attribute__((visibility("hidden"))).
//     2. No-op fallback (the symbol keeps its default visibility).
#ifndef D_HIDDEN
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_HIDDEN __attribute__((visibility("hidden")))
    #else
        #define D_HIDDEN
    #endif
#endif  // D_HIDDEN

// 6.2    Linkage
//------------------------------------------------------------------------------
// 6.2.1
// D_WEAK
//   macro: gives a symbol weak linkage. A strong definition in another
// translation unit overrides it; with none, the weak one is used. Useful for
// defaults a program may replace.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((weak)).
//     2. MSVC - __declspec(selectany), the closest equivalent for data.
//     3. No-op fallback.
//
//   note: MSVC accepts selectany only on data with external linkage, so
// D_WEAK on a function does not compile with MSVC.
#ifndef D_WEAK
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_WEAK __attribute__((weak))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_WEAK __declspec(selectany)
    #else
        #define D_WEAK
    #endif
#endif  // D_WEAK


//==============================================================================
// 7.  SECTIONS AND LIFETIME
//==============================================================================


// 7.1    Linker sections
//------------------------------------------------------------------------------
// 7.1.1
// D_SECTION
//   macro: places the annotated symbol in the named linker section.
//
//   usage:
//     D_SECTION(".my_data") int persistent_counter = 0;
//
//   resolution order:
//     1. GCC / Clang - __attribute__((section(name))).
//     2. MSVC - __declspec(allocate(name)), which needs a matching
//        #pragma section(name, …) beforehand.
//     3. No-op fallback.
#ifndef D_SECTION
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_SECTION(name) __attribute__((section(name)))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_SECTION(name) __declspec(allocate(name))
    #else
        #define D_SECTION(name)
    #endif
#endif  // D_SECTION

// 7.1.2
// D_USED
//   macro: keeps the linker from stripping the symbol even when nothing
// references it. Commonly paired with D_SECTION for registration tables,
// plugin descriptors, and linker-set entries.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((used)).
//     2. No-op fallback (LTO or --gc-sections may strip the symbol).
#ifndef D_USED
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_USED __attribute__((used))
    #else
        #define D_USED
    #endif
#endif  // D_USED

// 7.2    Startup and shutdown
//------------------------------------------------------------------------------
// 7.2.1
// D_CONSTRUCTOR / D_DESTRUCTOR
//   macro: declares a function the runtime calls automatically, before main()
// for D_CONSTRUCTOR, and after main() returns or exit() is called for
// D_DESTRUCTOR.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((constructor)) / ((destructor)).
//     2. No-op fallback.
//
//   note: MSVC achieves the same through CRT initialisation segments
// (#pragma section(".CRT$XCU", …)) and function pointers, a pattern no simple
// macro can express.
#ifndef D_CONSTRUCTOR
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_CONSTRUCTOR __attribute__((constructor))
    #else
        #define D_CONSTRUCTOR
    #endif
#endif  // D_CONSTRUCTOR

#ifndef D_DESTRUCTOR
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_DESTRUCTOR __attribute__((destructor))
    #else
        #define D_DESTRUCTOR
    #endif
#endif  // D_DESTRUCTOR


//==============================================================================
// 8.  BRANCH PREDICTION
//==============================================================================
// These complement D_LIKELY / D_UNLIKELY from env_attributes.h. The standard
// [[likely]] / [[unlikely]] attributes apply to statements; these macros work
// at the expression level, through __builtin_expect, so C can use them too.


// 8.1    Expression-level hints
//------------------------------------------------------------------------------
// 8.1.1
// D_EXPECT
//   macro: a general __builtin_expect wrapper, telling the compiler that `expr`
// is expected to evaluate to `val`. `expr` is normalised to 0 or 1 first, so
// `val` should be 0 or 1.
//
//   resolution order:
//     1. GCC / Clang - __builtin_expect(…).
//     2. Fallback - the normalised `expr` alone.
#ifndef D_EXPECT
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_EXPECT(expr, val) __builtin_expect(!!(expr), (val))
    #else
        #define D_EXPECT(expr, val) (!!(expr))
    #endif
#endif  // D_EXPECT

// 8.1.2
// D_EXPECT_TRUE / D_EXPECT_FALSE
//   macro: D_EXPECT for the common cases: a condition expected to be true, or
// expected to be false.
#ifndef D_EXPECT_TRUE
    #define D_EXPECT_TRUE(expr)  D_EXPECT((expr), 1)
#endif  // D_EXPECT_TRUE

#ifndef D_EXPECT_FALSE
    #define D_EXPECT_FALSE(expr) D_EXPECT((expr), 0)
#endif  // D_EXPECT_FALSE


//==============================================================================
// 9.  MISCELLANEOUS
//==============================================================================


// 9.1    Code generation
//------------------------------------------------------------------------------
// 9.1.1
// D_UNREACHABLE
//   macro: marks a code path that is never reached, enabling dead-code
// optimisations. Reaching it is undefined behaviour on the standard-library
// and compiler tiers, though libstdc++'s std::unreachable() traps in
// _GLIBCXX_ASSERTIONS builds.
//
//   resolution order:
//     1. C++23 - std::unreachable(), declared in <utility>.
//     2. GCC / Clang - __builtin_unreachable().
//     3. MSVC - __assume(0).
//     4. Infinite-loop fallback.
//
//   note: this header does not include <utility>, so a C++23 translation unit
// that uses D_UNREACHABLE must include it itself.
#ifndef D_UNREACHABLE
    #if ( (defined(__cplusplus)) &&                                            \
          (D_ENV_LANG_IS_CPP23_OR_HIGHER) )
        #define D_UNREACHABLE std::unreachable()
    #elif defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_UNREACHABLE __builtin_unreachable()
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_UNREACHABLE __assume(0)
    #else
        #define D_UNREACHABLE do { for(;;); } while(0)
    #endif
#endif  // D_UNREACHABLE

// 9.1.2
// D_PREFETCH
//   macro: issues a software prefetch hint for the cache line holding `addr`,
// for reading, with low temporal locality.
//
//   resolution order:
//     1. GCC / Clang - __builtin_prefetch(addr, 0, 0).
//     2. No-op fallback.
//
//   note: MSVC and Intel provide _mm_prefetch, but it needs <xmmintrin.h>,
// which this header does not pull in; pre-define D_PREFETCH to use it there.
#ifndef D_PREFETCH
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_PREFETCH(addr) __builtin_prefetch((addr), 0, 0)
    #else
        #define D_PREFETCH(addr) ((void)(addr))
    #endif
#endif  // D_PREFETCH

// 9.2    Storage duration
//------------------------------------------------------------------------------
// 9.2.1
// D_THREAD_LOCAL
//   macro: declares a variable with thread-local storage duration.
//
//   resolution order:
//     1. C++11 / C23 - the thread_local keyword.
//     2. C11 - the _Thread_local keyword.
//     3. GCC / Clang - __thread.
//     4. MSVC - __declspec(thread).
//     5. Otherwise left undefined, and D_THREAD_LOCAL_AVAILABLE is 0.
//
//   note: an empty definition would silently give a per-thread variable
// ordinary static storage, shared by every thread. Leaving the macro
// undefined makes such a declaration a compile error instead, and code
// with a fallback of its own tests D_THREAD_LOCAL_AVAILABLE. This is the
// one macro in this header that can be left undefined.
#ifndef D_THREAD_LOCAL
    #if defined(__cplusplus)
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_THREAD_LOCAL thread_local
        #elif defined(D_INTERNAL_ENV_GCC_COMPAT)
            #define D_THREAD_LOCAL __thread
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_THREAD_LOCAL __declspec(thread)
        #endif
    #else
        #if D_ENV_LANG_IS_C23_OR_HIGHER
            #define D_THREAD_LOCAL thread_local
        #elif D_ENV_LANG_IS_C11_OR_HIGHER
            #define D_THREAD_LOCAL _Thread_local
        #elif defined(D_INTERNAL_ENV_GCC_COMPAT)
            #define D_THREAD_LOCAL __thread
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_THREAD_LOCAL __declspec(thread)
        #endif
    #endif
#endif  // D_THREAD_LOCAL

// 9.2.2
// D_THREAD_LOCAL_AVAILABLE
//   constant: 1 when D_THREAD_LOCAL is defined, by the cascade above or by
// the build, and 0 when the compiler offers no thread-local storage.
#ifndef D_THREAD_LOCAL_AVAILABLE
    #ifdef D_THREAD_LOCAL
        #define D_THREAD_LOCAL_AVAILABLE 1
    #else
        #define D_THREAD_LOCAL_AVAILABLE 0
    #endif  // D_THREAD_LOCAL
#endif  // D_THREAD_LOCAL_AVAILABLE

// 9.3    Function prologues
//------------------------------------------------------------------------------
// 9.3.1
// D_NAKED
//   macro: omits the compiler-generated prologue and epilogue (no stack-frame
// setup, register saves, or return sequence), so the function body must be
// written entirely in inline assembly.
//
//   resolution order:
//     1. GCC / Clang - __attribute__((naked)).
//     2. MSVC - __declspec(naked), on x86 only.
//     3. No-op fallback.
#ifndef D_NAKED
    #if defined(D_INTERNAL_ENV_GCC_COMPAT)
        #define D_NAKED __attribute__((naked))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_NAKED __declspec(naked)
    #else
        #define D_NAKED
    #endif
#endif  // D_NAKED


// D_INTERNAL_ENV_GCC_COMPAT is file-local; see 1.1.1
#undef D_INTERNAL_ENV_GCC_COMPAT


#endif  // DJINTERP_ENV_C_ENV_VENDOR_ATTRIBUTES_H
