/******************************************************************************
* djinterp [core]                                       env_vendor_attributes.h
*
*   Portable wrappers for vendor-specific compiler attributes that have no
*   standard [[…]] equivalent (or whose standard form arrived too recently
*   to be relied upon universally).
*
*   Requires:  env.h  (must be #included first for D_ENV_COMPILER_*,
*              D_ENV_LANG_*, and D_ENV_OS_* detection macros).
*
*   This header intentionally does NOT redefine macros that already live
*   in other headers:
*     djinterp.h          — D_INLINE, D_NOINLINE, D_RESTRICT
*     env_attributes.h    — D_NORETURN, D_DEPRECATED, D_NODISCARD, etc.
*                           (standard attributes with vendor fallbacks)
*
*   Attributes defined herein:
*
*     Function purity & optimisation
*       D_PURE                    pure function (reads globals, no writes)
*       D_CONST                   const function (depends only on params)
*       D_HOT                     hot-path optimisation hint
*       D_COLD                    cold-path optimisation hint
*       D_FLATTEN                 inline all calls within the function
*
*     Memory & allocation
*       D_MALLOC                  returns pointer to unaliased memory
*       D_ALLOC_SIZE(...)         which params describe allocation size
*       D_ALLOC_ALIGN(n)          which param gives alignment
*       D_ALIGNED(n)              minimum alignment for types/variables
*       D_PACKED                  remove struct padding
*
*     Null & parameter contracts
*       D_NONNULL(...)            listed params must not be NULL
*       D_NONNULL_ALL             all pointer params must not be NULL
*       D_RETURNS_NONNULL         return value is never NULL
*
*     Format-string checking
*       D_FORMAT_PRINTF(fmt, va)  printf-style format/args validation
*       D_FORMAT_SCANF(fmt, va)   scanf-style format/args validation
*
*     Symbol visibility & linkage
*       D_EXPORT                  public symbol (dllexport / default)
*       D_IMPORT                  imported symbol (dllimport / default)
*       D_HIDDEN                  hidden symbol (not exported)
*       D_WEAK                    weak linkage
*
*     Section & lifetime
*       D_SECTION(name)           place symbol in named section
*       D_USED                    retain symbol even if unreferenced
*       D_CONSTRUCTOR             run before main
*       D_DESTRUCTOR              run after main
*
*     Branch prediction (expression-level)
*       D_EXPECT(expr, val)       general __builtin_expect wrapper
*       D_EXPECT_TRUE(expr)       branch expected to be taken
*       D_EXPECT_FALSE(expr)      branch expected NOT to be taken
*
*     Miscellaneous
*       D_UNREACHABLE             mark unreachable code paths
*       D_PREFETCH(addr)          software prefetch hint
*       D_THREAD_LOCAL            thread-local storage duration
*       D_NAKED                   omit function prologue/epilogue
*
*   Every macro is pre-definable: #define it before including this header
*   to override the detected value.
*
* path:      /inc/c/core/config/env_vendor_attributes.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          date: 2023.11.12
******************************************************************************/

#ifndef DJINTERP_ENV_VENDOR_ATTRIBUTES_
#define DJINTERP_ENV_VENDOR_ATTRIBUTES_ 1


// Internal helper: true when the compiler is GCC-compatible (GCC or Clang).
#if ( defined(D_ENV_COMPILER_GCC) ||  \
      defined(D_ENV_COMPILER_CLANG) )
    #define D_INTERNAL_GCC_COMPAT_ 1
#endif


// =============================================================================
// I.   FUNCTION PURITY & OPTIMISATION
// =============================================================================


// -----------------------------------------------------------------------------
// D_PURE
//   Declares that a function has no side effects and its return value
//   depends only on its parameters and global state.  The compiler may
//   eliminate redundant calls when the relevant state has not changed.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((pure)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_PURE
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_PURE __attribute__((pure))
    #else
        #define D_PURE
    #endif
#endif  // D_PURE


// -----------------------------------------------------------------------------
// D_CONST
//   Stricter than D_PURE: the function depends ONLY on its parameters
//   (no reads from global memory or dereferenced pointers).  Calls with
//   identical arguments may be freely CSE'd or hoisted out of loops.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((const)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_CONST
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_CONST __attribute__((const))
    #else
        #define D_CONST
    #endif
#endif  // D_CONST


// -----------------------------------------------------------------------------
// D_HOT
//   Hints that the function is a hot path.  The compiler may place it
//   in a dedicated section and optimise more aggressively.
//
//   Resolution order:
//     1. GCC 4.3+ / Clang — __attribute__((hot)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_HOT
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_HOT __attribute__((hot))
    #else
        #define D_HOT
    #endif
