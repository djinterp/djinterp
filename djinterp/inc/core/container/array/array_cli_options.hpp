/******************************************************************************
* djinterp [container]                                 array_cli_options.hpp
*
* Array-specific CLI option parsing and configuration module.
*   Specializes the generic container_cli_traits.hpp infrastructure
* for contiguous array containers, exploiting data() pointer access
* for positional CLI argument mapping, bulk argument application,
* and contiguous option table generation.
*
*   Where container_cli_traits.hpp detects CLI pipeline capabilities
* generically, this module provides array-optimized paths that
* leverage the natural correspondence between positional CLI
* arguments and array indices.
*
* ARRAY-SPECIFIC OPTIMIZATIONS:
*   - Positional argument mapping: CLI positional arguments map
*     directly to array indices via data()[i], enabling O(1)
*     argument binding without key lookup
*   - Bulk argument application: apply argc/argv to a contiguous
*     option array in a single data()-based sweep
*   - Contiguous option table generation: build a CLI option
*     table by scanning data() for named entries
*   - Index-keyed parsing: parse "index=value" strings directly
*     into data()[index] without intermediate key resolution
*   - Help text generation: generate aligned help text by
*     scanning contiguous option entries for documentation
*     members
*
* DESIGN:
*   Two layers following the array CRTP pattern:
*
*   array_cli_options_base<D>  — read-only CLI operations
*     CLI option table generation, help text production,
*     option validation, positional argument queries.
*
*   array_cli_options_mutable<D>  — mutating CLI operations
*     Parse positional arguments, parse key=value pairs,
*     parse argc/argv, apply CLI configuration.
*
*   Free functions provide non-member interfaces for use with
*   any contiguous container without CRTP inheritance.
*
* DEPENDENCIES:
*   array_container.hpp            — array CRTP base, chunk_ref
*   array_options.hpp              — option CRTP bases
*   array_container_traits.hpp     — is_contiguous_array_v, etc.
*   container_cli_traits.hpp       — CLI trait detection
*   container_option_traits.hpp    — option trait detection
*
* TABLE OF CONTENTS
* =================
* I.      array_cli_options_base (CRTP) — read-only
* II.     array_cli_options_mutable (CRTP) — mutating
* III.    Free-Function Positional Parsing
* IV.     Free-Function Key=Value Parsing
* V.      Free-Function argc/argv Parsing
* VI.     Free-Function Help Text Generation
*
*
* path:      \inc\container\array_cli_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.29
******************************************************************************/

#ifndef DJINTERP_ARRAY_CLI_OPTIONS_
#define DJINTERP_ARRAY_CLI_OPTIONS_ 1

#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "..\djinterp.hpp"
#include "array_container.hpp"
#include "array_options.hpp"
#include "meta\array_container_traits.hpp"
#include "meta\container_cli_traits.hpp"
#include "meta\container_option_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   array_cli_options_base (CRTP) — read-only
// =============================================================================
// CLI query and export operations.  All methods operate via
// data() + size() and do not modify the container.
//
// These provide array-optimized paths for generating help
// text, validating CLI arguments, and querying the option
// table layout.

template<typename _Derived>
class array_cli_options_base
{
protected:
    array_cli_options_base()  = default;
    ~array_cli_options_base() = default;

private:
    const _Derived& self() const
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- positional argument queries ---

    // positional_count
    //   returns the number of positional argument slots
    // available (== array size).
    std::size_t
    positional_count() const noexcept
    {
        return self().size();
    }

    // positional_at
    //   O(1) access to the positional argument entry
    // at _index.
    const value_type&
    positional_at(std::size_t _index) const
    {
        return self().data()[_index];
    }

    // --- option table generation ---

    // cli_option_entries
    //   returns a vector of (index, key) pairs describing
    // the positional option layout.  SFINAE-gated on
    // value_type having a .key member.
    template<typename V = value_type>
    auto
    cli_option_entries() const
        -> decltype(
            std::declval<const V&>().key,
            std::vector<std::pair<
                std::size_t,
                decltype(std::declval<V>().key)>>())
    {
        using K = decltype(std::declval<V>().key);

        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::pair<std::size_t, K>>
            result;

        result.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            result.push_back({ i, p[i].key });
        }

