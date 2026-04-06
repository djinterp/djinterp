/******************************************************************************
* djinterp [options]                                           option_set.hpp
*
*   A keyed collection of option_pair entries with a uniform key type and
* unique keys.  Values may be homogeneous (a single concrete _Value type)
* or heterogeneous (using compat::any as the value type).
*
*   option_set is parameterized on _Key and _Value.  When _Value is a
* type-erased container such as compat::any, each entry may hold a
* different runtime type — the option_set does not impose value-type
* uniformity.  When _Value is a concrete type (int, std::string, etc.),
* all values share that type and the set is homogeneous.
*
*   Storage is a flat std::vector of option_pair<_Key, _Value>, providing
* cache-friendly iteration and simple implementation.  Lookup is linear
* by default.  For large sets or hot paths, consider a sorted option_set
* with binary search (controlled by the _Sorted template parameter).
*
*   Satisfies the structural contracts detected by option_set_traits.hpp:
* exposes ::key_type, ::mapped_type, ::value_type, and provides find(),
* contains(), size(), empty(), begin(), end(), and data().
*
* TABLE OF CONTENTS
* =================
* I.    option_set
*
*
* path:      /inc/options/option_set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_OPTION_SET_
#define DJINTERP_OPTION_SET_ 1

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <vector>
#include "../djinterp.hpp"
#include "./option_pair.hpp"


NS_DJINTERP


// =============================================================================
// I.   option_set
// =============================================================================

// option_set
//   class: a keyed collection of option_pair entries.
// All keys share the type _Key.  Each key may appear at most
// once.  Values share the type _Value, which may be a
// type-erased container for heterogeneous storage.
//
// When _Sorted is true, the set maintains entries in key
// order and uses binary search for lookup.  When false
// (default), insertion order is preserved and lookup is
// linear.
//
// Example (homogeneous):
//   option_set<std::string, int> opts;
//   opts.insert("width", 800);
//   opts.insert("height", 600);
//
// Example (heterogeneous with compat::any):
//   option_set<std::string, compat::any> opts;
//   opts.insert("name", compat::any(std::string("foo")));
//   opts.insert("count", compat::any(42));
template<typename _Key,
         typename _Value,
         bool     _Sorted = false>
class option_set
{
public:
    using key_type        = _Key;
    using mapped_type     = _Value;
    using value_type      = option_pair<_Key, _Value>;
    using container_type  = std::vector<value_type>;
    using size_type       = typename container_type::size_type;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    option_set() = default;

    // from initializer list
    //   duplicate keys are silently ignored (first wins).
    option_set(
            std::initializer_list<value_type> _init
        )
    {
        m_entries.reserve(_init.size());

        for (const auto& entry : _init)
        {
            // enforce uniqueness
            if (!contains(entry.key))
            {
                m_entries.push_back(entry);
            }
        }

        if (_Sorted)
        {
            sort_entries();
        }
    }

    // -----------------------------------------------------------------
    //  capacity
    // -----------------------------------------------------------------

    // size
    size_type size() const noexcept
    {
        return m_entries.size();
    }

    // empty
    bool empty() const noexcept
    {
        return m_entries.empty();
    }

    // -----------------------------------------------------------------
    //  iteration
    // -----------------------------------------------------------------

    iterator       begin()       noexcept { return m_entries.begin(); }
    const_iterator begin() const noexcept { return m_entries.begin(); }
    iterator       end()         noexcept { return m_entries.end();   }
    const_iterator end()   const noexcept { return m_entries.end();   }

    // data
    const value_type* data() const noexcept
    {
        return m_entries.data();
    }

    value_type* data() noexcept
    {
        return m_entries.data();
    }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    // find
    //   returns an iterator to the entry with the given key,
    // or end() if not found.
    const_iterator
    find(const _Key& _k) const
    {
        if (_Sorted)
        {
            return find_sorted(_k);
        }

        return find_linear(_k);
    }

    iterator
    find(const _Key& _k)
    {
        if (_Sorted)
        {
            auto it = find_sorted_mut(_k);

            return it;
        }

        return find_linear_mut(_k);
    }

    // contains
    //   returns true if the set has an entry with the given key.
    bool
    contains(const _Key& _k) const
    {
        return (find(_k) != end());
    }

    // at
    //   returns a const reference to the value for the given
    // key.  Undefined behaviour if the key is absent.
    const _Value&
    at(const _Key& _k) const
    {
        auto it = find(_k);

        return it->value;
    }

    // at (mutable)
    _Value&
    at(const _Key& _k)
    {
        auto it = find(_k);

        return it->value;
    }

    // value_or
    //   returns the value for the given key if present,
    // otherwise returns the fallback.
    _Value
    value_or(const _Key&   _k,
             const _Value& _fallback) const
    {
        auto it = find(_k);

        if (it != end())
        {
            return it->value;
        }

        return _fallback;
    }

    // operator[]
    //   returns a reference to the value for the given key.
    // Inserts a default-constructed entry if absent.
    _Value&
    operator[](const _Key& _k)
    {
        auto it = find(_k);

        if (it != end())
        {
            return it->value;
        }

        m_entries.push_back(value_type(_k, _Value{}));

        if (_Sorted)
        {
            sort_entries();

            return find(_k)->value;
        }

        return m_entries.back().value;
    }

    // -----------------------------------------------------------------
    //  modifiers
    // -----------------------------------------------------------------

    // insert
    //   inserts a new entry.  Returns true if the key was new
    // and the entry was inserted; false if the key already
    // existed (no modification).
    bool
    insert(const _Key&   _k,
           const _Value& _v)
    {
        if (contains(_k))
        {
            return false;
        }

        m_entries.push_back(value_type(_k, _v));

        if (_Sorted)
        {
            sort_entries();
        }

        return true;
    }

    // insert (move)
    bool
    insert(_Key&&   _k,
           _Value&& _v)
    {
        if (contains(_k))
        {
            return false;
        }

        m_entries.push_back(
            value_type(static_cast<_Key&&>(_k),
                       static_cast<_Value&&>(_v)));

        if (_Sorted)
        {
            sort_entries();
        }

        return true;
    }

    // insert (pair)
    bool
    insert(const value_type& _entry)
    {
        return insert(_entry.key, _entry.value);
    }

    // insert_or_assign
    //   inserts or overwrites.  Returns true if a new entry
    // was created, false if an existing entry was overwritten.
    bool
    insert_or_assign(const _Key&   _k,
                     const _Value& _v)
    {
        auto it = find(_k);

        if (it != end())
        {
            it->value = _v;

            return false;
        }

        m_entries.push_back(value_type(_k, _v));

        if (_Sorted)
        {
            sort_entries();
        }

        return true;
    }

    // erase
    //   removes the entry with the given key.  Returns true
    // if the entry existed and was removed.
    bool
    erase(const _Key& _k)
    {
        auto it = find(_k);

        if (it == end())
        {
            return false;
        }

        m_entries.erase(it);

        return true;
    }

    // clear
    void clear() noexcept
    {
        m_entries.clear();

        return;
    }

    // reserve
    void reserve(size_type _n)
    {
        m_entries.reserve(_n);

        return;
    }

private:
    // -----------------------------------------------------------------
    //  internal lookup helpers
    // -----------------------------------------------------------------

    const_iterator
    find_linear(const _Key& _k) const
    {
        for (auto it = m_entries.begin();
             it != m_entries.end();
             ++it)
        {
            if (it->key == _k)
            {
                return it;
            }
        }

        return m_entries.end();
    }

    iterator
    find_linear_mut(const _Key& _k)
    {
        for (auto it = m_entries.begin();
             it != m_entries.end();
             ++it)
        {
            if (it->key == _k)
            {
                return it;
            }
        }

        return m_entries.end();
    }

    const_iterator
    find_sorted(const _Key& _k) const
    {
        value_type probe;
        probe.key = _k;

        auto it = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            probe);

        if ( (it != m_entries.end()) &&
             (it->key == _k) )
        {
            return it;
        }

        return m_entries.end();
    }

    iterator
    find_sorted_mut(const _Key& _k)
    {
        value_type probe;
        probe.key = _k;

        auto it = std::lower_bound(
            m_entries.begin(),
            m_entries.end(),
            probe);

        if ( (it != m_entries.end()) &&
             (it->key == _k) )
        {
            return it;
        }

        return m_entries.end();
    }

    void sort_entries()
    {
        std::sort(m_entries.begin(),
                  m_entries.end());

        return;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    container_type m_entries;
};


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_
