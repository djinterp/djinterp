/******************************************************************************
* djinterp [options]                                           option_pair.hpp
*
*   A minimal key-value pair satisfying the is_option_entry structural
* contract (.key + .value).  Unlike kv_pair (which uses m_key / m_value),
* option_pair exposes its members directly as .key and .value so that
* SFINAE expression aliases in option_pair_traits.hpp detect them without
* adaptation.
*   By itself an option_pair places no constraints on the types of either
* member - both key and value may be any type.  Constraints on key
* uniformity and uniqueness are imposed by the option_set container.
*   Usable as the value_type of an option_set, as a CLI string table
* entry, or as a building block for richer option entry types that add
* optional columns (description, default_value, bounds, etc.).
*
*
* path:      /inc/djinterp/core/options/option_pair.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.06
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    option_pair
II.   make_option_pair
*/

#ifndef DJINTERP_OPTION_PAIR_
#define DJINTERP_OPTION_PAIR_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
// I.   option_pair
// ===========================================================================

// option_pair
//   struct: a minimal key-value pair satisfying the
// is_option_entry structural contract (.key + .value).
// Both key and value can be any type; no constraints are
// imposed at this level.
//
// Example:
//   option_pair<std::string, int> p{"verbose", 1};
//   option_pair<DMyEnum, bool>    q{DMyEnum::flag, true};
template<typename _Key,
         typename _Value>
struct option_pair
{
    using key_type   = _Key;
    using value_type = _Value;

    _Key   key;
    _Value value;

    // default construction
    option_pair() = default;

    // construct from key + value (lvalue)
    D_CONSTEXPR option_pair(
		const _Key&   _key,
		const _Value& _value
	)
		: key  (_key),
		  value(_value)
    {}

    // construct from key + value (rvalue)
    D_CONSTEXPR option_pair(
		_Key&&   _key,
		_Value&& _value
	)
        : key(  static_cast<_Key&&>  (_key)),
          value(static_cast<_Value&&>(_value))
    {}

    // equality (compares key only)
    //   two option_pairs compare equal when their keys
    // match, regardless of value.  This mirrors the
    // uniqueness invariant of option_set.
    D_CONSTEXPR bool
    operator==(
        const option_pair& _other
    ) const
    {
        return (key == _other.key);
    }

    D_CONSTEXPR bool
    operator!=(
        const option_pair& _other
    ) const
    {
        return (key != _other.key);
    }

    // ordering (by key, for sorted option_sets)
    D_CONSTEXPR bool
    operator<(
        const option_pair& _other
    ) const
    {
        return (key < _other.key);
    }

    D_CONSTEXPR bool
    operator<=(
        const option_pair& _other
    ) const
    {
        return (key <= _other.key);
    }

    D_CONSTEXPR bool
    operator>(
        const option_pair& _other
    ) const
    {
        return (key > _other.key);
    }

    D_CONSTEXPR bool
    operator>=(
        const option_pair& _other
    ) const
    {
        return (key >= _other.key);
    }
};


// ===========================================================================
// II.  make_option_pair
// ===========================================================================

// make_option_pair
//   function: constructs an option_pair with deduced types.
// Decay is applied to both arguments so that array-to-pointer,
// function-to-pointer, and cv-ref stripping occur
// automatically.
//
// Example:
//   auto p = make_option_pair("verbose", 1);
//   // -> option_pair<const char*, int>
template<typename _Key,
         typename _Value>
D_CONSTEXPR option_pair<typename std::decay<_Key>::type,
                        typename std::decay<_Value>::type>
make_option_pair(
    _Key&&   _key,
    _Value&& _value
)
{
    return option_pair<typename std::decay<_Key>::type,
                       typename std::decay<_Value>::type>(
                           static_cast<_Key&&>(_key),
                           static_cast<_Value&&>(_value));
}


NS_END  // djinterp


#endif  // DJINTERP_OPTION_PAIR_