        return result;
    }

    // --- help text generation ---

    // help_lines
    //   generates a vector of help text lines, one per
    // option entry.  Each line contains the index,
    // key name, and description (if available).
    //
    // SFINAE-gated on value_type having .key and
    // .description members.
    template<typename V = value_type>
    auto
    help_lines() const
        -> decltype(
            std::declval<const V&>().key,
            std::declval<const V&>().description,
            std::vector<std::string>())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::string> result;

        result.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            std::string line;

            line += "[";
            line += std::to_string(i);
            line += "] ";
            line += p[i].key;
            line += " — ";
            line += p[i].description;

            result.push_back(std::move(line));
        }

        return result;
    }

    // help_lines (key-only variant)
    //   for entries without .description, generates
    // index + key lines only.
    template<typename V = value_type>
    auto
    help_keys() const
        -> decltype(
            std::declval<const V&>().key,
            std::vector<std::string>())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::string> result;

        result.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            std::string line;

            line += "[";
            line += std::to_string(i);
            line += "] ";
            line += p[i].key;

            result.push_back(std::move(line));
        }

        return result;
    }

    // --- validation ---

    // validate_positional_index
    //   true if _index is within bounds.
    bool
    validate_positional_index(
        std::size_t _index) const noexcept
    {
        return (_index < self().size());
    }

    // validate_key
    //   true if _key matches any entry.
    template<typename _Key>
    bool
    validate_key(const _Key& _key) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                return true;
            }
        }

        return false;
    }

    // configured_count
    //   returns the number of entries that have values
    // set.  SFINAE-gated on has_value().
    template<typename V = value_type>
    auto
    configured_count() const
        -> decltype(
            std::declval<const V&>().has_value(),
            std::size_t())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].has_value())
            {
                ++c;
            }
        }

        return c;
    }

    // is_fully_configured
    //   true if every entry has a value set.
    template<typename V = value_type>
    auto
    is_fully_configured() const
        -> decltype(
            std::declval<const V&>().has_value(),
            bool())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (!p[i].has_value())
            {
                return false;
            }
        }

        return true;
    }

    // --- export ---

    // to_cli_strings
    //   exports the option array as a vector of
    // "key=value" strings suitable for CLI output.
    // SFINAE-gated on value_type having .key and
    // .to_string() members.
    template<typename V = value_type>
    auto
    to_cli_strings() const
        -> decltype(
            std::declval<const V&>().key,
            std::declval<const V&>().to_string(),
            std::vector<std::string>())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::string> result;

        result.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            std::string entry;

            entry += p[i].key;
            entry += "=";
            entry += p[i].to_string();

            result.push_back(std::move(entry));
        }

        return result;
    }
};


// =============================================================================
// II.  array_cli_options_mutable (CRTP) — mutating
// =============================================================================
// CLI parsing and configuration operations that modify
// the container's option entries.

