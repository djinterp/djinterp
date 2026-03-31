/******************************************************************************
* djinterp [container]                                           enum_map.hpp
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
* _Backing template parameter, defaulting to
* std::vector<std::pair<const _Enum, _Value>>.  The map overlay base
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
*   _Enum       -- the enumeration type (scoped or unscoped).
*   _Value      -- the mapped value type.
*   _Backing    -- the underlying storage container.
*                   default: std::vector<std::pair<const _Enum, _Value>>.
*   _Compare    -- key comparison function object type.
*                   default: std::less<_Enum>.
*
* DEPENDENCIES:
*   djinterp.hpp                   -- namespace macros, clean_t
*   type_traits.hpp                -- detection idiom
*   container_traits.hpp           -- container classification
*   map.hpp                        -- map_overlay_base, vocabulary types
*   map_traits.hpp                 -- map structural traits
*
* TABLE OF CONTENTS
* =================
* I.      Vocabulary Types
* II.     enum_map Class
* III.    Deduction Guides (C++17)
* IV.     Factory Functions
*
*
* path:      /inc/container/enum_map.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_ENUM_MAP_
#define DJINTERP_ENUM_MAP_ 1

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "./meta/container_traits.hpp"
#include "./meta/map_traits.hpp"
#include "./map.hpp"


NS_DJINTERP
NS_CONTAINER
NS_MAP


// =============================================================================
// I.   Vocabulary Types
// =============================================================================

// enum_entry
//   type: canonical entry type for an enum-keyed map.
// Identical to map_entry<E, V> but provided for clarity
// at the call site.
template<typename _Enum,
         typename _Value>
using enum_entry = map_entry<_Enum, _Value>;


// =============================================================================
// II.  enum_map Class
// =============================================================================

// enum_map
//   class: a map container with an enumeration key type.
// Delegates storage to _Backing and inherits map semantics
// from map_overlay_base via CRTP.
//
// static_assert enforces that _Enum is an enumeration type.
// All map operations (find, insert, erase, contains, at,
// count) are provided by the CRTP base.  This class adds:
//   - owning storage for the backing container,
//   - constructors (default, initializer_list, range, copy, move),
//   - the backing() accessor required by the CRTP contract,
//   - operator[] with default-insertion semantics,
//   - swap.
template<typename _Enum,
         typename _Value,
         typename _Backing = std::vector<
             std::pair<const _Enum, _Value>>,
         typename _Compare = std::less<_Enum>>
class enum_map
    : public map_overlay_base<
          enum_map<_Enum, _Value, _Backing, _Compare>,
          _Enum,
          _Value,
          _Backing,
          _Compare>
{
    static_assert(std::is_enum_v<_Enum>,
                  "Template parameter `_Enum` must be an "
                  "enumeration type.");

private:
    using self_type = enum_map;
    using base_type = map_overlay_base<
        self_type, _Enum, _Value, _Backing, _Compare>;

    friend base_type;

public:
    // --- type aliases (container protocol) ---

    using key_type               = _Enum;
    using mapped_type            = _Value;
    using value_type             = enum_entry<_Enum, _Value>;
    using key_compare            = _Compare;
    using backing_container_type = _Backing;
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
    template<typename _InputIt>
    constexpr enum_map(_InputIt _first,
                       _InputIt _last)
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
    //   accesses or default-inserts the value for _key.
    // If _key does not exist, inserts a default-constructed
    // _Value and returns a reference to it.
    constexpr _Value&
    operator[](const _Enum& _key)
    {
        auto it = this->find(_key);

        // key exists: return reference
        if (it != this->end())
        {
            return it->second;
        }

        // key does not exist: default-insert
        auto result = this->insert(
            value_type(_key, _Value{}));

        return result.iterator->second;
    }

    // --- convenience insert overload ---

    // insert (key, value)
    //   convenience overload that constructs the entry
    // from separate key and value arguments.
    template<typename _V>
    constexpr typename base_type::insert_result
    insert(const _Enum& _key,
           _V&&         _value)
    {
        return base_type::insert(
            value_type(_key,
                       std::forward<_V>(_value)));
    }

    // --- swap ---

    constexpr void
    swap(enum_map& _other) noexcept(
        noexcept(std::declval<_Backing&>().swap(
            std::declval<_Backing&>())))
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

    constexpr _Backing&
    backing() noexcept
    {
        return m_backing;
    }

    constexpr const _Backing&
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
    _Backing m_backing;
};


// =============================================================================
// III. Deduction Guides                     (C++17)
// =============================================================================

// from initializer_list
template<typename _Enum,
         typename _Value>
enum_map(std::initializer_list<
             std::pair<const _Enum, _Value>>)
    -> enum_map<_Enum, _Value>;

// from iterator pair
template<typename _InputIt>
enum_map(_InputIt, _InputIt)
    -> enum_map<
        std::remove_const_t<
            typename std::iterator_traits<
                _InputIt>::value_type::first_type>,
        typename std::iterator_traits<
            _InputIt>::value_type::second_type>;


// =============================================================================
// IV.  Factory Functions
// =============================================================================

// make_enum_map
//   factory: creates an enum_map from an initializer list.
template<typename _Enum,
         typename _Value>
constexpr enum_map<_Enum, _Value>
make_enum_map(
    std::initializer_list<
        std::pair<const _Enum, _Value>> _init)
{
    return enum_map<_Enum, _Value>(_init);
}

// make_enum_map (empty)
//   factory: creates an empty enum_map.
template<typename _Enum,
         typename _Value>
constexpr enum_map<_Enum, _Value>
make_enum_map()
{
    return enum_map<_Enum, _Value>{};
}


NS_END  // map
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ENUM_MAP_
