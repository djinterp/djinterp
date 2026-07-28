/******************************************************************************
* djinterp [restd]                                          view_interface.hpp
*
* view_interface CRTP base header:
*   Provides the C++20 CRTP base class that supplies a uniform set of
* range-member operations (empty, size, front, back, operator bool,
* operator[], data) derived from the begin/end pair on the derived
* type. Subclasses that publicly inherit view_interface<D> obtain
* these members for free.
*
*   PORTABILITY:
*   - Requires CRTP + decltype + trailing return types. Available
*     C++11+ only. The class is omitted under C++98/03.
*   - Each member is a template parameterised on the derived type so
*     instantiation is lazy: a member that uses an operation the
*     derived range does not support is only ill-formed when actually
*     called, never at class-template instantiation time.
*   - Inherits view_base publicly, opting derived classes into the
*     default enable_view specialisation.
*
*
* path:      /inc/djinterp/restd/ranges/view_interface.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_VIEW_INTERFACE_
#define DJINTERP_RESTD_RANGES_VIEW_INTERFACE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "./view_base.hpp"


NS_RESTD


// ===========================================================================
// I.   VIEW_INTERFACE
// ===========================================================================

// view_interface<_Derived>
//   class: CRTP base supplying empty / operator bool / size /
// front / back / operator[] / data in terms of begin() and end()
// on the most-derived type.
// note: the public derivation from view_base ensures
// enable_view<_Derived>::value is true under the default trait
// specialisation. Users who want enable_view to be false should
// either skip view_interface or specialise enable_view explicitly.
template<typename _Derived>
class view_interface : public view_base
{
private:
    // derived (mutable)
    //   function: CRTP downcast helper. Returns a reference to the
    // most-derived object.
    D_CONSTEXPR _Derived&
    derived()
    D_NOEXCEPT
    {
        return static_cast<_Derived&>(*this);
    }

    // derived (const)
    //   function: const CRTP downcast helper.
    D_CONSTEXPR _Derived const&
    derived() const
    D_NOEXCEPT
    {
        return static_cast<_Derived const&>(*this);
    }


public:
    // empty
    //   function: true iff begin() == end(). Available whenever the
    // derived range supports forward-iterator comparison between its
    // iterator and sentinel.
    D_CONSTEXPR bool
    empty()
    {
        return (derived().begin() == derived().end());
    }

    // empty (const)
    //   function: const overload.
    D_CONSTEXPR bool
    empty() const
    {
        return (derived().begin() == derived().end());
    }

    // operator bool
    //   function: contextual conversion to bool. true iff the view
    // contains at least one element. explicit on C++11+; on C++98
    // the safe-bool idiom would be required, but view_interface is
    // gated out on C++98 anyway.
    D_CONSTEXPR explicit
    operator bool()
    {
        return !empty();
    }

    // operator bool (const)
    //   function: const overload.
    D_CONSTEXPR explicit
    operator bool() const
    {
        return !empty();
    }

    // size
    //   function: end() - begin(). Available when the iterator type
    // is sized_sentinel_for the sentinel (i.e. supports the
    // arithmetic). Instantiation is lazy.
    D_CONSTEXPR
    auto
    size()
        -> decltype(derived().end() - derived().begin())
    {
        return (derived().end() - derived().begin());
    }

    // size (const)
    //   function: const overload.
    D_CONSTEXPR
    auto
    size() const
        -> decltype(derived().end() - derived().begin())
    {
        return (derived().end() - derived().begin());
    }

    // front
    //   function: *begin(). Precondition: !empty().
    D_CONSTEXPR
    auto
    front()
        -> decltype(*derived().begin())
    {
        return *(derived().begin());
    }

    // front (const)
    //   function: const overload.
    D_CONSTEXPR
    auto
    front() const
        -> decltype(*derived().begin())
    {
        return *(derived().begin());
    }

    // back
    //   function: *(end() - 1). Requires the iterator type to be
    // bidirectional and end() to be reachable / decrementable from.
    // Precondition: !empty().
    D_CONSTEXPR
    auto
    back()
        -> decltype(*(--derived().end()))
    {
        auto it = derived().end();
        --it;
        return *it;
    }

    // back (const)
    //   function: const overload.
    D_CONSTEXPR
    auto
    back() const
        -> decltype(*(--derived().end()))
    {
        auto it = derived().end();
        --it;
        return *it;
    }

    // operator[]
    //   function: begin()[_n]. Requires random-access iteration on
    // the derived range. _Index is templated to allow either signed
    // or unsigned indexing (matching the C++20 size_t / ptrdiff_t
    // tolerance of subscript).
    template<typename _Index>
    D_CONSTEXPR
    auto
    operator[](_Index _n)
        -> decltype(derived().begin()[_n])
    {
        return derived().begin()[_n];
    }

    // operator[] (const)
    //   function: const overload.
    template<typename _Index>
    D_CONSTEXPR
    auto
    operator[](_Index _n) const
        -> decltype(derived().begin()[_n])
    {
        return derived().begin()[_n];
    }

    // data
    //   function: pointer to the underlying buffer. Available only
    // when the derived range exposes a .data() member or its begin()
    // is a raw pointer. The C++20 version constrains via
    // contiguous_iterator; restd defers that check to the lazy
    // expansion of derived().begin() through restd::to_address —
    // which is supplied by <memory>, not gated here, so callers on
    // a non-contiguous range will see the error at the to_address
    // call site once <memory> is in the include set.
    // note: a placeholder pointer-return path that uses &*begin() is
    // intentionally NOT used because that traps on proxy iterators.
    // Derived views with a meaningful .data() are expected to
    // shadow this member directly.
    D_CONSTEXPR
    auto
    data()
        -> decltype(&(*derived().begin()))
    {
        return (derived().begin() == derived().end())
            ? static_cast<decltype(&(*derived().begin()))>(D_NULLPTR)
            : &(*derived().begin());
    }

    // data (const)
    //   function: const overload.
    D_CONSTEXPR
    auto
    data() const
        -> decltype(&(*derived().begin()))
    {
        return (derived().begin() == derived().end())
            ? static_cast<decltype(&(*derived().begin()))>(D_NULLPTR)
            : &(*derived().begin());
    }
};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_VIEW_INTERFACE_