template<typename _Derived>
class array_cli_options_mutable
    : public array_cli_options_base<_Derived>
{
protected:
    array_cli_options_mutable()  = default;
    ~array_cli_options_mutable() = default;

private:
    _Derived& self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived& self() const
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- positional argument parsing ---

    // parse_positional
    //   assigns _value to the entry at _index.
    // Returns true if _index is in bounds.
    template<typename _Value>
    bool
    parse_positional(
        std::size_t  _index,
        _Value&&     _val)
    {
        if (_index >= self().size())
        {
            return false;
        }

        self().data()[_index].value =
            std::forward<_Value>(_val);

        return true;
    }

    // parse_positional_args
    //   assigns sequential values from an array of
    // C-strings to consecutive entries starting at
    // _start_index.  Each string is converted via
    // the entry's .from_string() method.
    // Returns count of entries successfully parsed.
    //
    // SFINAE-gated on value_type having .from_string().
    template<typename V = value_type>
    auto
    parse_positional_args(
        const char* const* _args,
        std::size_t        _count,
        std::size_t        _start_index = 0)
        -> decltype(
            std::declval<V&>().from_string(
                std::declval<const std::string&>()),
            std::size_t())
    {
        value_type* p  = self().data();
        std::size_t sz = self().size();

        std::size_t parsed = 0;

        for (std::size_t i = 0; i < _count; ++i)
        {
            std::size_t idx = _start_index + i;

            if (idx >= sz)
            {
                break;
            }

            if (_args[i])
            {
                p[idx].from_string(
                    std::string(_args[i]));
                ++parsed;
            }
        }

        return parsed;
    }

    // --- key=value parsing ---

    // parse_kv_string
    //   parses a single "key=value" string.  Finds the
    // entry matching the key and sets its value via
    // .from_string().  Returns true if matched.
    //
    // SFINAE-gated on value_type having .key and
    // .from_string().
    template<typename V = value_type>
    auto
    parse_kv_string(
        const std::string& _kv)
        -> decltype(
            std::declval<const V&>().key,
            std::declval<V&>().from_string(
                std::declval<const std::string&>()),
            bool())
    {
        std::size_t eq_pos = _kv.find('=');

        if (eq_pos == std::string::npos)
        {
            return false;
        }

        std::string key =
            _kv.substr(0, eq_pos);
        std::string val =
            _kv.substr(eq_pos + 1);

        value_type* p = self().data();
        std::size_t n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == key)
            {
                p[i].from_string(val);
                return true;
            }
        }

        return false;
    }

    // parse_kv_strings
    //   parses multiple "key=value" strings.  Returns
    // count of entries successfully matched.
    template<typename V = value_type>
    auto
    parse_kv_strings(
        const std::vector<std::string>& _kvs)
        -> decltype(
            std::declval<const V&>().key,
            std::declval<V&>().from_string(
                std::declval<const std::string&>()),
            std::size_t())
    {
        std::size_t matched = 0;

        for (const auto& kv : _kvs)
        {
            if (parse_kv_string(kv))
            {
                ++matched;
            }
        }

        return matched;
    }

    // --- argc/argv parsing ---

    // parse_args
    //   parses argc/argv, interpreting arguments as
    // either positional values or "key=value" pairs.
    //
    // Behavior:
    //   - arguments containing '=' are treated as
    //     key=value pairs
    //   - arguments without '=' are treated as
    //     positional, assigned to consecutive indices
    //
    // Returns a pair: (positional_count, kv_count).
    //
    // SFINAE-gated on value_type having .key and
    // .from_string().
    template<typename V = value_type>
    auto
    parse_args(
        int                _argc,
        const char* const* _argv,
        int                _skip = 1)
        -> decltype(
            std::declval<const V&>().key,
            std::declval<V&>().from_string(
                std::declval<const std::string&>()),
            std::pair<std::size_t, std::size_t>())
    {
        value_type* p  = self().data();
        std::size_t sz = self().size();

        std::size_t pos_idx  = 0;
        std::size_t pos_cnt  = 0;
        std::size_t kv_cnt   = 0;

        for (int ai = _skip; ai < _argc; ++ai)
        {
            if (!_argv[ai])
            {
                continue;
            }

            std::string arg(_argv[ai]);

            std::size_t eq_pos = arg.find('=');

            if (eq_pos != std::string::npos)
            {
                // key=value pair
                std::string key =
                    arg.substr(0, eq_pos);
                std::string val =
                    arg.substr(eq_pos + 1);

                for (std::size_t i = 0;
                     i < sz; ++i)
                {
                    if (p[i].key == key)
                    {
                        p[i].from_string(val);
                        ++kv_cnt;
                        break;
                    }
                }
            }
            else
            {
                // positional argument
                if (pos_idx < sz)
                {
                    p[pos_idx].from_string(arg);
                    ++pos_idx;
                    ++pos_cnt;
                }
            }
        }

        return { pos_cnt, kv_cnt };
    }

    // --- flag-based parsing ---

    // parse_flag
    //   enables a boolean-like option entry by key.
    // Requires value_type to have .key and .set_flag()
    // or bool-assignable .value.
    // Returns true if matched.
    template<typename _Key>
    bool
    parse_flag(const _Key& _key)
    {
        value_type* p = self().data();
        std::size_t n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                p[i].value = true;
                return true;
            }
        }

        return false;
    }

    // parse_flags
    //   enables multiple flag entries.  Returns count
    // of flags matched.
    template<typename _KeyIter>
    std::size_t
    parse_flags(
        _KeyIter _begin,
        _KeyIter _end)
    {
        std::size_t matched = 0;

        for (auto it = _begin; it != _end; ++it)
        {
            if (parse_flag(*it))
            {
                ++matched;
            }
        }

        return matched;
    }
};


