/******************************************************************************
* djinterp [options]                                            kv_pair.hpp
*
* Lightweight key-value pair and variadic option_set construction.
*   Provides a minimal kv_pair<K, V> struct that satisfies the
* is_option_entry contract (.key + .value), along with variadic
* factory utilities for constructing option_sets from a sequence of
* pairs.
*
*   kv_pair is the canonical building block for option_set population.
* It is intentionally simple: just the two mandatory fields.  Richer
* option entries (with description, default_value, short_name, etc.)
* can inherit from kv_pair or extend it with additional members while
* remaining structurally compatible with is_option_entry.
*
*   The string_kv alias (kv_pair<const char*, const char*>) serves as
* the canonical type for CLI string tables and option_table() return
* values.
*
* TABLE OF CONTENTS
* =================
* I.      kv_pair
* II.     Common Aliases
* III.    Factory Functions
* IV.     String↔Enum Key Map
* V.      Variadic option_set Construction
*
*
* path:      /inc/options/kv_pair.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_KV_PAIR_
#define DJINTERP_KV_PAIR_ 1

#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include "../djinterp.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


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

    _Key   key;
    _Value value;

    // default construction
    kv_pair() = default;

    // construct from key + value
    constexpr kv_pair(const _Key&   _k,
                      const _Value& _v)
        : key(_k)
        , value(_v)
    {
    }

    // construct from moved key + value
    constexpr kv_pair(_Key&&   _k,
                      _Value&& _v)
        : key(static_cast<_Key&&>(_k))
        , value(static_cast<_Value&&>(_v))
    {
    }

    // equality (compares key only)
    constexpr bool
    operator==(const kv_pair& _other) const
    {
        return (key == _other.key);
    }

    constexpr bool
    operator!=(const kv_pair& _other) const
    {
        return (key != _other.key);
    }

    // ordering (by key, for sorted option_sets)
    constexpr bool
    operator<(const kv_pair& _other) const
    {
        return (key < _other.key);
    }
};


// =============================================================================
// II.  Common Aliases
// =============================================================================

// string_kv
//   type: key-value pair with C-string key and C-string value.
// Canonical type for CLI string tables and option_table()
// return values.
using string_kv = kv_pair<const char*, const char*>;

// std_string_kv
//   type: key-value pair with std::string key and value.
using std_string_kv = kv_pair<std::string, std::string>;

// enum_string_kv
//   type alias template: maps an enum key to a string value.
// Used for enum↔string bidirectional tables.
template<typename _Enum>
using enum_string_kv = kv_pair<_Enum, const char*>;

// string_enum_kv
//   type alias template: maps a string key to an enum value.
// The reverse direction of enum_string_kv.
template<typename _Enum>
using string_enum_kv = kv_pair<const char*, _Enum>;


// =============================================================================
// III. Factory Functions
// =============================================================================

// make_kv
//   function: constructs a kv_pair with deduced types.
template<typename _Key,
         typename _Value>
constexpr kv_pair<
    typename std::decay<_Key>::type,
    typename std::decay<_Value>::type>
make_kv(_Key&& _k, _Value&& _v)
{
    return kv_pair<
        typename std::decay<_Key>::type,
        typename std::decay<_Value>::type>(
            static_cast<_Key&&>(_k),
            static_cast<_Value&&>(_v));
}


// =============================================================================
// IV.  String↔Enum Key Map
// =============================================================================
// A compile-time bidirectional mapping between enum values
// and their string representations.  Used by the CLI layer
// to resolve user-typed strings into typed enum keys, and by
// the options layer to generate help text.
//
// The key_map is an array of enum_string_kv<E> entries,
// wrapped in a struct that provides lookup methods.

// key_map
//   struct: compile-time bidirectional enum↔string mapping.
// _Enum is the key enum type.  _N is the number of entries.
template<typename _Enum,
         std::size_t _N>
struct key_map
{
    using enum_type  = _Enum;
    using entry_type = enum_string_kv<_Enum>;

    static constexpr std::size_t count = _N;

    std::array<entry_type, _N> entries;

    // from_string
    //   returns the enum value matching the given string
    // name, or the provided fallback if not found.
    constexpr _Enum
    from_string
    (
        const char* _name,
        _Enum       _fallback
    ) const
    {
        for (std::size_t i = 0; i < _N; ++i)
        {
            if (entries[i].value != nullptr &&
                _name != nullptr)
            {
                // runtime string comparison (constexpr
                // in C++14+ with helper)
                const char* a = entries[i].value;
                const char* b = _name;
                bool match = true;

                while (*a && *b)
                {
                    if (*a != *b)
                    {
                        match = false;
                        break;
                    }

                    ++a;
                    ++b;
                }

                if (match && (*a == *b))
                {
                    return entries[i].key;
                }
            }
        }

        return _fallback;
    }

    // to_string
    //   returns the string name for the given enum value,
    // or nullptr if not found.
    constexpr const char*
    to_string(_Enum _val) const
    {
        for (std::size_t i = 0; i < _N; ++i)
        {
            if (entries[i].key == _val)
            {
                return entries[i].value;
            }
        }

        return nullptr;
    }

    // contains_string
    //   returns true if the given string name exists in
    // the map.
    constexpr bool
    contains_string(const char* _name) const
    {
        for (std::size_t i = 0; i < _N; ++i)
        {
            if (entries[i].value != nullptr &&
                _name != nullptr)
            {
                const char* a = entries[i].value;
                const char* b = _name;
                bool match = true;

                while (*a && *b)
                {
                    if (*a != *b)
                    {
                        match = false;
                        break;
                    }

                    ++a;
                    ++b;
                }

                if (match && (*a == *b))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // contains_enum
    //   returns true if the given enum value exists in the
    // map.
    constexpr bool
    contains_enum(_Enum _val) const
    {
        for (std::size_t i = 0; i < _N; ++i)
        {
            if (entries[i].key == _val)
            {
                return true;
            }
        }

        return false;
    }
};

// make_key_map
//   function: constructs a key_map from a braced list of
// enum_string_kv entries.
template<typename _Enum,
         typename... _Pairs>
constexpr auto
make_key_map(_Pairs&&... _pairs)
    -> key_map<_Enum, sizeof...(_Pairs)>
{
    return key_map<_Enum, sizeof...(_Pairs)>{
        {{ static_cast<_Pairs&&>(_pairs)... }}
    };
}


// =============================================================================
// V.   Variadic option_set Construction
// =============================================================================
// Utilities for constructing option_set-compatible arrays or
// vectors from a variadic sequence of kv_pairs.

// make_option_array
//   function: constructs a std::array of kv_pairs from a
// variadic argument list.  All pairs must share the same
// key and value types.
template<typename _Key,
         typename _Value,
         typename... _Pairs>
constexpr auto
make_option_array
(
    kv_pair<_Key, _Value>&& _first,
    _Pairs&&...             _rest
) -> std::array<kv_pair<_Key, _Value>,
                1 + sizeof...(_Pairs)>
{
    return {{
        static_cast<kv_pair<_Key, _Value>&&>(_first),
        static_cast<_Pairs&&>(_rest)...
    }};
}

// make_option_list
//   function: constructs a std::vector of kv_pairs from
// a variadic argument list via initializer_list.
template<typename _Key,
         typename _Value>
inline std::vector<kv_pair<_Key, _Value>>
make_option_list
(
    std::initializer_list<kv_pair<_Key, _Value>> _pairs
)
{
    return std::vector<kv_pair<_Key, _Value>>(_pairs);
}


NS_END  // djinterp


#endif  // DJINTERP_KV_PAIR_
