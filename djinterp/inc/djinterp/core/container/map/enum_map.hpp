/*******************************************************************************
* djinterp [core]                                                   enum_map.hpp
*
* Enum-keyed map container.
*   A map whose key type is constrained to be an enumeration (scoped or
* unscoped).  Values have a fixed, template-parameterized type --
* consequently, an enum_map always has homogeneous values.
*
*   enum_map participates fully in the djinterp container trait system:
*     - container_class<enum_map<E,V>>   classifies along the 12 axes.
*     - map_class<enum_map<E,V>>         classifies map-specific capabilities.
*     - container_cast, views, iterators all work.
*
*   An enum_map is a backed container.  It delegates storage to a
* Backing template parameter, defaulting to
* std::vector<std::pair<const Enum, Value>>.  The map overlay base
* deduces the best strategy at compile time: if the backing is already
* a sorted tree, identity forwarding; if it is a vector, linear scan
* or sorted-flat depending on whether the backing maintains a sorted
* invariant.
*
*   For dense enums (contiguous values starting at 0), a future
* dense_enum_map specialization can use std::array for O(1)
* constant-time lookup by casting the enum to its underlying index.
* This module provides the general sparse-enum case.
*
* ELEMENT TYPES:
*   enum_entry<E, V>   -- type alias for std::pair<const E, V>.
*                          This is the value_type of the container.
*
*   The user may insert elements as:
*     - enum_entry<E, V> directly,
*     - std::pair<const E, V> (identical to above),
*     - via insert(key, value) convenience overload.
*
* TEMPLATE PARAMETERS:
*   Enum        -- the enumeration type (scoped or unscoped).
*   Value       -- the mapped value type.
*   Backing     -- the underlying storage container.
*                   default: std::vector<std::pair<const Enum, Value>>.
*   Compare     -- key comparison function object type.
*                   default: std::less<Enum>.
*
* DEPENDENCIES:
*   djinterp.hpp                   -- namespace macros, clean_t
*   type_traits.hpp                -- detection idiom
*   container_traits.hpp           -- container classification
*   map.hpp                        -- map_overlay_base, vocabulary types
*   map_traits.hpp                 -- map structural traits
*   enum_map_traits.hpp            -- enum-keyed map traits, enum_info
*
*
* path:      /inc/djinterp/core/container/map/enum_map.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.30
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Vocabulary Types
      ----------------

II.   enum_map Class
      --------------

III.  Deduction Guides (C++17)
      ------------------------

IV.   Factory Functions
      -----------------
*/

#ifndef DJINTERP_CONTAINER_MAP_ENUM_MAP_HPP
#define DJINTERP_CONTAINER_MAP_ENUM_MAP_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"
#include "map_traits.hpp"
#include "enum_map_traits.hpp"
#include "./map.hpp"


NS_DJINTERP
NS_CONTAINER
NS_MAP


// =============================================================================
// I.   Vocabulary Types
// =============================================================================

// enum_entry
//   type: canonical entry type for an enum-keyed map. Identical to
// map_entry<E, V> but provided for clarity at the call site.
template<typename Enum,
         typename Value>
using enum_entry = map_entry<Enum, Value>;


// =============================================================================
// II.  enum_map Class
// =============================================================================

// enum_map
//   class: a map container with an enumeration key type.
// Delegates storage to Backing and inherits map semantics from
// map_overlay_base via CRTP. static_assert enforces that Enum is an
// enumeration type. All map operations (find, insert, erase, contains, at,
// count) are provided by the CRTP base.
// This class adds:
//   - owning storage for the backing container,
//   - constructors (default, initializer_list, range, copy, move),
//   - the backing() accessor required by the CRTP contract,
//   - operator[] with default-insertion semantics,
//   - swap.
template<typename Enum,
         typename Value,
         typename Backing = std::vector<
             std::pair<const Enum, Value>>,
         typename Compare = std::less<Enum>>
