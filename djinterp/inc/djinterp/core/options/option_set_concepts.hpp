/******************************************************************************
* djinterp [options]                                      option_set_concepts.hpp
*
*   C++20 concept analogs of the option_set trait machinery.
*
*   Mirrors how option_concepts.hpp parallels option_traits.hpp.  Concepts
* here speak about the SAME shapes as the corresponding traits, just in
* requires-clause form.
*
*   Notably:
*     - keyed_c is the concept counterpart of is_keyed_v (the open-world
*       contract option_set enforces on every entry).  Useful for
*       constraining your own helpers against the same shape.
*     - option_set_c parallels is_option_set_v.
*     - option_set_contains_c is parameterized over an NTTP key, so
*       callers can write requires clauses like:
*         requires option_set_contains_c<MySet, my_enum::foo>
*
*
* path:      /inc/djinterp/core/options/option_set_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    keyed_c                              (the open-world option contract)
II.   option_set_c                         (option_set<> detection)
III.  option_set_contains_c                (NTTP-parameterized presence)
IV.   option_set_findable_c                (NTTP-parameterized findability)
V.    option_set_nonempty_c                (composite shape)
*/

#ifndef DJINTERP_OPTION_SET_CONCEPTS_
#define DJINTERP_OPTION_SET_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./option_set.hpp"
#include "./option_set_traits.hpp"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "option_set_concepts.hpp requires C++20 or later"
#endif

#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "option_set_concepts.hpp requires compiler support for concepts"
#endif


NS_DJINTERP


// ===========================================================================
// I.   keyed_c
// ===========================================================================

// keyed_c
//   concept: satisfied iff _Type exposes the keyed contract -
// nested ::key_type alias and static ::key member.  Parallels
// is_keyed_v.
//
//   This is the entire shape option_set requires of any type it
// accepts (after expansion).  Constrain your own helpers against
// it to compose cleanly with the open-world contract.
template<typename _Type>
concept keyed_c = requires
{
    typename _Type::key_type;
    _Type::key;
};


// ===========================================================================
// II.  option_set_c
// ===========================================================================

// option_set_c
//   concept: satisfied iff _Type is some option_set<...>
// specialization.  Parallels is_option_set_v.
template<typename _Type>
concept option_set_c = is_option_set_v<_Type>;


// ===========================================================================
// III. option_set_contains_c
// ===========================================================================

// option_set_contains_c
//   concept: satisfied iff _Set is an option_set that contains
// the key _Key.  Parameterized over the NTTP key for use in
// requires-clauses.  Parallels option_set_contains_v.
//
// Example:
//   template<typename _Set>
//     requires option_set_contains_c<_Set, cli::verbose>
//   void enable_verbosity();
template<typename _Set, auto _Key>
concept option_set_contains_c =
    option_set_c<_Set> &&
    requires
    {
        requires option_set_contains_v<_Set, _Key>;
    };


// ===========================================================================
// IV.  option_set_findable_c
// ===========================================================================

// option_set_findable_c
//   concept: satisfied iff _Set is an option_set and the find
// trait reports `found` for _Key.  Functionally identical to
// option_set_contains_c, but speaks in find vocabulary - useful
// where a downstream constraint wants a paired find_t<> alias to
// be meaningful.
template<typename _Set, auto _Key>
concept option_set_findable_c =
    option_set_c<_Set> &&
    requires
    {
        requires option_set_find<_Set, _Key>::found;
    };


// ===========================================================================
// V.   option_set_nonempty_c
// ===========================================================================

// option_set_nonempty_c
//   concept: satisfied iff _Set is a non-empty option_set.  Pairs
// naturally with option_set_key_type_t (which requires
// non-emptiness for its single-key-type extraction).
template<typename _Set>
concept option_set_nonempty_c =
    option_set_c<_Set> &&
    requires
    {
        requires (_Set::size > 0);
    };


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_CONCEPTS_
