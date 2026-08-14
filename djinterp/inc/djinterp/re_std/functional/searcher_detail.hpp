/******************************************************************************
* re_std [functional]                                       searcher_detail.hpp
*
*   internal support for the Boyer-Moore searchers.
*
*   Not a public header.  Provides two things the searchers need and re_std
* does not yet have anywhere else:
*
*     internal::searcher_buffer<T>   a minimal owning dynamic array
*     internal::bad_char_table<...>  the last-occurrence table, in two forms
*
*   WHY A LOCAL BUFFER AND NOT A CONTAINER.
*   libstdc++ builds these tables in std::vector and std::unordered_map.
* re_std has neither yet - <vector> is roadmap 50 and <unordered_map> is 58 -
* so the searchers would be blocked for a long time on containers they barely
* use.  A ~60-line owning buffer is the honest way through: it is an
* implementation detail, never named in the public interface, and it goes away
* the day <vector> lands.
*
*   THE TABLE HAS TWO STRATEGIES, AND THE SPLIT IS NOT A MICRO-OPTIMISATION.
*   For a one-byte value type the alphabet is 256 symbols, so the table is a
* flat array: O(1) lookup, no hashing, no allocation beyond a fixed block.
* That is the case that actually occurs - searching char strings - and it is
* worth having exactly right.
*   For everything else the alphabet is unbounded and the table must be a hash
* map.  It is open-addressed with linear probing rather than chained, because
* chaining would need a node allocation per entry and the table is built once
* and then only read.
*
*   NOTE ON HASH AVAILABILITY.
*   The general strategy needs re_std::hash, which floors at C++11.  The
* one-byte strategy needs no hash at all, which is why default_searcher (no
* table) reaches C++98 while the Boyer-Moore searchers stop at C++11.
*
*
* path:      /inc/djinterp/re_std/functional/searcher_detail.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_SEARCHER_DETAIL_
#define RESTD_FUNCTIONAL_SEARCHER_DETAIL_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./hash.hpp"

NS_DJINTERP
NS_RESTD
NS_INTERNAL

// searcher_buffer
//   class: a minimal owning dynamic array.  Deliberately not a container -
// no iterators, no growth, no allocator support.  Sized once at construction,
// which is all the searchers need.
template<typename _Type>
class searcher_buffer
{
    _Type* m_data;
    size_t m_size;

public:
    searcher_buffer() D_NOEXCEPT : m_data(0), m_size(0) {}

    explicit searcher_buffer(size_t n, const _Type& fill = _Type())
        : m_data(n ? new _Type[n] : 0), m_size(n)
    {
        for (size_t i = 0; i < m_size; ++i) { m_data[i] = fill; }
        return;
    }

    searcher_buffer(const searcher_buffer& other)
        : m_data(other.m_size ? new _Type[other.m_size] : 0),
          m_size(other.m_size)
    {
        for (size_t i = 0; i < m_size; ++i) { m_data[i] = other.m_data[i]; }
        return;
    }

    searcher_buffer& operator=(const searcher_buffer& other)
    {
        if (this != &other)
        {
            searcher_buffer tmp(other);
            swap(tmp);
        }
        return *this;
    }

    searcher_buffer(searcher_buffer&& other) D_NOEXCEPT
        : m_data(other.m_data), m_size(other.m_size)
    {
        other.m_data = 0;
        other.m_size = 0;
        return;
    }

    searcher_buffer& operator=(searcher_buffer&& other) D_NOEXCEPT
    {
        if (this != &other)
        {
            delete[] m_data;
            m_data       = other.m_data;
            m_size       = other.m_size;
            other.m_data = 0;
            other.m_size = 0;
        }
        return *this;
    }

    ~searcher_buffer() { delete[] m_data; }

    void swap(searcher_buffer& other) D_NOEXCEPT
    {
        _Type* d = m_data; m_data = other.m_data; other.m_data = d;
        size_t s = m_size; m_size = other.m_size; other.m_size = s;
        return;
    }

    size_t size()  const D_NOEXCEPT { return m_size; }
    bool   empty() const D_NOEXCEPT { return m_size == 0; }

    _Type&       operator[](size_t i)       { return m_data[i]; }
    const _Type& operator[](size_t i) const { return m_data[i]; }
};


// =============================================================================
// BAD-CHARACTER TABLE
// =============================================================================

// bad_char_table
//   class: maps a value to the index of its LAST occurrence in the pattern,
// or -1 when it does not occur.  Primary template is the general (hashed)
// strategy; the one-byte specialisation follows.
template<typename _Value,
         typename _Hash,
         typename _Pred,
         bool _Direct = (   is_integral<_Value>::value
                         && sizeof(_Value) == 1)>
class bad_char_table
{
    //   Open-addressed, linear-probed, power-of-two capacity.  Built once and
    // then read-only, so there is no erase and no tombstone handling.
    searcher_buffer<_Value>    m_keys;
    searcher_buffer<ptrdiff_t> m_vals;
    searcher_buffer<char>      m_used;
    size_t                     m_mask;
    _Hash                      m_hash;
    _Pred                      m_pred;

    size_t slot_for(const _Value& key) const
    {
        size_t i = static_cast<size_t>(m_hash(key)) & m_mask;
        while (m_used[i] && !m_pred(m_keys[i], key))
        {
            i = (i + 1) & m_mask;
        }
        return i;
    }

public:
    bad_char_table(size_t pattern_size, _Hash h, _Pred p)
        : m_keys(), m_vals(), m_used(), m_mask(0), m_hash(h), m_pred(p)
    {
        //   Capacity is the next power of two at least twice the pattern
        // length, so the load factor stays under 0.5 and linear probing does
        // not degrade.  A pattern of length 0 still needs one slot so that
        // slot_for has somewhere to land.
        size_t cap = 8;
        while (cap < (pattern_size * 2 + 1)) { cap <<= 1; }
        m_keys = searcher_buffer<_Value>(cap);
        m_vals = searcher_buffer<ptrdiff_t>(cap, -1);
        m_used = searcher_buffer<char>(cap, 0);
        m_mask = cap - 1;
        return;
    }

    void set(const _Value& key, ptrdiff_t index)
    {
        const size_t i = slot_for(key);
        m_keys[i] = key;
        m_vals[i] = index;
        m_used[i] = 1;
        return;
    }

    ptrdiff_t get(const _Value& key) const
    {
        const size_t i = slot_for(key);
        return m_used[i] ? m_vals[i] : static_cast<ptrdiff_t>(-1);
    }
};

// bad_char_table<..., true>
//   class: flat-array strategy for one-byte value types.  256 entries, direct
// indexing, no hashing.  This is the case that actually occurs.
template<typename _Value, typename _Hash, typename _Pred>
class bad_char_table<_Value, _Hash, _Pred, true>
{
    ptrdiff_t m_table[256];

    //   Index via unsigned char so that a negative signed char maps into
    // [0,256) rather than out of bounds - the classic char-signedness bug in
    // hand-written Boyer-Moore.
    static size_t index_of(const _Value& v)
    {
        return static_cast<size_t>(
            static_cast<unsigned char>(static_cast<char>(v)));
    }

public:
    bad_char_table(size_t, _Hash, _Pred)
    {
        for (size_t i = 0; i < 256; ++i) { m_table[i] = -1; }
        return;
    }

    void set(const _Value& key, ptrdiff_t index)
    {
        m_table[index_of(key)] = index;
        return;
    }

    ptrdiff_t get(const _Value& key) const
    {
        return m_table[index_of(key)];
    }
};

NS_END  // internal
NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_SEARCHER_DETAIL_