#endif  // D_HOT


// -----------------------------------------------------------------------------
// D_COLD
//   Hints that the function is rarely executed (error handlers, init
//   paths).  The compiler may place it in a cold section and optimise
//   for size rather than speed.
//
//   Resolution order:
//     1. GCC 4.3+ / Clang — __attribute__((cold)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_COLD
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_COLD __attribute__((cold))
    #else
        #define D_COLD
    #endif
#endif  // D_COLD


// -----------------------------------------------------------------------------
// D_FLATTEN
//   Requests that every call inside the annotated function be inlined,
//   regardless of the callee's own inline hints.  Useful for hot
//   dispatch wrappers or critical loops with many small helpers.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((flatten)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_FLATTEN
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_FLATTEN __attribute__((flatten))
    #else
        #define D_FLATTEN
    #endif
#endif  // D_FLATTEN


// =============================================================================
// II.  MEMORY & ALLOCATION
// =============================================================================


// -----------------------------------------------------------------------------
// D_MALLOC
//   Declares that the function returns a pointer to newly allocated
//   memory that does not alias any other pointer visible to the caller.
//   Enables alias-analysis optimisations similar to malloc(3).
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((malloc)).
//     2. MSVC — __declspec(restrict) (identical semantics).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_MALLOC
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_MALLOC __attribute__((malloc))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_MALLOC __declspec(restrict)
    #else
        #define D_MALLOC
    #endif
#endif  // D_MALLOC


// -----------------------------------------------------------------------------
// D_ALLOC_SIZE(...)
//   Specifies which parameter(s) of an allocation function describe the
//   total size of the returned block.  One argument means that parameter
//   IS the byte count; two arguments means the product of the two
//   parameters is the byte count (like calloc).
//
//   Usage:
//     void* my_malloc(size_t n) D_ALLOC_SIZE(1);
//     void* my_calloc(size_t n, size_t sz) D_ALLOC_SIZE(1, 2);
//
//   Resolution order:
//     1. GCC 4.3+ / Clang — __attribute__((alloc_size(…))).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_ALLOC_SIZE
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_ALLOC_SIZE(...) __attribute__((alloc_size(__VA_ARGS__)))
    #else
        #define D_ALLOC_SIZE(...)
    #endif
#endif  // D_ALLOC_SIZE


// -----------------------------------------------------------------------------
// D_ALLOC_ALIGN(n)
//   Specifies which parameter of an allocation function describes the
//   alignment of the returned block.
//
//   Usage:
//     void* my_aligned_alloc(size_t align, size_t sz) D_ALLOC_ALIGN(1);
//
//   Resolution order:
//     1. GCC 4.9+ / Clang — __attribute__((alloc_align(…))).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_ALLOC_ALIGN
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_ALLOC_ALIGN(n) __attribute__((alloc_align(n)))
    #else
        #define D_ALLOC_ALIGN(n)
    #endif
#endif  // D_ALLOC_ALIGN


// -----------------------------------------------------------------------------
// D_ALIGNED(n)
//   Specifies a minimum alignment (in bytes) for a type, variable, or
//   struct member.
//
//   Usage:
//     D_ALIGNED(16) float vec[4];
//     typedef struct D_ALIGNED(64) { ... } cache_line_t;
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((aligned(n))).
//     2. MSVC — __declspec(align(n)).
//     3. No-op fallback (natural alignment only).
//
//   NOTE: MSVC __declspec(align(…)) requires a compile-time constant
//   and cannot be used with template parameters or constexpr values.
// -----------------------------------------------------------------------------
#ifndef D_ALIGNED
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_ALIGNED(n) __attribute__((aligned(n)))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_ALIGNED(n) __declspec(align(n))
    #else
        #define D_ALIGNED(n)
    #endif
#endif  // D_ALIGNED


// -----------------------------------------------------------------------------
// D_PACKED
//   Removes padding between struct members so the struct occupies the
//   minimum number of bytes.
//
//   Usage:
//     typedef struct D_PACKED { uint8_t a; uint32_t b; } wire_msg_t;
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((packed)).
//     2. No-op fallback.
//
//   NOTE: MSVC uses #pragma pack(push, 1) / #pragma pack(pop) instead
//   of a per-type attribute.  For MSVC packing, wrap the struct
//   declaration with those pragmas manually.
// -----------------------------------------------------------------------------
#ifndef D_PACKED
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_PACKED __attribute__((packed))
    #else
        #define D_PACKED
    #endif
