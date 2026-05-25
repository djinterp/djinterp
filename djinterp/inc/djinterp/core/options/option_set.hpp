/******************************************************************************
* djinterp [options]                                            option_set.hpp
*
*   A compile-time collection of option<...> entries.  Pure type-level
* container: no storage, no methods, no instances needed.  An option_set
* is just a shape that:
*
*     1. requires every entry's key_type to match (so the set as a whole
*        has a single key_type, inferred from the entries),
*     2. requires every key to be unique,
*     3. exposes ::size, ::key_type, and a ::contains<_Key> query.
*
*   The underlying pack walks (uniqueness check, containment query) are
* delegated to the domain-agnostic lookup machinery in
* core/meta/lookup.hpp.  This header layers domain meaning (option,
* option_set) over those primitives without duplicating the search logic.
*
*
* path:      /inc/djinterp/core/options/option_set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    internal helpers
      1. all_same_type
II.   option_set
      1. option_set<>             (empty)
      2. option_set<_F, _Rest...> (non-empty)
*/

#ifndef DJINTERP_OPTION_SET_
#define DJINTERP_OPTION_SET_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/lookup.hpp"
#include "./option.hpp"


NS_DJINTERP


// ===========================================================================
// I.   internal helpers
// ===========================================================================

NS_INTERNAL

    // all_same_type
    //   helper: true if every type in the pack is identical.
    // Used to enforce a uniform key_type across an option_set.
    // (Not in lookup.hpp because that header deals in entries
    // and keys, not raw type uniformity.)
    template<typename...>
    struct all_same_type : std::true_type
    {};

    template<typename _First>
    struct all_same_type<_First> : std::true_type
    {};

    template<typename    _First,
             typename    _Second,
             typename... _Rest>
    struct all_same_type<_First, _Second, _Rest...>
        : std::integral_constant<bool,
            ( std::is_same<_First, _Second>::value &&
              all_same_type<_Second, _Rest...>::value )>
    {};

NS_END  // internal


// ===========================================================================
// II.  option_set
// ===========================================================================

// option_set
//   trait: a compile-time collection of option<...> entries.
// All entries must share a key_type; all keys must be unique.
// No instance data.
template<typename... _Options>
struct option_set;

// option_set<>
//   trait: empty set.  No key_type can be inferred; ::size is
// 0; ::contains<_Key> is always std::false_type.
template<>
struct option_set<>
{
    static constexpr std::size_t size = 0;

    // contains
    //   trait: always false for the empty set.
    template<auto /*_Key*/>
    struct contains : std::false_type
    {};
};

// option_set<_First, _Rest...>
//   trait: non-empty set.  key_type is inferred from the head
// entry; uniformity across the tail and uniqueness across all
// keys are enforced by static_assert.  Containment delegates
// to the generalized contains_key trait from lookup.hpp.
template<typename    _First,
         typename... _Rest>
struct option_set<_First, _Rest...>
{
private:
    // enforce uniform key_type across all entries
    static_assert(
        internal::all_same_type<
            typename _First::key_type,
            typename _Rest::key_type...
        >::value,
        "All entries in an option_set must share the same key_type. "
        "Use a single enum / class / scope for every key in the set.");

    // enforce key uniqueness (delegated to the generalized
    // NTTP-pack predicate from lookup.hpp)
    static_assert(
        value_pack_unique<
            _First::key,
            _Rest::key...
        >::value,
        "All keys in an option_set must be unique.");

public:
    using key_type = typename _First::key_type;

    static constexpr std::size_t size = (sizeof...(_Rest) + 1);

    // contains
    //   trait: true iff _Key appears as the key of some entry.
    // Thin wrapper over lookup's contains_key that adds a
    // key_type sanity check.
    template<auto _Key>
    struct contains
        : std::integral_constant<bool,
            contains_key<_Key, _First, _Rest...>::value>
    {
        static_assert(
            std::is_same<decltype(_Key), key_type>::value,
            "Key passed to option_set::contains<> does not match "
            "the set's key_type.");
    };
};


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_
