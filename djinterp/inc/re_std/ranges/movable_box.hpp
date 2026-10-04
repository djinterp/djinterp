/******************************************************************************
* djinterp [re_std]                                           movable_box.hpp
*
* movable_box header:
*   Provides the C++20/23 exposition-only "movable-box" utility.
* A movable_box<T> wraps a T and provides the operations a view's
* member type needs (default construction, copy/move construction,
* copy/move assignment) even when T itself is missing some of these
* operations.
*
*   The classical example: a view stores a callable (lambda or
* function object) supplied at construction. Many lambdas have no
* user-provided assignment operator, so storing one directly makes
* the view non-assignable, violating the view concept. movable_box
* simulates assignment by destroying and reconstructing the held T.
*
*   APPLICATIONS IN RE_STD:
*   - Future repeat_view enhancement: wrap the stored value in
*     movable_box<T> so the view can be default-constructed even
*     when T isn't default-constructible (the box's default state
*     is "empty"; the user must populate before iterating).
*   - Future transform_view / filter_view / take_while_view /
*     drop_while_view enhancement: wrap the stored callable in
*     movable_box for proper assignability semantics.
*
*   PORTABILITY:
*   - C++11+; depends on re_std::optional (shipped).
*   - Simplified relative to the C++23 spec: the spec uses different
*     paths for is_copy_assignable etc. for efficiency; re_std takes
*     the destroy-and-reconstruct path uniformly.
*
*
* path:      /inc/djinterp/re_std/ranges/movable_box.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_MOVABLE_BOX_
#define DJINTERP_RE_STD_RANGES_MOVABLE_BOX_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../optional/optional.hpp"
#include "../type_traits/type_traits.hpp"


NS_RESTD
NS_INTERNAL


// ===========================================================================
// I.   MOVABLE_BOX
// ===========================================================================

// movable_box<_T>
//   class: holds an optional _T. Provides default ctor, copy/move
// ctor, and copy/move assignment by destroying and reconstructing
// the held value as needed.
template<typename _T>
class movable_box
{
private:
    optional<_T>    m_value;


public:
    // -------- ctors --------

    // default ctor — leaves the box empty UNLESS _T is default-
    // constructible, in which case the contained _T is created.
    // The dispatch is SFINAE-lazy (well-formed only when valid).
    // D_CONSTEXPR_CPP14, not D_CONSTEXPR: C++11 requires a constexpr
    // constructor to have an EMPTY body, and this one dispatches
    // through _default_init(). Relaxed constexpr from C++14 permits it.
    D_CONSTEXPR_CPP14
    movable_box()
        : m_value()
    {
        _default_init(0);
    }

    // value ctor — copies _T in.
    D_CONSTEXPR
    movable_box(
        _T const&  _t
    )
        : m_value(_t)
    {}

    // value ctor — moves _T in.
    D_CONSTEXPR
    movable_box(
        _T&&  _t
    )
        : m_value(static_cast<_T&&>(_t))
    {}

    // copy ctor: copies the underlying optional (which copies the
    // contained _T iff present).
    D_CONSTEXPR
    movable_box(
        movable_box const&  _other
    )
        : m_value(_other.m_value)
    {}

    // move ctor: moves the underlying optional.
    movable_box(
        movable_box&&  _other
    )
        : m_value(static_cast<optional<_T>&&>(_other.m_value))
    {}


    // -------- assignment via destroy + reconstruct --------

    // copy assign — if other has a value, replace ours with a copy;
    // otherwise clear ours. This works for non-assignable _T because
    // we go through the optional's emplace (which uses placement
    // new internally).
    movable_box&
    operator=(
        movable_box const&  _other
    )
    {
        if (this != &_other)
        {
            if (_other.m_value.has_value())
            {
                m_value.emplace(*_other.m_value);
            }
            else
            {
                m_value.reset();
            }
        }
        return *this;
    }

    // move assign — similar, with move semantics.
    movable_box&
    operator=(
        movable_box&&  _other
    )
    {
        if (this != &_other)
        {
            if (_other.m_value.has_value())
            {
                m_value.emplace(static_cast<_T&&>(*_other.m_value));
            }
            else
            {
                m_value.reset();
            }
        }
        return *this;
    }


    // -------- accessors --------
    D_CONSTEXPR bool
    has_value() const
    D_NOEXCEPT
    {
        return m_value.has_value();
    }

    _T&
    operator*()
    D_NOEXCEPT
    {
        return *m_value;
    }

    _T const&
    operator*() const
    D_NOEXCEPT
    {
        return *m_value;
    }


    // -------- mutators --------
    template<typename _A1>
    _T&
    emplace(
        _A1&&  _a1
    )
    {
        m_value.emplace(static_cast<_A1&&>(_a1));
        return *m_value;
    }

    _T&
    emplace()
    {
        m_value.emplace();
        return *m_value;
    }

    void
    reset()
    D_NOEXCEPT
    {
        m_value.reset();
    }


private:
    // _default_init
    //   helper: default-constructs the held _T when _T allows it.
    // The unused int parameter creates a SFINAE ranking — the
    // pointer-overload wins iff _T is default-constructible,
    // otherwise the ellipsis catch-all does nothing.
    template<typename _U>
    typename enable_if<
                  is_default_constructible<_U>::value
              >::type
    _default_init_impl()
    {
        m_value.emplace();
    }

    template<typename _U>
    typename enable_if<
                  !is_default_constructible<_U>::value
              >::type
    _default_init_impl()
    {
        // leave empty
    }

    void
    _default_init(
        int /* dummy */
    )
    {
        _default_init_impl<_T>();
    }
};


NS_END  // internal
NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_MOVABLE_BOX_
