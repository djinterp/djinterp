/*******************************************************************************
* djinterp [re_std]                                    non_propagating_cache.hpp
*
* non_propagating_cache header:
*   Provides the C++20 exposition-only "non-propagating-cache"
* utility. A non_propagating_cache<T> holds an optionally-present
* T. Crucially, copy / move construction and assignment do NOT
* propagate the cached value — the destination always starts (or
* becomes) empty, and move operations additionally clear the source.
*
*   PURPOSE:
*   View adaptors that cache lazily-computed values (e.g. the first
* satisfying iterator for a filter_view; the active inner range for
* a join_view over prvalue inners) must NOT carry that cache
* across copies or moves of the view. The cache references the
* original view's storage and would be a dangling pointer after
* a copy/move. The non-propagating semantics enforce this safely.
*
*   APPLICATIONS IN RE_STD:
*   - Future join_view enhancement: remove the static_assert that
*     rejects prvalue inner ranges (R8). Wrap the active inner in a
*     non_propagating_cache<remove_reference<inner_t>>.
*   - Future filter_view / drop_view enhancement: replace the
*     mutable-bool + storage cache with non_propagating_cache for
*     cleaner copy semantics.
*
*   PORTABILITY:
*   - C++11+; depends on re_std::optional (shipped).
*   - The semantics are exactly as the C++20 exposition spec:
*       copy ctor       — resets destination to empty
*       move ctor       — resets destination to empty, clears source
*       copy assign     — resets destination to empty
*       move assign     — resets destination to empty, clears source
*
*
* path:      /inc/re_std/ranges/non_propagating_cache.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_NON_PROPAGATING_CACHE_HPP
#define RE_STD_RANGES_NON_PROPAGATING_CACHE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../optional/optional.hpp"
#include "../type_traits/type_traits.hpp"


namespace re_std
{
namespace internal
{


// ===========================================================================
// I.   NON_PROPAGATING_CACHE
// ===========================================================================

// non_propagating_cache<T>
//   class: holds an optional T whose presence does NOT survive
// copies, moves, or assignments of the cache. The cached value is
// computed locally to a given instance and stays with it.
template<typename T>
class non_propagating_cache
{
private:
    optional<T>    m_value;


public:
    // -------- ctors --------
    RE_STD_CONSTEXPR
    non_propagating_cache()
        : m_value()
    {}

    // copy ctor: destination empty.
    RE_STD_CONSTEXPR
    non_propagating_cache(
        non_propagating_cache const&
    )
    RE_STD_NOEXCEPT
        : m_value()
    {}

    // move ctor: destination empty, source cleared.
    non_propagating_cache(
        non_propagating_cache&&  _other
    )
    RE_STD_NOEXCEPT
        : m_value()
    {
        _other.m_value.reset();
    }


    // -------- assignments --------
    // copy assign: destination cleared (regardless of self-assign).
    non_propagating_cache&
    operator=(
        non_propagating_cache const&  _other
    )
    RE_STD_NOEXCEPT
    {
        if (this != &_other)
        {
            m_value.reset();
        }
        return *this;
    }

    // move assign: destination cleared, source cleared.
    non_propagating_cache&
    operator=(
        non_propagating_cache&&  _other
    )
    RE_STD_NOEXCEPT
    {
        m_value.reset();
        if (this != &_other)
        {
            _other.m_value.reset();
        }
        return *this;
    }


    // -------- accessors --------
    RE_STD_CONSTEXPR bool
    has_value() const
    RE_STD_NOEXCEPT
    {
        return m_value.has_value();
    }

    T&
    operator*()
    RE_STD_NOEXCEPT
    {
        return *m_value;
    }

    T const&
    operator*() const
    RE_STD_NOEXCEPT
    {
        return *m_value;
    }


    // -------- mutators --------

    // emplace_deref
    //   function: constructs the cached T from _ctor_args and
    // returns a reference to it. Mirrors the spec's name.
    template<typename A1>
    T&
    emplace_deref(
        A1&&  _a1
    )
    {
        m_value.emplace(static_cast<A1&&>(_a1));
        return *m_value;
    }

    template<typename A1, typename A2>
    T&
    emplace_deref(
        A1&&  _a1,
        A2&&  _a2
    )
    {
        m_value.emplace(static_cast<A1&&>(_a1), static_cast<A2&&>(_a2));
        return *m_value;
    }

    // emplace_deref (0-arg)
    T&
    emplace_deref()
    {
        m_value.emplace();
        return *m_value;
    }

    // reset — drop the cached value.
    void
    reset()
    RE_STD_NOEXCEPT
    {
        m_value.reset();
    }
};


}  // internal
}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_NON_PROPAGATING_CACHE_HPP
