/*******************************************************************************
* djinterp [re_std]                                                    tuple.hpp
*
* tuple class header:
*   Fixed-size collection of heterogeneous values. A generalisation of
* `pair` to N elements. The re_std::tuple is layout-compatible with
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
* RE_STD_LANG_HAS_VARIADIC_TEMPLATES.
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
* These template members are SFINAE-restricted to sizeof...(Tail)
* == 1 and require pair to be complete at the point of instantiation
* (a forward declaration is supplied above; the user must include
* "../utility/pair.hpp" before invoking).
*
*
* path:      /inc/re_std/tuple/tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_HPP
#define RE_STD_TUPLE_TUPLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: tuple requires variadic templates + rvalue refs
#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// std
#include <cstddef>
// re_std
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


// =============================================================================
// 0.   COMPATIBILITY
// =============================================================================
// In C++11 a constexpr member function is IMPLICITLY const. That makes the
// non-const and const accessor pairs below (head / head_ref / tail_ref)
// collapse into two functions differing only in return type -- ill-formed,
// and it took every tuple down on the C++11 tier. C++14 dropped the
// implicit const, so the non-const halves are gated to C++14 and are
// simply non-constexpr on C++11.
namespace re_std
{
//   Opened here 2026-08-25. This file previously began its
// namespaced content without opening re_std, so everything above
// the first closing brace lived at GLOBAL SCOPE, and that brace
// closed a namespace that was never opened.


// =============================================================================
// FORWARD DECLARATION OF PAIR
// =============================================================================
// Forward-declared rather than #included so tuple.hpp stays
// independent of <utility>'s pair definition at parse time. The
// pair-converting ctor / assignment templates below only instantiate
// when called, at which point pair must be complete (via the user's
// own #include of pair.hpp or via pair_tuple_size.hpp / pair_get.hpp).
template<typename T1,
         typename T2>
struct pair;


}  // re_std


namespace re_std
{


namespace internal
{

    // ------------------------------------------------------------------
    // tuple_head<I, T>
    //   class: holds the I-th element of a tuple. The index makes
    // each base distinct so that tuple<T, T, T> has three distinct
    // tuple_head bases, not one.
    //
    //   When T is empty and not final, EBO is applied by deriving
    // from T (giving zero-byte storage for empty types). Otherwise
    // T is held as a value member.
    // ------------------------------------------------------------------

    template<std::size_t I,
             typename    T,
             bool        UseEbo =
                 ( is_empty<T>::value && !is_final<T>::value )>
    class tuple_head;

    // EBO path: derive from T.
    template<std::size_t I,
             typename    T>
    class tuple_head<I, T, true> : private T
    {
    public:
        RE_STD_CONSTEXPR
        tuple_head() RE_STD_NOEXCEPT
        {}

        RE_STD_CONSTEXPR
        tuple_head(
            const T& _v
        )
            : T(_v)
        {}

        template<typename U>
        RE_STD_CONSTEXPR
        tuple_head(
            U&& _v
        )
            : T(static_cast<U&&>(_v))
        {}

        RE_STD_CONSTEXPR_CPP14
        T&
        head() RE_STD_NOEXCEPT
        {
            return *this;
        }

        RE_STD_CONSTEXPR
        const T&
        head() const RE_STD_NOEXCEPT
        {
            return *this;
        }
    };

    // value-member path: hold T as a member named m_value.
    template<std::size_t I,
             typename    T>
    class tuple_head<I, T, false>
    {
    public:
        RE_STD_CONSTEXPR
        tuple_head()
            : m_value()
        {}

        RE_STD_CONSTEXPR
        tuple_head(
            const T& _v
        )
            : m_value(_v)
        {}

        template<typename U>
        RE_STD_CONSTEXPR
        tuple_head(
            U&& _v
        )
            : m_value(static_cast<U&&>(_v))
        {}

        RE_STD_CONSTEXPR_CPP14
        T&
        head() RE_STD_NOEXCEPT
        {
            return m_value;
        }

        RE_STD_CONSTEXPR
        const T&
        head() const RE_STD_NOEXCEPT
        {
            return m_value;
        }

    private:
        T m_value;
    };

}  // internal


// =============================================================================
// I.   TUPLE
// =============================================================================
// The forward declaration in tuple_size.hpp / tuple_element.hpp is
// matched here. The primary definition lives in this file.

template<typename... Types>
class tuple;


// -----------------------------------------------------------------------------
// I-A. EMPTY TUPLE: tuple<>
// -----------------------------------------------------------------------------

template<>
class tuple<>
{
public:
    RE_STD_CONSTEXPR
    tuple() RE_STD_NOEXCEPT
    {}

    void
    swap(
        tuple&
    ) RE_STD_NOEXCEPT
    {
        return;
    }
};


// -----------------------------------------------------------------------------
// I-B. NON-EMPTY TUPLE: tuple<Head, Tail...>
// -----------------------------------------------------------------------------
// Recursive inheritance scheme. The element index is computed from
// the tail length so tuple_head<I, T> bases are distinct even when
// element types repeat.

template<typename    Head,
         typename... Tail>
class tuple<Head, Tail...>
    : private internal::tuple_head<sizeof...(Tail), Head>,
      private tuple<Tail...>
{
private:
    typedef internal::tuple_head<sizeof...(Tail), Head> _head_base;
    typedef tuple<Tail...>                                _tail_base;

public:
    // ---------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------

    // 1) Default constructor.
    //    Value-initialises every element. Requires every element type
    //    to be default-constructible.
    RE_STD_CONSTEXPR
    tuple()
        : _head_base(),
          _tail_base()
    {}

    // 2) Direct constructor.
    //    Initialises each element from the corresponding argument.
    RE_STD_CONSTEXPR
    tuple(
        const Head&    _h,
        const Tail&... _t
    )
        : _head_base(_h),
          _tail_base(_t...)
    {}

    // 3) Converting constructor (perfect forwarding).
    template<typename    UHead,
             typename... UTail,
             typename = typename enable_if<
                 ( sizeof...(UTail) == sizeof...(Tail) &&
                   is_constructible<Head, UHead&&>::value )
             >::type>
    RE_STD_CONSTEXPR
    tuple(
        UHead&&    _h,
        UTail&&... _t
    )
        : _head_base(static_cast<UHead&&>(_h)),
          _tail_base(static_cast<UTail&&>(_t)...)
    {}

    // 4) Converting copy constructor.
    template<typename    UHead,
             typename... UTail,
             typename = typename enable_if<
                 ( sizeof...(UTail) == sizeof...(Tail) &&
                   is_constructible<Head, const UHead&>::value )
             >::type>
    RE_STD_CONSTEXPR
    tuple(
        const tuple<UHead, UTail...>& _other
    )
        : _head_base(_other.head_ref()),
          _tail_base(_other.tail_ref())
    {}

    // 5) Converting move constructor.
    template<typename    UHead,
             typename... UTail,
             typename = typename enable_if<
                 ( sizeof...(UTail) == sizeof...(Tail) &&
                   is_constructible<Head, UHead&&>::value )
             >::type>
    RE_STD_CONSTEXPR
    tuple(
        tuple<UHead, UTail...>&& _other
    )
        : _head_base(static_cast<UHead&&>(_other.head_ref())),
          _tail_base(static_cast<tuple<UTail...>&&>(_other.tail_ref()))
    {}

    // 6) Pair-converting copy constructor (2-element tuples only).
    //    Initialises from pair.first / pair.second. SFINAE-restricted
    // to 2-element tuples (sizeof...(Tail) == 1) so it doesn't fire
    // for other arities. Requires pair to be complete at the point
    // of instantiation (via #include "../utility/pair.hpp" in user
    // code or via pair_tuple_size.hpp / pair_get.hpp).
    template<typename U1,
             typename U2,
             typename = typename enable_if<
                 ( sizeof...(Tail) == 1 &&
                   is_constructible<Head, const U1&>::value )
             >::type>
    RE_STD_CONSTEXPR
    tuple(
        const pair<U1, U2>& _p
    )
        : _head_base(_p.first),
          _tail_base(_p.second)
    {}

    // 7) Pair-converting move constructor (2-element tuples only).
    //    Same shape as (6) but rvalue-extracting pair.first / .second.
    template<typename U1,
             typename U2,
             typename = typename enable_if<
                 ( sizeof...(Tail) == 1 &&
                   is_constructible<Head, U1&&>::value )
             >::type>
    RE_STD_CONSTEXPR
    tuple(
        pair<U1, U2>&& _p
    )
        : _head_base(static_cast<U1&&>(_p.first)),
          _tail_base(static_cast<U2&&>(_p.second))
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
        head_ref() = static_cast<Head&&>(_other.head_ref());
        tail_ref() = static_cast<_tail_base&&>(_other.tail_ref());
        return *this;
    }

    // Copy constructor, EXPLICITLY DEFAULTED.
    //
    //   Declaring the move assignment operator above suppresses the
    // implicit copy constructor -- it is defined as DELETED, not merely
    // left undeclared -- so without this line no re_std::tuple was
    // copyable at all. The class still parses and every use site fails
    // instead, which is why it went unnoticed. (Copy ASSIGNMENT is
    // hand-written further up and was unaffected, which is why tuple
    // looked half-working rather than obviously broken.)
    //
    //   Defaulting is right here: each base is copyable exactly when its
    // element is, so the generated definition inherits the correct
    // constrained behaviour rather than over-promising.
    tuple(const tuple&) = default;

    template<typename    UHead,
             typename... UTail>
    typename enable_if<
        sizeof...(UTail) == sizeof...(Tail),
        tuple&
    >::type
    operator=(
        const tuple<UHead, UTail...>& _other
    )
    {
        head_ref() = _other.head_ref();
        tail_ref() = _other.tail_ref();
        return *this;
    }

    template<typename    UHead,
             typename... UTail>
    typename enable_if<
        sizeof...(UTail) == sizeof...(Tail),
        tuple&
    >::type
    operator=(
        tuple<UHead, UTail...>&& _other
    )
    {
        head_ref() = static_cast<UHead&&>(_other.head_ref());
        tail_ref() = static_cast<tuple<UTail...>&&>(_other.tail_ref());
        return *this;
    }

    // Pair-converting copy assignment (2-element tuples only).
    //   Assigns head from pair.first and the single-element tail's
    // head from pair.second. SFINAE-restricted to sizeof...(Tail)
    // == 1 so it does not match for other arities.
    // NOTE: the arity test is routed through N, a template parameter of
    // THIS member, not through sizeof...(Tail) directly. A condition that
    // depends only on the enclosing class's parameters is already fixed by
    // the time the member is declared, so enable_if<false, ...> becomes a
    // hard error instead of quietly removing the overload -- which made
    // every tuple<A,B> ill-formed on instantiation, and took tuple_cat
    // down with it.
    template<typename    U1,
             typename    U2,
             std::size_t N = sizeof...(Tail)>
    typename enable_if<
        N == 1,
        tuple&
    >::type
    operator=(
        const pair<U1, U2>& _p
    )
    {
        head_ref() = _p.first;
        tail_ref().head_ref() = _p.second;
        return *this;
    }

    // Pair-converting move assignment (2-element tuples only).
    template<typename    U1,
             typename    U2,
             std::size_t N = sizeof...(Tail)>
    typename enable_if<
        N == 1,
        tuple&
    >::type
    operator=(
        pair<U1, U2>&& _p
    )
    {
        head_ref() = static_cast<U1&&>(_p.first);
        tail_ref().head_ref() = static_cast<U2&&>(_p.second);
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
        Head tmp(static_cast<Head&&>(head_ref()));
        head_ref()         = static_cast<Head&&>(_other.head_ref());
        _other.head_ref()  = static_cast<Head&&>(tmp);
        tail_ref().swap(_other.tail_ref());
        return;
    }

    // ---------------------------------------------------------------
    // Internal accessors used by get() and by converting ctors of
    // sibling tuple instantiations. NOT part of the public API.
    // ---------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14
    Head&
    head_ref() RE_STD_NOEXCEPT
    {
        return _head_base::head();
    }

    RE_STD_CONSTEXPR
    const Head&
    head_ref() const RE_STD_NOEXCEPT
    {
        return _head_base::head();
    }

    RE_STD_CONSTEXPR_CPP14
    _tail_base&
    tail_ref() RE_STD_NOEXCEPT
    {
        return *this;
    }

    RE_STD_CONSTEXPR
    const _tail_base&
    tail_ref() const RE_STD_NOEXCEPT
    {
        return *this;
    }
};


}  // re_std -- inside the gate, like the opening it closes

#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_TUPLE_HPP
