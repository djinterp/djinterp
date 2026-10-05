/*******************************************************************************
* djinterp [core]                                          array_option_view.hpp
*
* Array-specific option configuration module.
*   Specializes the generic container_option_traits.hpp infrastructure
* for contiguous array containers, exploiting data() pointer access
* for positional option lookup, bulk option application, and
* contiguous option entry iteration.
*
*   Where container_option_traits.hpp detects capabilities generically,
* this module provides array-optimized paths that operate entirely
* through data() + size(), leveraging random-access indexing and
* contiguous storage for zero-overhead option management.
*
* ARRAY-SPECIFIC OPTIMIZATIONS:
*   - Positional option access: O(1) lookup by index into a
*     contiguous option entry array via data()
*   - Bulk option application: apply an option_set to all
*     elements in a single data()-based sweep
*   - Contiguous option scan: find/count options matching a
*     key or predicate without iterator indirection
*   - Option range extraction: return a chunk_ref view of
*     a contiguous sub-range of option entries
*   - Subarray configuration: apply options to a slice
*     [offset, offset+count) without copying
*   - Constexpr option queries: compile-time option lookup
*     and validation over data()
*
* DESIGN:
*   Three layers following the array CRTP pattern:
*
*   array_options_constexpr_base<D>  - constexpr layer
*     Compile-time option predicates over data():
*     constexpr_find_option, constexpr_has_option,
*     constexpr_count_options, constexpr_all_configured.
*
*   array_options_immutable_base<D>  - read-only runtime layer
*     Option lookup, export, and query without modification:
*     option_at, find_option, find_option_index,
*     count_options, option_range, to_option_vector,
*     has_option, options_match.
*
*   array_options_mutable_base<D>  - mutating layer
*     In-place option modification and bulk application:
*     set_option_at, apply_option_range,
*     bulk_set_options, clear_options,
*     remove_option_at, configure_from.
*
*   Free functions provide non-member interfaces for use with
*   any contiguous container without CRTP inheritance.
*
* DEPENDENCIES:
*   array_iterator.hpp             - chunk_ref
*   array_traits.hpp               - is_contiguous_array_v
*
*
* path:      /inc/djinterp/core/container/array/array_option_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.29
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    array_options_constexpr_base (CRTP)
      -----------------------------------

II.   array_options_immutable_base (CRTP)
      -----------------------------------

III.  array_options_mutable_base (CRTP)
      ---------------------------------

IV.   free-function option Lookup
      ---------------------------

V.    free-function bulk option application
      -------------------------------------

VI.   free-function option export
      ---------------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_ARRAY_OPTION_VIEW_HPP
#define DJINTERP_CONTAINER_ARRAY_ARRAY_OPTION_VIEW_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "./array_iterator.hpp"    // chunk_ref
#include "./array_traits.hpp"      // is_contiguous_array_v


NS_DJINTERP

// =============================================================================
// I.   array_options_constexpr_base (CRTP) - constexpr layer
// =============================================================================
// Compile-time option predicates.  All methods operate via
// data() + size() and are fully constexpr.
//
// Requires the element type to be an option entry (has
// .key member) for key-based queries.  Non-option-entry
// containers compile cleanly - key-based methods are
// SFINAE-gated.

template<typename Derived>
class array_options_constexpr_base
{
protected:
    constexpr array_options_constexpr_base()  = default;
    ~array_options_constexpr_base() = default;

private:
    constexpr const Derived& self() const noexcept
    {
        return static_cast<
            const Derived&>(*this);
    }

    using value_type =
        typename Derived::value_type;

public:
    // --- constexpr option queries ---

    // constexpr_find_option
    //   returns index of first element whose .key matches _key, or size() if
    // not found.
    template<typename Key>
    constexpr std::size_t
    constexpr_find_option(
        const Key& _key) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                return i;
            }
        }

        return n;
    }

    // constexpr_has_option
    //   true if any element's .key matches _key.
    template<typename Key>
    constexpr bool
    constexpr_has_option(
        const Key& _key) const noexcept
    {
        return (constexpr_find_option(_key) !=
                self().size());
    }

    // constexpr_count_options
    //   counts elements whose .key matches _key.
    template<typename Key>
    constexpr std::size_t
    constexpr_count_options(
        const Key& _key) const noexcept
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                ++c;
            }
        }

        return c;
    }

    // constexpr_all_configured
    //   true if every element has a non-default value. Requires value_type to
    // have an .is_set() or .has_value() method.
    template<typename V = value_type>
    constexpr auto
    constexpr_all_configured() const noexcept
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

    // constexpr_find_unconfigured
    //   returns index of first element without a value, or size() if all are
    // configured.
    template<typename V = value_type>
    constexpr auto
    constexpr_find_unconfigured() const noexcept
        -> decltype(
            std::declval<const V&>().has_value(),
            std::size_t())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (!p[i].has_value())
            {
                return i;
            }
        }

        return n;
    }

    // constexpr_option_keys_unique
    //   true if no two elements share the same .key. O(n²) - suitable for
    // small compile-time arrays.
    template<typename V = value_type>
    constexpr auto
    constexpr_option_keys_unique() const noexcept
        -> decltype(
            std::declval<const V&>().key,
            bool())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            for (std::size_t j = i + 1; j < n; ++j)
            {
                if (p[i].key == p[j].key)
                {
                    return false;
                }
            }
        }

        return true;
    }
};


// =============================================================================
// II.  array_options_immutable_base (CRTP) - runtime immutable
// =============================================================================
// Read-only option operations that query, look up, and export
// option data.  No modification of the source.

template<typename Derived>
class array_options_immutable_base
    : public array_options_constexpr_base<Derived>
{
protected:
    array_options_immutable_base()  = default;
    ~array_options_immutable_base() = default;

private:
    const Derived& self() const
    {
        return static_cast<
            const Derived&>(*this);
    }

    using value_type =
        typename Derived::value_type;

public:
    // --- positional option access ---

    // option_at
    //   O(1) direct access to the option entry at _index.
    const value_type&
    option_at(std::size_t _index) const
    {
        return self().data()[_index];
    }

    // --- key-based lookup ---

    // find_option
    //   returns a pointer to the first entry matching _key, or nullptr if not
    // found.
    template<typename Key>
    const value_type*
    find_option(const Key& _key) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                return &p[i];
            }
        }

        return nullptr;
    }

    // find_option_index
    //   returns index of the first entry matching _key, or size() if not
    // found.
    template<typename Key>
    std::size_t
    find_option_index(const Key& _key) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                return i;
            }
        }

        return n;
    }

    // has_option
    //   true if any entry matches _key.
    template<typename Key>
    bool
    has_option(const Key& _key) const
    {
        return (find_option(_key) != nullptr);
    }

    // count_options
    //   counts entries matching _key.
    template<typename Key>
    std::size_t
    count_options(const Key& _key) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();
        std::size_t       c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                ++c;
            }
        }

        return c;
    }

    // --- predicate-based lookup ---

    // find_option_if
    //   returns a pointer to the first entry satisfying _predicate, or
    // nullptr.
    template<typename Predicate>
    const value_type*
    find_option_if(Predicate _predicate) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_predicate(p[i]))
            {
                return &p[i];
            }
        }

        return nullptr;
    }

    // find_option_if_index
    //   returns index of first entry satisfying _predicate, or size().
    template<typename Predicate>
    std::size_t
    find_option_if_index(Predicate _predicate) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (_predicate(p[i]))
            {
                return i;
            }
        }

        return n;
    }

    // --- range extraction ---

    // option_range
    //   returns a non-owning view of [_offset, _offset + _count) option
    // entries.
    chunk_ref<value_type>
    option_range(
        std::size_t _offset,
        std::size_t _count) const noexcept
    {
        std::size_t sz = self().size();

        std::size_t actual_offset =
            (_offset < sz) ? _offset : sz;

        std::size_t remaining =
            sz - actual_offset;

        std::size_t actual_count =
            (_count < remaining)
                ? _count : remaining;

        return {
            self().data() + actual_offset,
            actual_count
        };
    }

    // --- export ---

    // to_option_vector
    //   copies all option entries into a std::vector.
    std::vector<value_type>
    to_option_vector() const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        return std::vector<value_type>(p, p + n);
    }

    // configured_option_indices
    //   returns indices of all entries that have values set. SFINAE-gated on
    // has_value().
    template<typename V = value_type>
    auto
    configured_option_indices() const
        -> decltype(
            std::declval<const V&>().has_value(),
            std::vector<std::size_t>())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::size_t> result;

        result.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].has_value())
            {
                result.push_back(i);
            }
        }

        return result;
    }

    // unconfigured_option_indices
    //   returns indices of all entries without values.
    template<typename V = value_type>
    auto
    unconfigured_option_indices() const
        -> decltype(
            std::declval<const V&>().has_value(),
            std::vector<std::size_t>())
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        std::vector<std::size_t> result;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (!p[i].has_value())
            {
                result.push_back(i);
            }
        }

        return result;
    }

    // options_match
    //   true if every entry in this array has a matching key and value in
    // _other.
    template<typename Other>
    bool
    options_match(const Other& _other) const
    {
        const value_type* p = self().data();
        std::size_t       n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            const auto* found =
                _other.find_option(p[i].key);

            if (!found)
            {
                return false;
            }

            if (!(found->value == p[i].value))
            {
                return false;
            }
        }

        return true;
    }
};


// =============================================================================
// III. array_options_mutable_base (CRTP) - mutating layer
// =============================================================================
// In-place option modification and bulk application.

template<typename Derived>
class array_options_mutable_base
    : public array_options_immutable_base<Derived>
{
protected:
    array_options_mutable_base()  = default;
    ~array_options_mutable_base() = default;

private:
    Derived& self()
    {
        return static_cast<Derived&>(*this);
    }

    const Derived& self() const
    {
        return static_cast<
            const Derived&>(*this);
    }

    using value_type =
        typename Derived::value_type;

public:
    // --- positional modification ---

    // option_at (mutable)
    //   O(1) direct mutable access by index.
    value_type&
    option_at(std::size_t _index)
    {
        return self().data()[_index];
    }

    // set_option_at
    //   sets the value of the entry at _index.
    template<typename Value>
    void
    set_option_at(
        std::size_t  _index,
        Value&&     _val)
    {
        self().data()[_index].value =
            std::forward<Value>(_val);
    }

    // --- key-based modification ---

    // set_option
    //   finds the first entry matching _key and sets its value. Returns true
    // if found.
    template<typename Key,
             typename Value>
    bool
    set_option(
        const Key&  _key,
        Value&&     _val)
    {
        value_type* p = self().data();
        std::size_t n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                p[i].value =
                    std::forward<Value>(_val);
                return true;
            }
        }

        return false;
    }

    // set_all_options
    //   sets the value of ALL entries matching _key. Returns count of entries
    // modified.
    template<typename Key,
             typename Value>
    std::size_t
    set_all_options(
        const Key&  _key,
        Value&&     _val)
    {
        value_type* p = self().data();
        std::size_t n = self().size();
        std::size_t c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                p[i].value = _val;
                ++c;
            }
        }

        return c;
    }

    // --- bulk application ---

    // apply_option_range
    //   applies _fn to each entry in [_offset, _offset + _count). Returns
    // count of entries visited.
    template<typename Fn>
    std::size_t
    apply_option_range(
        std::size_t _offset,
        std::size_t _count,
        Fn          _fn)
    {
        value_type* p  = self().data();
        std::size_t sz = self().size();

        std::size_t actual_offset =
            (_offset < sz) ? _offset : sz;

        std::size_t remaining =
            sz - actual_offset;

        std::size_t actual_count =
            (_count < remaining)
                ? _count : remaining;

        for (std::size_t i = 0;
             i < actual_count; ++i)
        {
            _fn(p[actual_offset + i]);
        }

        return actual_count;
    }

    // bulk_set_options
    //   applies options from a source array (or any container with data() +
    // size()). For each entry in _src, finds the matching key in this array
    // and sets its value. Returns count of matched entries.
    template<typename Source>
    typename std::enable_if<
        is_contiguous_array_v<Source>,
        std::size_t
    >::type
    bulk_set_options(const Source& _src)
    {
        using src_value = typename Source::value_type;

        const src_value* sp = _src.data();
        std::size_t      sn = _src.size();

        value_type* dp = self().data();
        std::size_t dn = self().size();

        std::size_t matched = 0;

        for (std::size_t si = 0; si < sn; ++si)
        {
            for (std::size_t di = 0;
                 di < dn; ++di)
            {
                if (dp[di].key == sp[si].key)
                {
                    dp[di].value = sp[si].value;
                    ++matched;
                    break;
                }
            }
        }

        return matched;
    }

    // configure_from
    //   applies all entries from _src by matching keys. Unlike
    // bulk_set_options, this accepts any iterable container - not limited to
    // contiguous.
    template<typename Source>
    std::size_t
    configure_from(
        const Source& _src
    )
    {
        value_type* dp = self().data();
        std::size_t dn = self().size();

        std::size_t matched = 0;

        for (const auto& entry : _src)
        {
            for (std::size_t di = 0;
                 di < dn; ++di)
            {
                if (dp[di].key == entry.key)
                {
                    dp[di].value = entry.value;
                    ++matched;
                    break;
                }
            }
        }

        return matched;
    }

    // --- clear / reset ---

    // clear_option
    //   resets the entry at _index to its default- constructed state.
    void
    clear_option(
        std::size_t _index
    )
    {
        self().data()[_index] = value_type{};
    }

    // clear_all_options
    //   resets every entry to default-constructed.
    void
    clear_all_options()
    {
        value_type* p = self().data();
        std::size_t n = self().size();

        for (std::size_t i = 0; i < n; ++i)
        {
            p[i] = value_type{};
        }
    }

    // clear_options_by_key
    //   resets all entries matching _key. Returns count of entries cleared.
    template<typename Key>
    std::size_t
    clear_options_by_key(
        const Key& _key
    )
    {
        value_type* p = self().data();
        std::size_t n = self().size();
        std::size_t c = 0;

        for (std::size_t i = 0; i < n; ++i)
        {
            if (p[i].key == _key)
            {
                p[i] = value_type{};
                ++c;
            }
        }

        return c;
    }
};


// =============================================================================
// IV.  Free-Function Option Lookup
// =============================================================================
// Non-member option lookup for any contiguous array
// whose value_type is an option entry.

// array_find_option
//   returns pointer to first entry matching _key, or nullptr. O(n) scan via
// data().
template<typename Container,
         typename Key>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    const typename Container::value_type*
>::type
array_find_option(
    const Container& _src,
    const Key&       _key)
{
    using V = typename Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].key == _key)
        {
            return &p[i];
        }
    }

    return nullptr;
}

// array_find_option_index
//   returns index of first entry matching _key, or size().
template<typename Container,
         typename Key>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    std::size_t
>::type
array_find_option_index(
    const Container& _src,
    const Key&       _key)
{
    using V = typename Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].key == _key)
        {
            return i;
        }
    }

    return n;
}

// array_has_option
template<typename Container,
         typename Key>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    bool
>::type
array_has_option(
    const Container& _src,
    const Key&       _key)
{
    return (array_find_option(_src, _key) !=
            nullptr);
}

// array_count_options
template<typename Container,
         typename Key>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    std::size_t
>::type
array_count_options(
    const Container& _src,
    const Key&       _key)
{
    using V = typename Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();
    std::size_t c = 0;

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].key == _key)
        {
            ++c;
        }
    }

    return c;
}

// array_option_keys
//   extracts a vector of all .key values from a contiguous option entry array.
template<typename Container>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    std::vector<decltype(
        std::declval<
            typename Container::value_type>().key)>
>::type
array_option_keys(const Container& _src)
{
    using V = typename Container::value_type;
    using K = decltype(std::declval<V>().key);

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::vector<K> result;

    result.reserve(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        result.push_back(p[i].key);
    }

    return result;
}


// =============================================================================
// V.   Free-Function Bulk Option Application
// =============================================================================
// Non-member bulk option operations on contiguous arrays.

// array_set_option
//   sets the first entry matching _key. Returns true if found and set.
template<typename Container,
         typename Key,
         typename Value>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    bool
>::type
array_set_option(
    Container&  _dst,
    const Key&  _key,
    Value&&     _val)
{
    using V = typename Container::value_type;

    V*          p = _dst.data();
    std::size_t n = _dst.size();

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].key == _key)
        {
            p[i].value =
                std::forward<Value>(_val);
            return true;
        }
    }

    return false;
}

// array_bulk_set_options
//   applies all entries from _src into _dst by key matching. Returns count of
// matched entries.
template<typename Dst,
         typename Src>
inline typename std::enable_if<
    ( is_contiguous_array_v<Dst> &&
      is_contiguous_array_v<Src> ),
    std::size_t
>::type
array_bulk_set_options(
    Dst&       _dst,
    const Src& _src)
{
    using DV = typename Dst::value_type;
    using SV = typename Src::value_type;

    DV*         dp = _dst.data();
    std::size_t dn = _dst.size();

    const SV*   sp = _src.data();
    std::size_t sn = _src.size();

    std::size_t matched = 0;

    for (std::size_t si = 0; si < sn; ++si)
    {
        for (std::size_t di = 0;
             di < dn; ++di)
        {
            if (dp[di].key == sp[si].key)
            {
                dp[di].value = sp[si].value;
                ++matched;
                break;
            }
        }
    }

    return matched;
}

// array_apply_option_fn
//   applies _fn to each entry in the contiguous option array. Returns count of
// entries visited.
template<typename Container,
         typename Fn>
inline typename std::enable_if<
    is_contiguous_array_v<Container>,
    std::size_t
>::type
array_apply_option_fn(
    Container& _dst,
    Fn          _fn)
{
    using V = typename Container::value_type;

    V*          p = _dst.data();
    std::size_t n = _dst.size();

    for (std::size_t i = 0; i < n; ++i)
    {
        _fn(p[i]);
    }

    return n;
}

// array_clear_all_options
//   resets every entry to default-constructed.
template<typename Container>
inline typename std::enable_if<
    is_contiguous_array_v<Container>
>::type
array_clear_all_options(Container& _dst)
{
    using V = typename Container::value_type;

    V*          p = _dst.data();
    std::size_t n = _dst.size();

    for (std::size_t i = 0; i < n; ++i)
    {
        p[i] = V{};
    }
}


// =============================================================================
// VI.  Free-Function Option Export
// =============================================================================
// Non-member option export for contiguous arrays.

// array_option_diff
//   returns a vector of entries that differ between _a and _b (by key match +
// value comparison). Entries present in _a but not in _b are included. Entries
// in _a whose value differs from _b are included.
template<typename A,
         typename B>
inline typename std::enable_if<
    ( is_contiguous_array_v<A> &&
      is_contiguous_array_v<B> ),
    std::vector<typename A::value_type>
>::type
array_option_diff(
    const A& _a,
    const B& _b)
{
    using VA = typename A::value_type;
    using VB = typename B::value_type;

    const VA*   ap = _a.data();
    std::size_t an = _a.size();

    const VB*   bp = _b.data();
    std::size_t bn = _b.size();

    std::vector<VA> result;

    for (std::size_t ai = 0; ai < an; ++ai)
    {
        bool found_match = false;

        for (std::size_t bi = 0; bi < bn; ++bi)
        {
            if (bp[bi].key == ap[ai].key)
            {
                if (!(bp[bi].value ==
                      ap[ai].value))
                {
                    result.push_back(ap[ai]);
                }

                found_match = true;
                break;
            }
        }

        if (!found_match)
        {
            result.push_back(ap[ai]);
        }
    }

    return result;
}

// array_configured_options
//   returns a vector of entries that have values set.
template<typename Container>
inline auto
array_configured_options(
    const Container& _src
)
    -> decltype(std::declval<
            const typename Container::value_type&>().has_value(),
            typename std::enable_if<is_contiguous_array_v<Container>,
            std::vector<
                typename Container::value_type>
        >::type())
{
    using V = typename Container::value_type;

    const V*    p = _src.data();
    std::size_t n = _src.size();

    std::vector<V> result;

    result.reserve(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        if (p[i].has_value())
        {
            result.push_back(p[i]);
        }
    }

    return result;
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_ARRAY_OPTION_VIEW_HPP