#endif  // D_PACKED


// =============================================================================
// III. NULL & PARAMETER CONTRACTS
// =============================================================================


// -----------------------------------------------------------------------------
// D_NONNULL(...)
//   Declares that the listed parameter positions (1-based) must not be
//   NULL.  The compiler may emit a warning if a provably-null argument
//   is passed and may optimise under the assumption that the pointer is
//   non-null.
//
//   Usage:
//     void copy(void* dst, const void* src, size_t n) D_NONNULL(1, 2);
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((nonnull(…))).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NONNULL
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
    #else
        #define D_NONNULL(...)
    #endif
#endif  // D_NONNULL


// -----------------------------------------------------------------------------
// D_NONNULL_ALL
//   Short-hand: ALL pointer parameters must not be NULL.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((nonnull)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NONNULL_ALL
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_NONNULL_ALL __attribute__((nonnull))
    #else
        #define D_NONNULL_ALL
    #endif
#endif  // D_NONNULL_ALL


// -----------------------------------------------------------------------------
// D_RETURNS_NONNULL
//   Declares that the function never returns NULL.  Enables the
//   compiler to elide null checks on the call site.
//
//   Resolution order:
//     1. GCC 4.9+ / Clang — __attribute__((returns_nonnull)).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_RETURNS_NONNULL
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_RETURNS_NONNULL __attribute__((returns_nonnull))
    #else
        #define D_RETURNS_NONNULL
    #endif
#endif  // D_RETURNS_NONNULL


// =============================================================================
// IV.  FORMAT-STRING CHECKING
// =============================================================================


// -----------------------------------------------------------------------------
// D_FORMAT_PRINTF(fmt_idx, first_arg)
//   Enables compile-time printf-style format-string validation.
//   `fmt_idx` is the 1-based index of the format parameter; `first_arg`
//   is the 1-based index of the first variadic argument (or 0 for
//   vprintf-style functions that take a va_list).
//
//   For C++ non-static member functions, the implicit `this` occupies
//   position 1, so indices are shifted by one compared to free functions.
//
//   Usage:
//     void my_printf(const char* fmt, ...) D_FORMAT_PRINTF(1, 2);
//     void my_vprintf(const char* fmt, va_list ap) D_FORMAT_PRINTF(1, 0);
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((format(printf, …, …))).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_FORMAT_PRINTF
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_FORMAT_PRINTF(fmt_idx, first_arg)  \
            __attribute__((format(printf, fmt_idx, first_arg)))
    #else
        #define D_FORMAT_PRINTF(fmt_idx, first_arg)
    #endif
#endif  // D_FORMAT_PRINTF


// -----------------------------------------------------------------------------
// D_FORMAT_SCANF(fmt_idx, first_arg)
//   Same as D_FORMAT_PRINTF but validates scanf-style format strings.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((format(scanf, …, …))).
//     2. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_FORMAT_SCANF
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_FORMAT_SCANF(fmt_idx, first_arg)  \
            __attribute__((format(scanf, fmt_idx, first_arg)))
    #else
        #define D_FORMAT_SCANF(fmt_idx, first_arg)
    #endif
#endif  // D_FORMAT_SCANF


// =============================================================================
// V.   SYMBOL VISIBILITY & LINKAGE
// =============================================================================


// -----------------------------------------------------------------------------
// D_EXPORT
//   Marks a symbol as publicly exported from a shared library / DLL.
//
//   Resolution order:
//     1. Windows (MSVC / MinGW) — __declspec(dllexport).
//     2. GCC 4+ / Clang — __attribute__((visibility("default"))).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_EXPORT
    #if ( defined(D_ENV_COMPILER_MSVC) ||  \
          defined(_WIN32) )
        #define D_EXPORT __declspec(dllexport)
    #elif defined(D_INTERNAL_GCC_COMPAT_)
        #define D_EXPORT __attribute__((visibility("default")))
    #else
        #define D_EXPORT
    #endif
#endif  // D_EXPORT


// -----------------------------------------------------------------------------
// D_IMPORT
//   Marks a symbol as imported from a shared library / DLL.
//
//   Resolution order:
//     1. Windows (MSVC / MinGW) — __declspec(dllimport).
//     2. GCC / Clang — __attribute__((visibility("default"))) (ELF
//        does not distinguish import from export at the symbol level).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_IMPORT
    #if ( defined(D_ENV_COMPILER_MSVC) ||  \
          defined(_WIN32) )
        #define D_IMPORT __declspec(dllimport)
    #elif defined(D_INTERNAL_GCC_COMPAT_)
        #define D_IMPORT __attribute__((visibility("default")))
    #else
        #define D_IMPORT
    #endif
