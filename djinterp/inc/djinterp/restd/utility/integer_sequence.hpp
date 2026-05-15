/***********************************************************************
* restd                                              integer_sequence.hpp
*
* compile-time sequence of integers:
*   restd::integer_sequence<T, Ints...> packs a fixed list of integer
* values into a type, used as a parameter pack for unpacking with
* `Ints...` in a function template.
*
* aliases provided:
*   index_sequence<Ns...>          = integer_sequence<size_t, Ns...>
*   make_integer_sequence<T, N>    = integer_sequence<T, 0, 1, ..., N-1>
*   make_index_sequence<N>         = make_integer_sequence<size_t, N>
*   index_sequence_for<Ts...>      = make_index_sequence<sizeof...(Ts)>
*
* canonical use: unpacking a tuple-like into a pack-expansion. With
* I... bound to the make_index_sequence:
*   template<size_t... I>
*   void apply_impl(std::index_sequence<I...>, Tuple& t) {
*     f(get<I>(t)...);
*   }
*
* compile-time complexity:
*   The naive recursive expansion of make_integer_sequence is O(N)
*   instantiation depth and blows the compiler at the lowest of
*   ~512 (gcc default), ~900 (clang), or N+epsilon (MSVC). To avoid
*   that, prefer the compiler builtin:
*
*     - __make_integer_seq<integer_sequence, T, N>     (Clang, MSVC)
*     - __integer_pack(N)                              (GCC, Intel)
*
*   restd uses the builtin path when available (gated on
*   D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC). The recursive fallback
*   still works but should be considered a last resort; consumers
*   needing big sequences on a fallback compiler should split work.
*
* added in std C++14.
*
*
* path:      /inc/restd/utility/integer_sequence.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_INTEGER_SEQUENCE_
#define RESTD_UTILITY_INTEGER_SEQUENCE_ 1

#include "djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

    #include <cstddef>


// ----------------------------------------------------------------------
// D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC
//   1 if a compiler builtin can synthesise a 0..N-1 integer pack in
//   O(1) instantiation depth. Two flavours wired up here:
//
//     __make_integer_seq<TmplName, T, N> -- Clang and MSVC. Yields
//        TmplName<T, 0, 1, ..., N-1> as the type.
//
//     __integer_pack(N) -- GCC (and Intel via GCC compatibility).
//        Yields the pack 0, 1, ..., N-1 expanded inline.
//
//   Detection is deliberately conservative: only set to 1 when one
//   of these is known to work. Override by predefining the macro
//   before including this header.
// ----------------------------------------------------------------------
#ifndef D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC
    #if defined(__clang__) || defined(_MSC_VER)
        // Clang has __make_integer_seq since Clang 5.0; MSVC since 19.16.
        // Both are stable on the versions restd targets.
        #define D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC  1
        #define D_RESTD_INTEGER_SEQ_FLAVOUR_CLANG_MSVC  1
    #elif defined(__GNUC__) || defined(__INTEL_COMPILER)
        // GCC's __integer_pack since GCC 8 (in C++14 mode and later).
        #define D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC  1
        #define D_RESTD_INTEGER_SEQ_FLAVOUR_GCC         1
    #else
        #define D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC  0
    #endif
#endif


namespace restd
{

// =====================================================================
// integer_sequence -- the type itself
// =====================================================================

template<typename _T, _T... _Ints>
struct integer_sequence
{
    typedef _T value_type;

    static D_CONSTEXPR std::size_t size() D_NOEXCEPT
    {
        return sizeof...(_Ints);
    }
};


// =====================================================================
// index_sequence -- the size_t specialisation alias
// =====================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<std::size_t... _Ns>
    using index_sequence = integer_sequence<std::size_t, _Ns...>;

#endif


// =====================================================================
// make_integer_sequence
// =====================================================================
//
// Two paths: builtin (preferred) and recursive (fallback).
//
// The builtin paths use type aliases when available, and a class
// trait yielding ::type otherwise, so that downstream code can
// always say `typename make_integer_sequence<T,N>::type` if it needs
// to. Both paths produce identical types from the user's perspective.

#if D_RESTD_HAS_MAKE_INTEGER_SEQ_INTRINSIC

    #if defined(D_RESTD_INTEGER_SEQ_FLAVOUR_CLANG_MSVC)

        // Clang/MSVC: __make_integer_seq<Tmpl, T, N>
        // is itself integer_sequence<T, 0, 1, ..., N-1>.
      #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename _T, _T _N>
        using make_integer_sequence = __make_integer_seq<integer_sequence, _T, _N>;
      #else
        template<typename _T, _T _N>
        struct make_integer_sequence_helper
        {
            typedef __make_integer_seq<integer_sequence, _T, _N> type;
        };
      #endif

    #elif defined(D_RESTD_INTEGER_SEQ_FLAVOUR_GCC)

        // GCC: __integer_pack(N) expands to the pack 0..N-1 inline.
      #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename _T, _T _N>
        using make_integer_sequence =
            integer_sequence<_T, __integer_pack(_N)...>;
      #else
        template<typename _T, _T _N>
        struct make_integer_sequence_helper
        {
            typedef integer_sequence<_T, __integer_pack(_N)...> type;
        };
      #endif

    #endif

#else

    // Recursive fallback. O(N) instantiation depth. Caps out around
    // 512 on GCC (with -ftemplate-depth bumped, more is possible)
    // and 900 on Clang. Documented limit; fallback compilers should
    // ship something newer.
    namespace internal
    {
        // Build [I... , Tail] one step at a time. Terminates when the
        // counter hits zero.
        template<typename _T, std::size_t _N, _T... _Built>
        struct iseq_builder
            : iseq_builder<_T, _N - 1, static_cast<_T>(_N - 1), _Built...>
        {
        };

        template<typename _T, _T... _Built>
        struct iseq_builder<_T, 0, _Built...>
        {
            typedef integer_sequence<_T, _Built...> type;
        };
    }

  #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
    template<typename _T, _T _N>
    using make_integer_sequence
        = typename internal::iseq_builder<_T, static_cast<std::size_t>(_N)>::type;
  #else
    template<typename _T, _T _N>
    struct make_integer_sequence_helper
    {
        typedef typename internal::iseq_builder<_T,
                                                 static_cast<std::size_t>(_N)
                                                >::type type;
    };
  #endif

#endif


// =====================================================================
// make_index_sequence and index_sequence_for
// =====================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<std::size_t _N>
    using make_index_sequence = make_integer_sequence<std::size_t, _N>;

    template<typename... _Ts>
    using index_sequence_for = make_index_sequence<sizeof...(_Ts)>;

#endif


}  // namespace restd

#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

#endif  // RESTD_UTILITY_INTEGER_SEQUENCE_
