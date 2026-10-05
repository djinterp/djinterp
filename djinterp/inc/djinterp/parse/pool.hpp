/*******************************************************************************
* djinterp [parse]                                                      pool.hpp
*
*   The C++ face of the intern pool declared in pool.h.
*   `pool` derives from d_parse_pool, adds no data member, and is asserted
* layout-identical -- so a C stage interns into a C++ pool by taking its
* address.  What the C++ side adds is lifetime and the two typed spellings of
* an intern, since a blob and a string differ only in whether the terminator
* travels with them and getting that wrong is silent.
*
*   `fixed_pool<Bytes, Entries>` carries its own storage, for a stage that must
* not allocate.
*
*
* path:      /inc/djinterp/parse/pool.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_POOL_HPP
#define DJINTERP_PARSE_POOL_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>      // std::is_standard_layout
// djinterp
#include "../djinterp.hpp"  // framework root
#include "./charset.hpp"    // parse::charset, and NS_PARSE
#include "./c/pool.h"       // the C pool this layer faces
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t, uint64_t


NS_DJINTERP
NS_PARSE


// pool
//   class: the blob store and its table, with lifetime.  Move-only: two owners
// of one blob store is not a thing worth supporting, and every index handed
// out refers into exactly one pool.
class pool : public d_parse_pool
{
public:
    // pool
    //   constructor: an empty pool owning nothing, which interns nothing.
    pool() noexcept
    {
        d_parse_pool_init(this, nullptr, 0u, nullptr, 0u);
    }

    // pool
    //   constructor: a pool over caller-supplied storage.
    pool(
        char*                      _bytes,
        re_std::uint32_t              _byte_capacity,
        d_parse_pool_entry*        _entries,
        re_std::uint32_t              _entry_capacity
    ) noexcept
    {
        d_parse_pool_init(this,
                          _bytes,
                          _byte_capacity,
                          _entries,
                          _entry_capacity);
    }

    pool(const pool&)            = delete;
    pool& operator=(const pool&) = delete;

    // pool
    //   constructor: takes over another pool's storage and ownership.
    pool(
        pool&& _other
    ) noexcept
        : d_parse_pool(_other)
    {
        d_parse_pool_init(&_other, nullptr, 0u, nullptr, 0u);
    }

    // operator=
    //   function: releases this pool, then takes over another's.
    pool&
    operator=(
        pool&& _other
    ) noexcept
    {
        // guard against self-move, which would release the storage being taken
        if (this != &_other)
        {
            d_parse_pool_release(this);

            static_cast<d_parse_pool&>(*this) = _other;

            d_parse_pool_init(&_other, nullptr, 0u, nullptr, 0u);
        }

        return *this;
    }

    // ~pool
    //   destructor: releases any storage this pool owns.
    ~pool() noexcept
    {
        d_parse_pool_release(this);
    }

#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
    // reserve
    //   function: replaces this pool's storage with storage it allocates and
    // owns, which then grows on demand.  Returns false if refused.
    D_NODISCARD bool
    reserve(
        re_std::uint32_t _byte_capacity  = 0u,
        re_std::uint32_t _entry_capacity = 0u
    ) noexcept
    {
        d_parse_pool_release(this);

        return (d_parse_pool_init_heap(this,
                                       _byte_capacity,
                                       _entry_capacity) == 0);
    }
#endif  // D_INTERNAL_PARSE_POOL_HEAP

    // intern
    //   function: the index of a blob, adding it if it is not already present.
    re_std::uint32_t
    intern(
        const void*   _data,
        re_std::uint32_t _length
    ) noexcept
    {
        return d_parse_pool_intern(this, _data, _length);
    }

    // intern
    //   function: the index of a string, terminator included.
    re_std::uint32_t
    intern(
        const char* _text
    ) noexcept
    {
        return d_parse_pool_intern_string(this, _text);
    }

    // intern
    //   function: the index of a character class, stored as its own bytes.
    re_std::uint32_t
    intern(
        const d_parse_charset& _set
    ) noexcept
    {
        return d_parse_pool_intern(this, _set.bits, D_PARSE_CHARSET_BYTES);
    }

    // find
    //   accessor: the index of a blob already interned, without adding one.
    re_std::uint32_t
    find(
        const void*   _data,
        re_std::uint32_t _length
    ) const noexcept
    {
        return d_parse_pool_find(this, _data, _length);
    }

    // data
    //   accessor: the bytes of an interned blob, or null.
    const void*
    data(
        re_std::uint32_t _index
    ) const noexcept
    {
        return d_parse_pool_data(this, _index);
    }

    // length
    //   accessor: the length of an interned blob, or 0.
    re_std::uint32_t
    length(
        re_std::uint32_t _index
    ) const noexcept
    {
        return d_parse_pool_length(this, _index);
    }

    // text
    //   accessor: an interned string, or "".  Never null.
    const char*
    text(
        re_std::uint32_t _index
    ) const noexcept
    {
        return d_parse_pool_string(this, _index);
    }

    // set
    //   accessor: an interned character class, or null when the index names a
    // blob of another size.
    const d_parse_charset*
    set(
        re_std::uint32_t _index
    ) const noexcept
    {
        if (length(_index) != D_PARSE_CHARSET_BYTES)
        {
            return nullptr;
        }

        return static_cast<const d_parse_charset*>(data(_index));
    }

    // clear
    //   function: empties the pool for reuse, keeping its storage.
    //   CAUTION: every index handed out becomes meaningless.
    void
    clear() noexcept
    {
        d_parse_pool_reset(this);
    }

    // size
    //   accessor: how many blobs are interned.
    D_CONSTEXPR_CPP14 re_std::uint32_t
    size() const noexcept
    {
        return count;
    }

    // digest
    //   accessor: a 64-bit key over everything the pool holds.
    re_std::uint64_t
    digest() const noexcept
    {
        return d_parse_pool_hash(this);
    }
};


// fixed_pool
//   class: a pool carrying its own storage, for a stage that must not
// allocate.  Not layout-identical to the C pool, because it adds the arrays as
// members; it converts through its base as any derived class does.
template<re_std::uint32_t Bytes,
         re_std::uint32_t Entries>
class fixed_pool : public pool
{
public:
    // fixed_pool
    //   constructor: binds the embedded arrays as this pool's storage.
    fixed_pool() noexcept
    {
        d_parse_pool_init(this, m_bytes, Bytes, m_entries, Entries);
    }

private:
    char               m_bytes[Bytes];
    d_parse_pool_entry m_entries[Entries];
};


//   The claim this header makes is that its type costs nothing over the C one.
static_assert(sizeof(pool) == sizeof(d_parse_pool),
              "parse::pool must add no data member");
static_assert(alignof(pool) == alignof(d_parse_pool),
              "parse::pool must add no data member");
static_assert(std::is_standard_layout<pool>::value,
              "parse::pool must remain standard-layout");


NS_END  // parse
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_POOL_HPP
