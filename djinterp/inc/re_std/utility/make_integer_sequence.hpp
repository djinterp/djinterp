/*******************************************************************************
* djinterp [re_std]                                    make_integer_sequence.hpp
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
* by RE_STD_LANG_HAS_ALIAS_TEMPLATES because the std spec
* forms the generators as alias templates and we mirror that exactly.
*
*
* path:      /inc/re_std/utility/make_integer_sequence.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef RE_STD_UTILITY_MAKE_INTEGER_SEQUENCE_HPP
#define RE_STD_UTILITY_MAKE_INTEGER_SEQUENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES \
    && RE_STD_LANG_HAS_ALIAS_TEMPLATES

#include <cstddef>  // std::size_t
#include "integer_sequence.hpp"

// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// RE_STD_HAS_MAKE_INTEGER_SEQ
//   constant: 1 if the compiler provides a builtin that produces an
//   integer_sequence in a single instantiation. Recognises Clang's
//   __make_integer_seq and GCC's __integer_pack.
#ifndef RE_STD_HAS_MAKE_INTEGER_SEQ
    #if defined(__has_builtin)
        #if __has_builtin(__make_integer_seq)
            #define RE_STD_HAS_MAKE_INTEGER_SEQ 1
        #elif __has_builtin(__integer_pack)
            #define RE_STD_HAS_MAKE_INTEGER_SEQ 1
        #else
            #define RE_STD_HAS_MAKE_INTEGER_SEQ 0
        #endif
    #elif defined(RE_STD_COMPILER_GCC) \
        && RE_STD_COMPILER_VERSION_AT_LEAST(8, 0, 0)
        #define RE_STD_HAS_MAKE_INTEGER_SEQ 1
    #else
        #define RE_STD_HAS_MAKE_INTEGER_SEQ 0
    #endif
#endif


namespace re_std
{

// =============================================================================
// MAKE_INTEGER_SEQUENCE -- intrinsic-backed when available
// =============================================================================

#if RE_STD_HAS_MAKE_INTEGER_SEQ

    #if defined(__has_builtin) && __has_builtin(__make_integer_seq)

        // Clang form: __make_integer_seq<integer_sequence, T, N> directly
        // produces an integer_sequence<T, 0, ..., N-1>.
        template<typename Type, Type Count>
        using make_integer_sequence
            = __make_integer_seq<integer_sequence, Type, Count>;

    #else

        // GCC form: __integer_pack(N) is a pack-expansion macro that
        // expands to 0, 1, ..., N-1 inside a parameter list.
        template<typename Type, Type Count>
        using make_integer_sequence
            = integer_sequence<Type, __integer_pack(Count)...>;

    #endif

#else  // recursive fallback

    namespace internal
    {

        // The recursion runs on std::size_t, which depends on no template
        // parameter, and the values take the requested type once, at the
        // end. A partial specialization may not fix a non-type argument
        // whose type is one of its own parameters (C++11 [temp.class.spec]
        // paragraph 8), which is what <Type, 0> and <Type, 1> did: both
        // compilers refused them from C++11 on. On std::size_t the bottom
        // cases are full specializations.

        // make_int_seq_concat_
        //   trait: concatenates two index sequences. The right-hand
        //   sequence has each of its values bumped by Offset before
        //   joining, so make_int_seq_concat_<seq<0,1>, seq<0,1,2>, 2>
        //   yields seq<0,1,2,3,4>.
        template<typename    Lhs,
                 typename    Rhs,
                 std::size_t Offset>
        struct make_int_seq_concat_;

        template<std::size_t... LhsVals,
                 std::size_t... RhsVals,
                 std::size_t    Offset>
        struct make_int_seq_concat_<
            integer_sequence<std::size_t, LhsVals...>,
            integer_sequence<std::size_t, RhsVals...>,
            Offset>
        {
            typedef integer_sequence<
                std::size_t, LhsVals..., (RhsVals + Offset)... > type;
        };

        // make_int_seq_helper_
        //   trait: log-N recursive halving. The size-N sequence is
        //   built from two ~N/2 sequences. Bottom cases at N=0, N=1.
        template<std::size_t Count>
        struct make_int_seq_helper_
        {
            typedef typename make_int_seq_concat_<
                typename make_int_seq_helper_<Count / 2>::type,
                typename make_int_seq_helper_<Count - Count / 2>::type,
                Count / 2 >::type type;
        };

        template<>
        struct make_int_seq_helper_<0>
        {
            typedef integer_sequence<std::size_t> type;
        };

        template<>
        struct make_int_seq_helper_<1>
        {
            typedef integer_sequence<std::size_t, 0> type;
        };

        // make_int_seq_cast_
        //   trait: an index sequence's values as an integer_sequence of
        //   Type, each converted once.
        template<typename Type,
                 typename Seq>
        struct make_int_seq_cast_;

        template<typename       Type,
                 std::size_t... Vals>
        struct make_int_seq_cast_<Type,
                                  integer_sequence<std::size_t, Vals...> >
        {
            typedef integer_sequence<Type, static_cast<Type>(Vals)...> type;
        };

    }  // internal

    template<typename Type, Type Count>
    using make_integer_sequence
        = typename internal::make_int_seq_cast_<
              Type,
              typename internal::make_int_seq_helper_<
                  static_cast<std::size_t>(Count)>::type>::type;

#endif  // intrinsic-backed vs recursive

// =============================================================================
// MAKE_INDEX_SEQUENCE / INDEX_SEQUENCE_FOR
// =============================================================================

// make_index_sequence
//   alias: shorthand for make_integer_sequence specialised on size_t.
template<std::size_t Count>
using make_index_sequence = make_integer_sequence<std::size_t, Count>;

// index_sequence_for
//   alias: an index_sequence whose length matches a parameter pack.
//   Idiomatic in destructuring code: function templates often take
//   an index_sequence_for<Args...> to enumerate Args by index.
template<typename... Types>
using index_sequence_for = make_index_sequence<sizeof...(Types)>;

}  // re_std

#endif  // variadic && alias templates

#endif  // RE_STD_UTILITY_MAKE_INTEGER_SEQUENCE_HPP