class enum_map
    : public map_overlay_base<
          enum_map<Enum, Value, Backing, Compare>,
          Enum,
          Value,
          Backing,
          Compare>
{
    static_assert(std::is_enum_v<Enum>,
                  "Template parameter `Enum` must be an "
                  "enumeration type.");

private:
    using self_type = enum_map;
    using base_type = map_overlay_base<
        self_type, Enum, Value, Backing, Compare>;

    friend base_type;

public:
    // --- type aliases (container protocol) ---

    using key_type               = Enum;
    using mapped_type            = Value;
    using value_type             = enum_entry<Enum, Value>;
    using key_compare            = Compare;
    using backing_container_type = Backing;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;

    using iterator       = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;

    // --- constructors ---

    // default
    constexpr enum_map() = default;

    // initializer_list
    constexpr enum_map(
        std::initializer_list<value_type> _init)
    {
        for (const auto& entry : _init)
        {
            this->insert(entry);
        }
    }

    // range
    template<typename InputIt>
    constexpr enum_map(InputIt _first,
                       InputIt _last)
    {
        for (auto it = _first; it != _last; ++it)
        {
            this->insert(*it);
        }
    }

    // copy
    constexpr enum_map(const enum_map&) = default;

    // move
    constexpr enum_map(enum_map&&) = default;

    // copy assignment
    constexpr enum_map&
    operator=(const enum_map&) = default;

    // move assignment
    constexpr enum_map&
    operator=(enum_map&&) = default;

    // initializer_list assignment
    constexpr enum_map&
    operator=(std::initializer_list<value_type> _init)
    {
        this->clear();

        for (const auto& entry : _init)
        {
            this->insert(entry);
        }

        return *this;
    }

    // destructor
    ~enum_map() = default;

    // --- operator[] ---

    // operator[]
    //   accesses or default-inserts the value for _key. If _key does not
    // exist, inserts a default-constructed Value and returns a reference to
    // it.
    constexpr Value&
    operator[](const Enum& _key)
    {
        auto it = this->find(_key);

        // key exists: return reference
        if (it != this->end())
        {
            return it->second;
        }

        // key does not exist: default-insert
        auto result = this->insert(
            value_type(_key, Value{}));

        return result.iterator->second;
    }

    // --- convenience insert overload ---

    // insert (key, value)
    //   convenience overload that constructs the entry from separate key and
    // value arguments.
    template<typename V>
    constexpr typename base_type::insert_result
    insert(const Enum& _key,
           V&&         _value)
    {
        return base_type::insert(
            value_type(_key,
                       std::forward<V>(_value)));
    }

    // --- swap ---

    constexpr void
    swap(enum_map& _other) noexcept(
        noexcept(std::declval<Backing&>().swap(
            std::declval<Backing&>())))
    {
        m_backing.swap(_other.m_backing);

        return;
    }

    friend constexpr void
    swap(enum_map& _a,
         enum_map& _b) noexcept(noexcept(_a.swap(_b)))
    {
        _a.swap(_b);

        return;
    }

    // --- comparison ---

    friend constexpr bool
    operator==(const enum_map& _a,
               const enum_map& _b)
    {
        if (_a.size() != _b.size())
        {
            return false;
        }

        for (const auto& entry : _a)
        {
            auto it = _b.find(entry.first);

            if ( it == _b.end() ||
                 !(it->second == entry.second) )
            {
                return false;
            }
        }

        return true;
    }

    friend constexpr bool
    operator!=(const enum_map& _a,
               const enum_map& _b)
    {
        return !(_a == _b);
    }

    // --- backing access (CRTP contract) ---

    constexpr Backing&
    backing() noexcept
    {
        return m_backing;
    }

    constexpr const Backing&
    backing() const noexcept
    {
        return m_backing;
    }

    // --- max_size ---

    constexpr size_type
    max_size() const noexcept
    {
        return m_backing.max_size();
    }

private:
    Backing m_backing;
};


// =============================================================================
// III. Deduction Guides                     (C++17)
// =============================================================================

// from initializer_list
template<typename Enum,
         typename Value>
enum_map(std::initializer_list<
             std::pair<const Enum, Value>>)
    -> enum_map<Enum, Value>;

// from iterator pair
template<typename InputIt>
enum_map(InputIt, InputIt)
    -> enum_map<
        std::remove_const_t<
            typename std::iterator_traits<
                InputIt>::value_type::first_type>,
        typename std::iterator_traits<
            InputIt>::value_type::second_type>;


// =============================================================================
// IV.  Factory Functions
// =============================================================================

// make_enum_map
//   factory: creates an enum_map from an initializer list.
template<typename Enum,
         typename Value>
constexpr enum_map<Enum, Value>
make_enum_map(
    std::initializer_list<
        std::pair<const Enum, Value>> _init)
{
    return enum_map<Enum, Value>(_init);
}

// make_enum_map (empty)
//   factory: creates an empty enum_map.
template<typename Enum,
         typename Value>
constexpr enum_map<Enum, Value>
make_enum_map()
{
    return enum_map<Enum, Value>{};
}


NS_END  // map
NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_MAP_ENUM_MAP_HPP