#endif  // D_IMPORT


// -----------------------------------------------------------------------------
// D_HIDDEN
//   Marks a symbol as library-internal (not exported).  On ELF
//   platforms this produces smaller, faster shared objects.
//
//   Resolution order:
//     1. GCC 4+ / Clang — __attribute__((visibility("hidden"))).
//     2. No-op fallback (symbol remains at its default visibility).
// -----------------------------------------------------------------------------
#ifndef D_HIDDEN
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_HIDDEN __attribute__((visibility("hidden")))
    #else
        #define D_HIDDEN
    #endif
#endif  // D_HIDDEN


// -----------------------------------------------------------------------------
// D_WEAK
//   Declares a symbol with weak linkage.  A strong definition in
//   another translation unit will override it; if no strong definition
//   exists, the weak one is used.  Useful for providing overridable
//   defaults.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((weak)).
//     2. MSVC — __declspec(selectany) (closest equivalent for data;
//        no exact analogue for functions).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_WEAK
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_WEAK __attribute__((weak))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_WEAK __declspec(selectany)
    #else
        #define D_WEAK
    #endif
#endif  // D_WEAK


// =============================================================================
// VI.  SECTION & LIFETIME
// =============================================================================


// -----------------------------------------------------------------------------
// D_SECTION(name)
//   Places the annotated symbol into the named linker section.
//
//   Usage:
//     D_SECTION(".my_data") int persistent_counter = 0;
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((section(name))).
//     2. MSVC — __declspec(allocate(name))  (requires a matching
//        #pragma section(name, …) beforehand).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_SECTION
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_SECTION(name) __attribute__((section(name)))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_SECTION(name) __declspec(allocate(name))
    #else
        #define D_SECTION(name)
    #endif
#endif  // D_SECTION


// -----------------------------------------------------------------------------
// D_USED
//   Prevents the linker from stripping the symbol even if it appears
//   unreferenced.  Commonly paired with D_SECTION for registration
//   tables, plugin descriptors, or linker-set entries.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((used)).
//     2. No-op fallback (symbol may be stripped by LTO or --gc-sections).
// -----------------------------------------------------------------------------
#ifndef D_USED
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_USED __attribute__((used))
    #else
        #define D_USED
    #endif
#endif  // D_USED


// -----------------------------------------------------------------------------
// D_CONSTRUCTOR / D_DESTRUCTOR
//   Declares functions that are called automatically before main()
//   (constructor) or after main() / exit() (destructor).
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((constructor)) / ((destructor)).
//     2. No-op fallback.
//
//   NOTE: MSVC achieves the same via CRT initialisation segments
//   (#pragma section(".CRT$XCU", …)) and function pointers.  That
//   pattern cannot be expressed as a simple macro.
// -----------------------------------------------------------------------------
#ifndef D_CONSTRUCTOR
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_CONSTRUCTOR __attribute__((constructor))
    #else
        #define D_CONSTRUCTOR
    #endif
#endif  // D_CONSTRUCTOR

#ifndef D_DESTRUCTOR
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_DESTRUCTOR __attribute__((destructor))
    #else
        #define D_DESTRUCTOR
    #endif
#endif  // D_DESTRUCTOR


// =============================================================================
// VII. BRANCH PREDICTION (EXPRESSION-LEVEL)
// =============================================================================
//
//   These complement D_LIKELY / D_UNLIKELY from env_attributes.h.
//   The [[likely]] / [[unlikely]] standard attributes are statement-
//   level; these macros operate at the expression level via
//   __builtin_expect and are usable in C as well.
//


// -----------------------------------------------------------------------------
// D_EXPECT(expr, val)
//   General-purpose __builtin_expect wrapper.  Tells the compiler that
//   `expr` is expected to evaluate to `val`.
//
//   Resolution order:
//     1. GCC / Clang — __builtin_expect(…).
//     2. Identity fallback.
// -----------------------------------------------------------------------------
#ifndef D_EXPECT
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_EXPECT(expr, val) __builtin_expect(!!(expr), (val))
    #else
        #define D_EXPECT(expr, val) (!!(expr))
    #endif
#endif  // D_EXPECT


// -----------------------------------------------------------------------------
// D_EXPECT_TRUE(expr) / D_EXPECT_FALSE(expr)
//   Convenience wrappers for the common case.
// -----------------------------------------------------------------------------
#ifndef D_EXPECT_TRUE
    #define D_EXPECT_TRUE(expr)  D_EXPECT((expr), 1)
