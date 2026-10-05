/*******************************************************************************
* djinterp [core]                                                    kv_pair.hpp
*
*
* path:      /inc/djinterp/core/meta/kv_pair.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.23
*                                                            revised: 2026.10.01
*******************************************************************************/

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
#include "../../djinterp.hpp"


NS_DJINTERP


// =============================================================================
// I.   kv_pair
// =============================================================================

// kv_pair
//   struct: a minimal key-value pair satisfying the
// is_option_entry structural contract (.key + .value).
// Usable as the value_type of an option_set, as a CLI
// string table entry, or as a building block for richer
// option entry types.
template<typename Key,
         typename Value>
struct kv_pair
{
    using key_type   = Key;
    using value_type = Value;

    Key   m_key;
    Value m_value;

    // default construction
    kv_pair() = default;

    // construct from key + value
    constexpr kv_pair(const Key&   _k,
                      const Value& _v)
        : m_key(_k),
          m_value(_v)
    {}

    // construct from moved key + value
    constexpr kv_pair(Key&&   _k,
                      Value&& _v)
        : m_key(static_cast<Key&&>(_k)),
          m_value(static_cast<Value&&>(_v))
    {}

    // equality (compares key only)
    constexpr bool
    operator==(const kv_pair& _other) const
    {
        return (m_key == _other.m_key);
    }

    constexpr bool
    operator!=(const kv_pair& _other) const
    {
        return (m_key != _other.m_key);
    }

    // ordering (by key, for sorted option_sets)
    constexpr bool
    operator<(const kv_pair& _other) const
    {
        return (m_key < _other.m_key);
    }
};

// make_kv
//   function: constructs a kv_pair with deduced types.
template<typename Key,
         typename Value>
D_CONSTEXPR kv_pair<typename std::decay<Key>::type,
                    typename std::decay<Value>::type>
make_kv(
    Key&& _k,
    Value&& _v
)
{
    return kv_pair<typename std::decay<Key>::type,
                   typename std::decay<Value>::type>(
                       static_cast<Key&&>(_k),
                       static_cast<Value&&>(_v) );
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_KV_PAIR_HPP
