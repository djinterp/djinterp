/***********************************************************************
* restd                                         make_integer_sequence.hpp
*
* integer_sequence generator and helpers:
*   Three aliases that build integer_sequence values:
*
*     make_integer_sequence<T, N>   -- integer_sequence<T, 0, 1, ..., N-1>
*     make_index_sequence<N>        -- make_integer_sequence<size_t, N>
*     index_sequence_for<Ts...>     -- make_index_sequence<sizeof...(Ts)>
*
*   IMPLEMENTATION:
*   When the compiler provides __make_integer_seq (Clang) or
* __integer_pack (GCC 8+), the generator is O(1) instantiations and
* the resulting recursion-free build is dramatically faster on large
* tuples. Otherwise we fall back to log-N recursive concatenation
* (the libstdc++ technique), which is still much better than the
* naive linear recursion.
*
*   STANDARD STATUS:
*   C++14. Requires alias templates (the generators are aliases) and
* variadic templates. Both are C++11+, but alias templates are gated
* by D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES because the std spec
* forms the generators as alias templates and we mirror that exactly.
*
*
* path:      /inc/djinterp/re_std/utility/make_integer_sequence.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_MAKE_INTEGER_SEQUENCE_
#define RESTD_UTILITY_MAKE_INTEGER_SEQUENCE_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES \
    && D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

#include <cstddef>  // std::size_t
#include "../utility/integer_sequence.hpp"

// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_MAKE_INTEGER_SEQ
//   constant: 1 if the compiler provides a builtin that produces an
//   integer_sequence in a single instantiation. Recognises Clang's
//   __make_integer_seq and GCC's __integer_pack.
#ifndef D_RESTD_HAS_MAKE_INTEGER_SEQ
    #if defined(__has_builtin)
        #if __has_builtin(__make_integer_seq)
            #define D_RESTD_HAS_MAKE_INTEGER_SEQ 1
        #elif __has_builtin(__integer_pack)
            #define D_RESTD_HAS_MAKE_INTEGER_SEQ 1
        #else
            #define D_RESTD_HAS_MAKE_INTEGER_SEQ 0
        #endif
    #elif defined(D_ENV_COMPILER_GCC) \
        && D_ENV_COMPILER_VERSION_AT_LEAST(8, 0, 0)
        #define D_RESTD_HAS_MAKE_INTEGER_SEQ 1
    #else
        #define D_RESTD_HAS_MAKE_INTEGER_SEQ 0
    #endif
#endif


NS_RESTD

// =============================================================================
// MAKE_INTEGER_SEQUENCE -- intrinsic-backed when available
// =============================================================================

#if D_RESTD_HAS_MAKE_INTEGER_SEQ

    #if defined(__has_builtin) && __has_builtin(__make_integer_seq)

        // Clang form: __make_integer_seq<integer_sequence, T, N> directly
        // produces an integer_sequence<T, 0, ..., N-1>.
        template<typename _Type, _Type _Count>
        using make_integer_sequence
            = __make_integer_seq<integer_sequence, _Type, _Count>;

    #else

        // GCC form: __integer_pack(N) is a pack-expansion macro that
        // expands to 0, 1, ..., N-1 inside a parameter list.
        template<typename _Type, _Type _Count>
        using make_integer_sequence
            = integer_sequence<_Type, __integer_pack(_Count)...>;

    #endif

#else  // recursive fallback

    NS_INTERNAL

        // make_int_seq_concat_
        //   trait: concatenates two integer_sequences. The right-hand
        //   sequence has each of its values bumped by _Offset before
        //   joining, so make_int_seq_concat_<seq<0,1>, seq<0,1,2>, 2>
        //   yields seq<0,1,2,3,4>.
        template<typename _Lhs, typename _Rhs, typename _Lhs::value_type _Offset>
        struct make_int_seq_concat_;

        template<typename _Type,
                 _Type... _LhsVals,
                 _Type... _RhsVals,
                 _Type _Offset>
        struct make_int_seq_concat_<
            integer_sequence<_Type, _LhsVals...>,
            integer_sequence<_Type, _RhsVals...>,
            _Offset>
        {
            typedef integer_sequence<
                _Type, _LhsVals..., (_RhsVals + _Offset)... > type;
        };

        // make_int_seq_helper_
        //   trait: log-N recursive halving. The size-N sequence is
        //   built from two ~N/2 sequences. Bottom cases at N=0, N=1.
        template<typename _Type, _Type _Count>
        struct make_int_seq_helper_
        {
            typedef typename make_int_seq_concat_<
                typename make_int_seq_helper_<_Type, _Count / 2>::type,
                typename make_int_seq_helper_<_Type, _Count - _Count / 2>::type,
                _Count / 2 >::type type;
        };

        template<typename _Type>
        struct make_int_seq_helper_<_Type, 0>
        {
            typedef integer_sequence<_Type> type;
        };

        template<typename _Type>
        struct make_int_seq_helper_<_Type, 1>
        {
            typedef integer_sequence<_Type, 0> type;
        };

    NS_END  // internal

    template<typename _Type, _Type _Count>
    using make_integer_sequence
        = typename internal::make_int_seq_helper_<_Type, _Count>::type;

#endif  // intrinsic-backed vs recursive

// =============================================================================
// MAKE_INDEX_SEQUENCE / INDEX_SEQUENCE_FOR
// =============================================================================

// make_index_sequence
//   alias: shorthand for make_integer_sequence specialised on size_t.
template<std::size_t _Count>
using make_index_sequence = make_integer_sequence<std::size_t, _Count>;

// index_sequence_for
//   alias: an index_sequence whose length matches a parameter pack.
//   Idiomatic in destructuring code: function templates often take
//   an index_sequence_for<Args...> to enumerate Args by index.
template<typename... _Types>
using index_sequence_for = make_index_sequence<sizeof...(_Types)>;

NS_END  // restd

#endif  // variadic && alias templates

#endif  // RESTD_UTILITY_MAKE_INTEGER_SEQUENCE_