// =============================================================================
// III. Free-Function Positional Parsing
// =============================================================================
// Non-member positional argument parsing for contiguous
// option arrays.

// array_parse_positional
//   assigns _val to the entry at _index in _dst.
// Returns true if _index is in bounds.
template<typename _Container,
         typename _Value>
inline typename std::enable_if<
    traits::is_contiguous_array_v<_Container>,
    bool
>::type
array_parse_positional(
    _Container&  _dst,
    std::size_t  _index,
    _Value&&     _val)
{
    if (_index >= _dst.size())
    {
        return false;
    }

    _dst.data()[_index].value =
        std::forward<_Value>(_val);

    return true;
}

// array_parse_positional_strings
//   assigns sequential C-strings to consecutive entries
// starting at _start.  Calls .from_string() on each.
// Returns count of entries parsed.
template<typename _Container>
inline auto
array_parse_positional_strings(
    _Container&        _dst,
    const char* const* _args,
    std::size_t        _count,
    std::size_t        _start = 0)
    -> decltype(
        std::declval<
            typename _Container::value_type&>()
                .from_string(
                    std::declval<
                        const std::string&>()),
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::size_t
        >::type())
{
    using V = typename _Container::value_type;

    V*          p  = _dst.data();
    std::size_t sz = _dst.size();
    std::size_t parsed = 0;

    for (std::size_t i = 0; i < _count; ++i)
    {
        std::size_t idx = _start + i;

        if (idx >= sz)
        {
            break;
        }

        if (_args[i])
        {
            p[idx].from_string(
                std::string(_args[i]));
            ++parsed;
        }
    }

    return parsed;
}


// =============================================================================
// IV.  Free-Function Key=Value Parsing
// =============================================================================
// Non-member key=value parsing for contiguous option arrays.

// array_parse_kv_string
//   parses a single "key=value" string into _dst.
// Returns true if a matching key was found.
template<typename _Container>
inline auto
array_parse_kv_string(
    _Container&        _dst,
    const std::string& _kv)
    -> decltype(
        std::declval<
            const typename _Container::value_type&>()
                .key,
        std::declval<
            typename _Container::value_type&>()
                .from_string(
                    std::declval<
                        const std::string&>()),
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            bool
        >::type())
{
    using V = typename _Container::value_type;

    std::size_t eq_pos = _kv.find('=');

    if (eq_pos == std::string::npos)
    {
        return false;
    }

    std::string key = _kv.substr(0, eq_pos);
    std::string val = _kv.substr(eq_pos + 1);

    V*          p = _dst.data();
    std::size_t n = _dst.size();

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].key == key)
        {
            p[i].from_string(val);
            return true;
        }
    }

    return false;
}

// array_parse_kv_strings
//   parses multiple "key=value" strings.  Returns
// count of matched entries.
template<typename _Container>
inline auto
array_parse_kv_strings(
    _Container&                     _dst,
    const std::vector<std::string>& _kvs)
    -> decltype(
        array_parse_kv_string(
            _dst,
            std::declval<const std::string&>()),
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::size_t
        >::type())
{
    std::size_t matched = 0;

    for (const auto& kv : _kvs)
    {
        if (array_parse_kv_string(_dst, kv))
        {
            ++matched;
        }
    }

    return matched;
}


// =============================================================================
// V.   Free-Function argc/argv Parsing
// =============================================================================
// Non-member argc/argv parsing combining positional and
// key=value interpretation.

