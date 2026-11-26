/*******************************************************************************
* djinterp [cpp][meta]                                               kv_pair.hpp
*
*
* path:      /inc/djinterp/cpp/meta/kv_pair.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.23
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DKvPair
    -------
*/

#ifndef DJINTERP_META_KV_PAIR_HPP
#define DJINTERP_META_KV_PAIR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


//==============================================================================
// 1.  DKvPair
//==============================================================================

// DKvPair
//   struct: a minimal key-value pair satisfying the key/value structural
// contract (.key + .value) that meta/kv.hpp's is_kv_pair detects and
// option/'s entries are read through. Usable as the value_type of an
// option_set, as a CLI string table entry, or as a building block for richer
// option entry types.
//   The members are public and named by the contract, not by the m_ rule,
// as std::pair keeps first and second: a pair is read through them. (They
// were m_key and m_value from 2026.10.04 to 2026.11.11, and in that time
// DKvPair met neither is_kv_pair nor the option readers; the owner ruled for
// the contract's names.)
template<typename Key,
         typename Value>
struct DKvPair
{
    using key_type   = Key;
    using value_type = Value;

    Key   key;
    Value value;

    // default construction
    DKvPair() = default;

    // construct from key + value
    constexpr DKvPair(const Key&   _k,
                      const Value& _v)
        : key(_k),
          value(_v)
    {}

    // construct from moved key + value
    constexpr DKvPair(Key&&   _k,
                      Value&& _v)
        : key(static_cast<Key&&>(_k)),
          value(static_cast<Value&&>(_v))
    {}

    // equality (compares key only)
    constexpr
    bool operator==(
        const DKvPair& _other
    ) const
    {
        return (key == _other.key);
    }

    constexpr
    bool operator!=(
        const DKvPair& _other
    ) const
    {
        return (key != _other.key);
    }

    // ordering (by key, for sorted option_sets)
    constexpr
    bool operator<(
        const DKvPair& _other
    ) const
    {
        return (key < _other.key);
    }
};

// make_kv
//   function: constructs a DKvPair with deduced types.
template<typename Key,
         typename Value>
D_CONSTEXPR DKvPair<typename std::decay<Key>::type,
                    typename std::decay<Value>::type>
make_kv(
    Key&& _k,
    Value&& _v
)
{
    return DKvPair<typename std::decay<Key>::type,
                   typename std::decay<Value>::type>(
                       static_cast<Key&&>(_k),
                       static_cast<Value&&>(_v) );
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_KV_PAIR_HPP
