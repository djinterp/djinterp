/***********************************************************************
* restd                                              integer_sequence.hpp
*
* compile-time integer pack:
*   integer_sequence<T, Is...> wraps a parameter pack of compile-time
* integer values. Most commonly used (via the index_sequence alias and
* the make_index_sequence factory) to expand a tuple's element list
* across a function call.
*
*   Members:
*     value_type     -- the integer type T
*     static size()  -- sizeof...(Is)
*
*   Family in this file:
*     integer_sequence<T, Is...>   -- the class template
*     index_sequence<Is...>        -- alias for integer_sequence<size_t, Is...>
*
*   The factory generators (make_integer_sequence, make_index_sequence,
*   index_sequence_for) live in make_integer_sequence.hpp.
*
*   STANDARD STATUS:
*   Introduced in C++14. Requires variadic templates and (for
* index_sequence) alias templates. Both are C++11+ features.
*
*
* path:      /inc/restd/utility/integer_sequence.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_INTEGER_SEQUENCE_
#define RESTD_UTILITY_INTEGER_SEQUENCE_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

#include <cstddef>  // std::size_t

NS_RESTD

// =============================================================================
// INTEGER_SEQUENCE
// =============================================================================

// integer_sequence
//   class: holds a compile-time pack of integer values of type _Type.
//   The size() function is constexpr-eligible from C++11 (single
//   return statement of a sizeof... expression).
template<typename _Type, _Type... _Values>
struct integer_sequence
{
    typedef _Type value_type;

    // size: number of values in the pack.
    static D_CONSTEXPR std::size_t size() noexcept
    {
        return sizeof...(_Values);
    }
};

// =============================================================================
// INDEX_SEQUENCE
// =============================================================================

// index_sequence
//   alias: integer_sequence specialised on std::size_t. Available
//   only when alias templates are; the generators in
//   make_integer_sequence.hpp are gated the same way.
#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<std::size_t... _Indices>
    using index_sequence = integer_sequence<std::size_t, _Indices...>;

#endif

NS_END  // restd

#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

#endif  // RESTD_UTILITY_INTEGER_SEQUENCE_