#endif

#ifndef D_EXPECT_FALSE
    #define D_EXPECT_FALSE(expr) D_EXPECT((expr), 0)
#endif


// =============================================================================
// VIII. MISCELLANEOUS
// =============================================================================


// -----------------------------------------------------------------------------
// D_UNREACHABLE
//   Marks a code path that should never be reached.  Enables dead-code
//   optimisations and may trap in debug builds.
//
//   Resolution order:
//     1. C++23 — std::unreachable() (defined in <utility>).
//     2. GCC / Clang — __builtin_unreachable().
//     3. MSVC — __assume(0).
//     4. Infinite-loop fallback (safe, pessimises).
// -----------------------------------------------------------------------------
#ifndef D_UNREACHABLE
    #if defined(__cplusplus) && D_ENV_LANG_IS_CPP23_OR_HIGHER
        // NOTE: requires #include <utility> in the translation unit.
        #define D_UNREACHABLE std::unreachable()
    #elif defined(D_INTERNAL_GCC_COMPAT_)
        #define D_UNREACHABLE __builtin_unreachable()
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_UNREACHABLE __assume(0)
    #else
        #define D_UNREACHABLE do { for(;;); } while(0)
    #endif
#endif  // D_UNREACHABLE


// -----------------------------------------------------------------------------
// D_PREFETCH(addr)
//   Issues a software prefetch hint for the cache line containing
//   `addr`.  Uses read-access, low-temporal-locality defaults.
//
//   Resolution order:
//     1. GCC / Clang — __builtin_prefetch(addr, 0, 0).
//     2. MSVC / Intel — _mm_prefetch  (requires <xmmintrin.h>; left
//        as a no-op here to avoid header pollution — define D_PREFETCH
//        manually if you need it with MSVC).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_PREFETCH
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_PREFETCH(addr) __builtin_prefetch((addr), 0, 0)
    #else
        #define D_PREFETCH(addr) ((void)(addr))
    #endif
#endif  // D_PREFETCH


// -----------------------------------------------------------------------------
// D_THREAD_LOCAL
//   Declares a variable with thread-local storage duration.
//
//   Resolution order:
//     1. C++11 / C23 — thread_local keyword.
//     2. C11 — _Thread_local keyword.
//     3. GCC / Clang — __thread.
//     4. MSVC — __declspec(thread).
//     5. No-op fallback (variable has normal storage duration —
//        this is a silent degradation that may cause data races;
//        prefer a build error in production by pre-defining
//        D_THREAD_LOCAL to #error).
// -----------------------------------------------------------------------------
#ifndef D_THREAD_LOCAL
    #if defined(__cplusplus)
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_THREAD_LOCAL thread_local
        #elif defined(D_INTERNAL_GCC_COMPAT_)
            #define D_THREAD_LOCAL __thread
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_THREAD_LOCAL __declspec(thread)
        #else
            #define D_THREAD_LOCAL
        #endif
    #else
        #if D_ENV_LANG_IS_C23_OR_HIGHER
            #define D_THREAD_LOCAL thread_local
        #elif D_ENV_LANG_IS_C11_OR_HIGHER
            #define D_THREAD_LOCAL _Thread_local
        #elif defined(D_INTERNAL_GCC_COMPAT_)
            #define D_THREAD_LOCAL __thread
        #elif defined(D_ENV_COMPILER_MSVC)
            #define D_THREAD_LOCAL __declspec(thread)
        #else
            #define D_THREAD_LOCAL
        #endif
    #endif
#endif  // D_THREAD_LOCAL


// -----------------------------------------------------------------------------
// D_NAKED
//   Omits the compiler-generated function prologue and epilogue (no
//   stack frame setup, register saves, or return sequence).  The
//   function body must be written entirely in inline assembly.
//
//   Resolution order:
//     1. GCC / Clang — __attribute__((naked)).
//     2. MSVC — __declspec(naked)  (x86 only).
//     3. No-op fallback.
// -----------------------------------------------------------------------------
#ifndef D_NAKED
    #if defined(D_INTERNAL_GCC_COMPAT_)
        #define D_NAKED __attribute__((naked))
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_NAKED __declspec(naked)
    #else
        #define D_NAKED
    #endif
#endif  // D_NAKED


// Clean up internal helper.
#ifdef D_INTERNAL_GCC_COMPAT_
    #undef D_INTERNAL_GCC_COMPAT_
#endif


#endif  // DJINTERP_ENV_VENDOR_ATTRIBUTES_
