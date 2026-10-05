/*******************************************************************************
* djinterp [core]                                              option_record.hpp
*
* A runtime option record: string keys, scalar values, and a cascade.
*   option_set (option_set.hpp) is the compile-time face of options: a typed
* schema whose keys and value types are fixed when it is instantiated. Some
* consumers need the dynamic face instead -- a record assembled at run time
* from several modules that do not know each other's keys. uxoxo's element
* attributes and element state are the first such consumer.
*   A record maps keys to a small closed set of scalar values: bool, long,
* double and string. overlay is the cascade: the right-hand record's entries win
* over the left's, key by key. The record is an aggregate, so it can be written
* as a braced list of key/value pairs.
*
*
* path:      /inc/djinterp/core/option/option_record.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  option_value
2.  THE RECORD
    ----------
    1.  option_record
    2.  overlay
*/

#ifndef DJINTERP_OPTION_OPTION_RECORD_HPP
#define DJINTERP_OPTION_OPTION_RECORD_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <map>          // std::map
#include <string>       // std::string
#include <type_traits>  // std::is_integral, std::is_floating_point
#include <utility>      // std::forward
#include <variant>      // std::variant, std::get_if
// djinterp
#include "../../djinterp.hpp"  // framework root


NS_DJINTERP


//==============================================================================
// 1.  VALUES
//==============================================================================


// 1.1    option_value
//------------------------------------------------------------------------------
// 1.1.1
// option_value
//   type: one value in a record. Integers are held as long and reals as double,
// so a record never has to ask which integer width a writer used.
using option_value = std::variant<bool, long, double, std::string>;


//==============================================================================
// 2.  THE RECORD
//==============================================================================


// 2.1    option_record
//------------------------------------------------------------------------------
// 2.1.1
// option_record
//   struct: a string-keyed record of option_values. An aggregate: its one data
// member is public so a record can be brace-initialized from key/value pairs.
// The typed readers return the caller's fallback when the key is absent or
// holds a different type, so reading never throws.
struct option_record
{
    std::map<std::string, option_value> entries;

    // set
    //   modifier: stores _value under _key, replacing any previous value. Every
    // integer type is stored as long and every floating type as double; a
    // string literal is stored as a string, never converted to bool.
    template<typename Value>
    void set(
        const std::string& _key,
        Value&&            _value
    )
    {
        entries[_key] = to_value(std::forward<Value>(_value));

        return;
    }

    // has
    //   accessor: whether _key holds any value.
    D_NODISCARD
    bool has(
        const std::string& _key
    ) const
    {
        return (entries.find(_key) != entries.end());
    }

    // find
    //   accessor: the value under _key, or nullptr when it is absent.
    D_NODISCARD
    const option_value* find(
        const std::string& _key
    ) const
    {
        const auto found = entries.find(_key);

        return (found == entries.end()) ? nullptr
                                        : &found->second;
    }

    // as_bool / as_long / as_double / as_string
    //   accessors: the value under _key as that type, or _fallback when the
    // key is absent or holds another type.
    D_NODISCARD
    bool as_bool(
        const std::string& _key,
        bool               _fallback
    ) const
    {
        return read<bool>(_key, _fallback);
    }

    D_NODISCARD
    long as_long(
        const std::string& _key,
        long               _fallback
    ) const
    {
        return read<long>(_key, _fallback);
    }

    D_NODISCARD
    double as_double(
        const std::string& _key,
        double             _fallback
    ) const
    {
        return read<double>(_key, _fallback);
    }

    D_NODISCARD
    std::string as_string(
        const std::string& _key,
        const std::string& _fallback
    ) const
    {
        return read<std::string>(_key, _fallback);
    }

    // size / empty
    //   accessors: how many keys the record holds.
    D_NODISCARD
    std::size_t size() const
    {
        return entries.size();
    }

    D_NODISCARD
    bool empty() const
    {
        return entries.empty();
    }

private:
    // read
    //   helper: the value under _key if it holds a Type, else _fallback.
    template<typename Type>
    Type read(
        const std::string& _key,
        const Type&        _fallback
    ) const
    {
        const option_value* value = find(_key);

        // absent key or a value of another type both read as the fallback
        if (!value)
        {
            return _fallback;
        }

        const Type* held = std::get_if<Type>(value);

        return (held != nullptr) ? *held
                                 : _fallback;
    }

    // to_value
    //   helpers: map a written value onto the four alternatives explicitly,
    // so overload resolution never picks bool for a pointer or a narrowing
    // conversion for an integer.
    static option_value to_value(bool _value)
    {
        return option_value(_value);
    }

    static option_value to_value(const char* _value)
    {
        return option_value(std::string(_value ? _value : ""));
    }

    static option_value to_value(const std::string& _value)
    {
        return option_value(_value);
    }

    static option_value to_value(std::string&& _value)
    {
        return option_value(std::move(_value));
    }

    static option_value to_value(const option_value& _value)
    {
        return _value;
    }

    template<typename Number>
    static option_value to_value(Number _value)
    {
        static_assert(( std::is_integral<Number>::value ||
                        std::is_floating_point<Number>::value ),
                      "option_record::set: a value must be a bool, a number, "
                      "a string or an option_value");

        if constexpr (std::is_integral<Number>::value)
        {
            return option_value(static_cast<long>(_value));
        }
        else
        {
            return option_value(static_cast<double>(_value));
        }
    }
};


// 2.2    overlay
//------------------------------------------------------------------------------
// 2.2.1
// overlay
//   function: the cascade. A copy of _base with every entry of _over written on
// top, so _over wins wherever both hold a key.
D_NODISCARD
inline option_record overlay(
    const option_record& _base,
    const option_record& _over
)
{
    option_record result = _base;

    // later layers win, key by key
    for (const auto& entry : _over.entries)
    {
        result.entries[entry.first] = entry.second;
    }

    return result;
}


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTION_RECORD_HPP
