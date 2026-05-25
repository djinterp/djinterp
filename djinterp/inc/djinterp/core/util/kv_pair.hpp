/******************************************************************************
* djinterp [util]                                                  kv_pair.hpp
*
*
*
*
* path:      /inc/djinterp/util/kv_pair.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_KV_PAIR_
#define DJINTERP_KV_PAIR_ 1

#include <type_traits>
#include <utility>
#include "../djinterp.hpp"


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
template<typename _Key,
         typename _Value>
struct kv_pair
{
    using key_type   = _Key;
    using value_type = _Value;

    _Key   m_key;
    _Value m_value;

    // default construction
    kv_pair() = default;

    // construct from key + value
    constexpr kv_pair(const _Key&   _k,
                      const _Value& _v)
        : m_key(_k),
          m_value(_v)
    {}

    // construct from moved key + value
    constexpr kv_pair(_Key&&   _k,
                      _Value&& _v)
        : m_key(static_cast<_Key&&>(_k)),
          m_value(static_cast<_Value&&>(_v))
    {}

    // equality (compares key only)
    constexpr bool
    operator==(const kv_pair& _other) const
    {
        return (m_key == _other.key);
    }

    constexpr bool
    operator!=(const kv_pair& _other) const
    {
        return (m_key != _other.key);
    }

    // ordering (by key, for sorted option_sets)
    constexpr bool
    operator<(const kv_pair& _other) const
    {
        return (m_key < _other.key);
    }
};

// make_kv
//   function: constructs a kv_pair with deduced types.
template<typename _Key,
         typename _Value>
D_CONSTEXPR kv_pair<typename std::decay<_Key>::type,
                    typename std::decay<_Value>::type>
make_kv(
	_Key&& _k,
	_Value&& _v
)
{
    return kv_pair<typename std::decay<_Key>::type,
                   typename std::decay<_Value>::type>(
                       static_cast<_Key&&>(_k),
                       static_cast<_Value&&>(_v) );
}


NS_END  // djinterp


#endif  // DJINTERP_KV_PAIR_