// array_parse_args
//   parses argc/argv into a contiguous option array.
// Arguments with '=' are key=value; others are
// positional.  Returns (positional_count, kv_count).
template<typename _Container>
inline auto
array_parse_args(
    _Container&        _dst,
    int                _argc,
    const char* const* _argv,
    int                _skip = 1)
    -> decltype(
        std::declval<
            const typename _Container::value_type&>()
                .key,
        std::declval<
            typename _Container::value_type&>()
                .from_string(
                    std::declval<
                        const std::string&>()),
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::pair<std::size_t, std::size_t>
        >::type())
{
    using V = typename _Container::value_type;

    V*          p  = _dst.data();
    std::size_t sz = _dst.size();

    std::size_t pos_idx = 0;
    std::size_t pos_cnt = 0;
    std::size_t kv_cnt  = 0;

    for (int ai = _skip; ai < _argc; ++ai)
    {
        if (!_argv[ai])
        {
            continue;
        }

        std::string arg(_argv[ai]);

        std::size_t eq_pos = arg.find('=');

        if (eq_pos != std::string::npos)
        {
            std::string key =
                arg.substr(0, eq_pos);
            std::string val =
                arg.substr(eq_pos + 1);

            for (std::size_t i = 0;
                 i < sz; ++i)
            {
                if (p[i].key == key)
                {
                    p[i].from_string(val);
                    ++kv_cnt;
                    break;
                }
            }
        }
        else
        {
            if (pos_idx < sz)
            {
                p[pos_idx].from_string(arg);
                ++pos_idx;
                ++pos_cnt;
            }
        }
    }

    return { pos_cnt, kv_cnt };
}


// =============================================================================
// VI.  Free-Function Help Text Generation
// =============================================================================
// Non-member help text generation for contiguous option
// arrays.

// array_cli_help_string
//   generates a formatted help string from a contiguous
// option array.  Each entry is listed with its index,
// key, and description (if available).
//
// SFINAE-gated on value_type having .key and
// .description.
template<typename _Container>
inline auto
array_cli_help_string(
    const _Container&  _src,
    const std::string& _header = "")
    -> decltype(
        std::declval<
            const typename _Container::value_type&>()
                .key,
        std::declval<
            const typename _Container::value_type&>()
                .description,
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::string
        >::type())
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::string result;

    if (!_header.empty())
    {
        result += _header;
        result += "\n\n";
    }

    for (std::size_t i = 0; i < n; ++i)
    {
        result += "  [";
        result += std::to_string(i);
        result += "] ";
        result += p[i].key;
        result += "\n";
        result += "      ";
        result += p[i].description;
        result += "\n";
    }

    return result;
}

// array_cli_help_string (key-only variant)
//   for option entries without .description.
template<typename _Container>
inline auto
array_cli_help_keys_string(
    const _Container&  _src,
    const std::string& _header = "")
    -> decltype(
        std::declval<
            const typename _Container::value_type&>()
                .key,
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::string
        >::type())
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::string result;

    if (!_header.empty())
    {
        result += _header;
        result += "\n\n";
    }

    for (std::size_t i = 0; i < n; ++i)
    {
        result += "  [";
        result += std::to_string(i);
        result += "] ";
        result += p[i].key;
        result += "\n";
    }

    return result;
}

// array_cli_export_strings
//   exports the configured option entries as a vector
// of "key=value" strings.  Only includes entries with
// values set.
template<typename _Container>
inline auto
array_cli_export_strings(
    const _Container& _src)
    -> decltype(
        std::declval<
            const typename _Container::value_type&>()
                .key,
        std::declval<
            const typename _Container::value_type&>()
                .has_value(),
        std::declval<
            const typename _Container::value_type&>()
                .to_string(),
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::vector<std::string>
        >::type())
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::vector<std::string> result;

    result.reserve(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].has_value())
        {
            std::string entry;

            entry += p[i].key;
            entry += "=";
            entry += p[i].to_string();

            result.push_back(std::move(entry));
        }
    }

    return result;
}

// array_cli_validate_args
//   validates that all provided keys exist in the
// option array.  Returns a vector of unrecognized
// key strings.
template<typename _Container>
inline auto
array_cli_validate_args(
    const _Container&               _src,
    const std::vector<std::string>& _keys)
    -> decltype(
        std::declval<
            const typename _Container::value_type&>()
                .key,
        typename std::enable_if<
            traits::is_contiguous_array_v<
                _Container>,
            std::vector<std::string>
        >::type())
{
    using V = typename _Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::vector<std::string> unrecognized;

    for (const auto& key : _keys)
    {
        bool found = false;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == key)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            unrecognized.push_back(key);
        }
    }

    return unrecognized;
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARRAY_CLI_OPTIONS_
