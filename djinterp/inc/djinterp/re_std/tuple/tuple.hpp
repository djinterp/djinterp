/******************************************************************************
* djinterp [restd]                                                    tuple.hpp
*
* tuple class header:
*   Fixed-size collection of heterogeneous values. A generalisation of
* `pair` to N elements. The restd::tuple is layout-compatible with
* a recursive-inheritance scheme: tuple<T0, T1, T2> derives from
* tuple<T1, T2> derives from tuple<T2> derives from tuple<>.
*
*     tuple<int, char, double> t(1, 'x', 3.14);
*     get<0>(t);     // -> int&  (1)
*     get<1>(t);     // -> char& ('x')
*     get<double>(t);// -> double& (3.14)        (C++14+ by-type get)
*
*   DESIGN:
*   Each non-empty tuple instantiation inherits from a head holder
* (storing the first element) and the tail tuple (storing the rest).
* This gives:
*     - O(1) access to any element via static_cast to the appropriate
*       base.
*     - Empty Base Optimisation for empty element types.
*     - Trivial special members when every element type's special
*       members are trivial.
*
*   STORAGE FORMAT:
*   - tuple<>                 : empty struct (no members).
*   - tuple<T0, T1, ..., Tn-1>: derives from tuple_head<0, T0>
*                                and tuple<T1, ..., Tn-1>.
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+). The
* whole header is omitted on C++98/03; consumer code must gate on
* D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES.
*
*   constexpr is applied opportunistically:
*   - The default and copy constructors are constexpr on C++11+.
*   - Element-wise constructors are constexpr.
*   - get<I>() and get<T>() are constexpr (via free-function form).
*   - Assignment is constexpr only on C++14+ (relaxed constexpr).
*
*   PAIR INTEROP:
*   The 2-element specialisation supports pair-converting copy and
* move construction plus pair-converting copy and move assignment.
* These template members are SFINAE-restricted to sizeof...(_Tail)
* == 1 and require pair to be complete at the point of instantiation
* (a forward declaration is supplied above; the user must include
* "../utility/pair.hpp" before invoking).
*
*
* path:      /inc/djinterp/restd/tuple/tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_TUPLE_
#define DJINTERP_RESTD_TUPLE_TUPLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: tuple requires variadic templates + rvalue refs
#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "../type_traits/integral_constant.hpp"
#include "../type_traits/conditional.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_assignable.hpp"
#include "../type_traits/is_convertible.hpp"
#include "../type_traits/is_nothrow_constructible.hpp"
#include "../type_traits/is_nothrow_assignable.hpp"
#include "../type_traits/is_empty.hpp"
#include "../type_traits/is_final.hpp"
#include "../type_traits/decay.hpp"
#include "../type_traits/remove_reference.hpp"


NS_RESTD


// =============================================================================
// FORWARD DECLARATION OF PAIR
// =============================================================================
// Forward-declared rather than #included so tuple.hpp stays
// independent of <utility>'s pair definition at parse time. The
// pair-converting ctor / assignment templates below only instantiate
// when called, at which point pair must be complete (via the user's
// own #include of pair.hpp or via pair_tuple_size.hpp / pair_get.hpp).
template<typename _T1,
         typename _T2>
struct pair;


NS_END  // restd


NS_RESTD


NS_INTERNAL

    // ------------------------------------------------------------------
    // tuple_head<_I, _T>
    //   class: holds the _I-th element of a tuple. The index makes
    // each base distinct so that tuple<T, T, T> has three distinct
    // tuple_head bases, not one.
    //
    //   When _T is empty and not final, EBO is applied by deriving
    // from _T (giving zero-byte storage for empty types). Otherwise
    // _T is held as a value member.
    // ------------------------------------------------------------------

    template<std::size_t _I,
             typename    _T,
             bool        _UseEbo =
                 ( is_empty<_T>::value && !is_final<_T>::value )>
    class tuple_head;

    // EBO path: derive from _T.
    template<std::size_t _I,
             typename    _T>
    class tuple_head<_I, _T, true> : private _T
    {
    public:
        D_CONSTEXPR
        tuple_head() D_NOEXCEPT
        {}

        D_CONSTEXPR
        tuple_head(
            const _T& _v
        )
            : _T(_v)
        {}

        template<typename _U>
        D_CONSTEXPR
        tuple_head(
            _U&& _v
        )
            : _T(static_cast<_U&&>(_v))
        {}

        D_CONSTEXPR
        _T&
        head() D_NOEXCEPT
        {
            return *this;
        }

        D_CONSTEXPR
        const _T&
        head() const D_NOEXCEPT
        {
            return *this;
        }
    };

    // value-member path: hold _T as a member named m_value.
    template<std::size_t _I,
             typename    _T>
    class tuple_head<_I, _T, false>
    {
    public:
        D_CONSTEXPR
        tuple_head()
            : m_value()
        {}

        D_CONSTEXPR
        tuple_head(
            const _T& _v
        )
            : m_value(_v)
        {}

        template<typename _U>
        D_CONSTEXPR
        tuple_head(
            _U&& _v
        )
            : m_value(static_cast<_U&&>(_v))
        {}

        D_CONSTEXPR
        _T&
        head() D_NOEXCEPT
        {
            return m_value;
        }

        D_CONSTEXPR
        const _T&
        head() const D_NOEXCEPT
        {
            return m_value;
        }

    private:
        _T m_value;
    };

NS_END  // internal


// =============================================================================
// I.   TUPLE
// =============================================================================
// The forward declaration in tuple_size.hpp / tuple_element.hpp is
// matched here. The primary definition lives in this file.

template<typename... _Types>
class tuple;


// -----------------------------------------------------------------------------
// I-A. EMPTY TUPLE: tuple<>
// -----------------------------------------------------------------------------

template<>
class tuple<>
{
public:
    D_CONSTEXPR
    tuple() D_NOEXCEPT
    {}

    void
    swap(
        tuple&
    ) D_NOEXCEPT
    {
        return;
    }
};


// -----------------------------------------------------------------------------
// I-B. NON-EMPTY TUPLE: tuple<_Head, _Tail...>
// -----------------------------------------------------------------------------
// Recursive inheritance scheme. The element index is computed from
// the tail length so tuple_head<I, T> bases are distinct even when
// element types repeat.

template<typename    _Head,
         typename... _Tail>
class tuple<_Head, _Tail...>
    : private internal::tuple_head<sizeof...(_Tail), _Head>,
      private tuple<_Tail...>
{
private:
    typedef internal::tuple_head<sizeof...(_Tail), _Head> _head_base;
    typedef tuple<_Tail...>                                _tail_base;

public:
    // ---------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------

    // 1) Default constructor.
    //    Value-initialises every element. Requires every element type
    //    to be default-constructible.
    D_CONSTEXPR
    tuple()
        : _head_base(),
          _tail_base()
    {}

    // 2) Direct constructor.
    //    Initialises each element from the corresponding argument.
    D_CONSTEXPR
    tuple(
        const _Head&    _h,
        const _Tail&... _t
    )
        : _head_base(_h),
          _tail_base(_t...)
    {}

    // 3) Converting constructor (perfect forwarding).
    template<typename    _UHead,
             typename... _UTail,
             typename = typename enable_if<
                 ( sizeof...(_UTail) == sizeof...(_Tail) &&
                   is_constructible<_Head, _UHead&&>::value )
             >::type>
    D_CONSTEXPR
    tuple(
        _UHead&&    _h,
        _UTail&&... _t
    )
        : _head_base(static_cast<_UHead&&>(_h)),
          _tail_base(static_cast<_UTail&&>(_t)...)
    {}

    // 4) Converting copy constructor.
    template<typename    _UHead,
             typename... _UTail,
             typename = typename enable_if<
                 ( sizeof...(_UTail) == sizeof...(_Tail) &&
                   is_constructible<_Head, const _UHead&>::value )
             >::type>
    D_CONSTEXPR
    tuple(
        const tuple<_UHead, _UTail...>& _other
    )
        : _head_base(_other.head_ref()),
          _tail_base(_other.tail_ref())
    {}

    // 5) Converting move constructor.
    template<typename    _UHead,
             typename... _UTail,
             typename = typename enable_if<
                 ( sizeof...(_UTail) == sizeof...(_Tail) &&
                   is_constructible<_Head, _UHead&&>::value )
             >::type>
    D_CONSTEXPR
    tuple(
        tuple<_UHead, _UTail...>&& _other
    )
        : _head_base(static_cast<_UHead&&>(_other.head_ref())),
          _tail_base(static_cast<tuple<_UTail...>&&>(_other.tail_ref()))
    {}

    // 6) Pair-converting copy constructor (2-element tuples only).
    //    Initialises from pair.first / pair.second. SFINAE-restricted
    // to 2-element tuples (sizeof...(_Tail) == 1) so it doesn't fire
    // for other arities. Requires pair to be complete at the point
    // of instantiation (via #include "../utility/pair.hpp" in user
    // code or via pair_tuple_size.hpp / pair_get.hpp).
    template<typename _U1,
             typename _U2,
             typename = typename enable_if<
                 ( sizeof...(_Tail) == 1 &&
                   is_constructible<_Head, const _U1&>::value )
             >::type>
    D_CONSTEXPR
    tuple(
        const pair<_U1, _U2>& _p
    )
        : _head_base(_p.first),
          _tail_base(_p.second)
    {}

    // 7) Pair-converting move constructor (2-element tuples only).
    //    Same shape as (6) but rvalue-extracting pair.first / .second.
    template<typename _U1,
             typename _U2,
             typename = typename enable_if<
                 ( sizeof...(_Tail) == 1 &&
                   is_constructible<_Head, _U1&&>::value )
             >::type>
    D_CONSTEXPR
    tuple(
        pair<_U1, _U2>&& _p
    )
        : _head_base(static_cast<_U1&&>(_p.first)),
          _tail_base(static_cast<_U2&&>(_p.second))
    {}

    // ---------------------------------------------------------------
    // Assignment
    // ---------------------------------------------------------------

    tuple&
    operator=(
        const tuple& _other
    )
    {
        head_ref() = _other.head_ref();
        tail_ref() = _other.tail_ref();
        return *this;
    }

    tuple&
    operator=(
        tuple&& _other
    )
    {
        head_ref() = static_cast<_Head&&>(_other.head_ref());
        tail_ref() = static_cast<_tail_base&&>(_other.tail_ref());
        return *this;
    }

    template<typename    _UHead,
             typename... _UTail>
    typename enable_if<
        sizeof...(_UTail) == sizeof...(_Tail),
        tuple&
    >::type
    operator=(
        const tuple<_UHead, _UTail...>& _other
    )
    {
        head_ref() = _other.head_ref();
        tail_ref() = _other.tail_ref();
        return *this;
    }

    template<typename    _UHead,
             typename... _UTail>
    typename enable_if<
        sizeof...(_UTail) == sizeof...(_Tail),
        tuple&
    >::type
    operator=(
        tuple<_UHead, _UTail...>&& _other
    )
    {
        head_ref() = static_cast<_UHead&&>(_other.head_ref());
        tail_ref() = static_cast<tuple<_UTail...>&&>(_other.tail_ref());
        return *this;
    }

    // Pair-converting copy assignment (2-element tuples only).
    //   Assigns head from pair.first and the single-element tail's
    // head from pair.second. SFINAE-restricted to sizeof...(_Tail)
    // == 1 so it does not match for other arities.
    template<typename _U1,
             typename _U2>
    typename enable_if<
        sizeof...(_Tail) == 1,
        tuple&
    >::type
    operator=(
        const pair<_U1, _U2>& _p
    )
    {
        head_ref() = _p.first;
        tail_ref().head_ref() = _p.second;
        return *this;
    }

    // Pair-converting move assignment (2-element tuples only).
    template<typename _U1,
             typename _U2>
    typename enable_if<
        sizeof...(_Tail) == 1,
        tuple&
    >::type
    operator=(
        pair<_U1, _U2>&& _p
    )
    {
        head_ref() = static_cast<_U1&&>(_p.first);
        tail_ref().head_ref() = static_cast<_U2&&>(_p.second);
        return *this;
    }

    // ---------------------------------------------------------------
    // swap
    // ---------------------------------------------------------------

    void
    swap(
        tuple& _other
    )
    {
        // canonical three-way swap (no <utility> dependency).
        _Head tmp(static_cast<_Head&&>(head_ref()));
        head_ref()         = static_cast<_Head&&>(_other.head_ref());
        _other.head_ref()  = static_cast<_Head&&>(tmp);
        tail_ref().swap(_other.tail_ref());
        return;
    }

    // ---------------------------------------------------------------
    // Internal accessors used by get() and by converting ctors of
    // sibling tuple instantiations. NOT part of the public API.
    // ---------------------------------------------------------------

    D_CONSTEXPR
    _Head&
    head_ref() D_NOEXCEPT
    {
        return _head_base::head();
    }

    D_CONSTEXPR
    const _Head&
    head_ref() const D_NOEXCEPT
    {
        return _head_base::head();
    }

    D_CONSTEXPR
    _tail_base&
    tail_ref() D_NOEXCEPT
    {
        return *this;
    }

    D_CONSTEXPR
    const _tail_base&
    tail_ref() const D_NOEXCEPT
    {
        return *this;
    }
};


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_TUPLE_
