/******************************************************************************
* djinterp [restd]                                  range_adaptor_closure.hpp
*
* range_adaptor_closure header:
*   Provides the C++23 range-adaptor-closure CRTP base and the
* operator| overloads that drive pipe-style adaptor composition:
*       r | C1 | C2 | C3  ==  C3(C2(C1(r)))
*
*   PORTABILITY:
*   - C++11+; closure-detection is SFINAE-based, no concept syntax.
*   - The CRTP base is empty; closures publicly derive from
*     range_adaptor_closure<DerivedClosure> to be detected.
*
*   THREE PARTS:
*   1. range_adaptor_closure<D> — empty CRTP marker.
*   2. is_range_adaptor_closure<T> — SFINAE trait that returns true
*      when T (after decay) publicly inherits from
*      range_adaptor_closure<that-decay>.
*   3. operator| overloads, two of them:
*      (a) (non-closure | closure) -> closure(non-closure).  When
*          the LHS is a range and the RHS is a closure, applies the
*          closure to the range.
*      (b) (closure | closure)     -> pipe_composition.  Composes
*          two closures into a new closure that, when invoked,
*          applies the first then the second.
*   4. pipe_composition<C1, C2> — itself a closure (derives from
*      range_adaptor_closure), so compositions chain.
*
*
* path:      /inc/djinterp/re_std/ranges/range_adaptor_closure.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_ADAPTOR_CLOSURE_
#define DJINTERP_RESTD_RANGES_RANGE_ADAPTOR_CLOSURE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"


NS_RESTD


// ===========================================================================
// I.   RANGE_ADAPTOR_CLOSURE  (CRTP marker base)
// ===========================================================================

// range_adaptor_closure<_Derived>
//   class: empty CRTP base. Closures derive from this to be
// recognised by the pipe-detection trait below.
// note: the C++23 standard adds a single helper member
// 'operator()' to this base that lets `closure(range)` work uniformly;
// restd's closures implement operator() directly on the derived
// class, so the base is purely a marker.
template<typename _Derived>
struct range_adaptor_closure
{
};


// ===========================================================================
// II.  IS_RANGE_ADAPTOR_CLOSURE  (SFINAE detection trait)
// ===========================================================================

NS_INTERNAL

// is_rac_helper
//   trait: SFINAE detection — test() is overloaded so that a
// pointer to a publicly-derived range_adaptor_closure<U> base is
// preferred; otherwise the catch-all overload kicks in. Result
// captured as a static bool.
template<typename _T>
class is_rac_helper
{
private:
    template<typename _U>
    static D_CONSTEXPR true_type
    test(range_adaptor_closure<_U> const*);

    static D_CONSTEXPR false_type
    test(...);

public:
    static const bool value =
        decltype(test(
            static_cast<typename decay<_T>::type*>(D_NULLPTR)
        ))::value;
};

NS_END  // internal


// is_range_adaptor_closure
//   trait: true when _T (after decay) publicly inherits from
// range_adaptor_closure<decay_t<_T>>.
template<typename _T>
struct is_range_adaptor_closure
    : integral_constant<bool, internal::is_rac_helper<_T>::value>
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_range_adaptor_closure_v
//   variable: convenience constexpr accessor.
template<typename _T>
D_CONSTEXPR bool is_range_adaptor_closure_v =
    is_range_adaptor_closure<_T>::value;

#endif  // variable templates


// ===========================================================================
// III. PIPE_COMPOSITION  (itself a closure)
// ===========================================================================

// pipe_composition<_C1, _C2>
//   class: holds two closures and applies them in sequence —
// operator()(_R) -> _C2(_C1(_R)). Deriving from
// range_adaptor_closure allows compositions to chain naturally:
// (r | a | b | c) parses as ((r | a) | b) | c, building a final
// view by binding from the left.
template<typename _C1,
         typename _C2>
struct pipe_composition : range_adaptor_closure<pipe_composition<_C1, _C2> >
{
    _C1 first;
    _C2 second;


    D_CONSTEXPR
    pipe_composition()
        : first(),
          second()
    {}

    D_CONSTEXPR
    pipe_composition(
        _C1  _c1,
        _C2  _c2
    )
        : first(static_cast<_C1&&>(_c1)),
          second(static_cast<_C2&&>(_c2))
    {}


    // operator()
    //   function: applies first then second. Trailing return type
    // is decltype of the actual chain so SFINAE applies on
    // invocation of an incompatible range.
    template<typename _R>
    D_CONSTEXPR
    auto
    operator()(_R&& _r) const
        -> decltype(second(first(static_cast<_R&&>(_r))))
    {
        return second(first(static_cast<_R&&>(_r)));
    }
};


// ===========================================================================
// IV.  OPERATOR|  (the two pipe overloads)
// ===========================================================================

// operator| (non-closure | closure)
//   function: when the LHS is anything OTHER than a range adaptor
// closure (i.e. presumed to be a range), apply the RHS closure to
// it. The non-closure SFINAE constraint avoids overload-resolution
// ambiguity with the closure|closure form.
template<typename _LHS,
         typename _RHS>
D_CONSTEXPR
typename enable_if<
    is_range_adaptor_closure<_RHS>::value
        && !is_range_adaptor_closure<_LHS>::value,
    decltype(declval<_RHS>()(declval<_LHS>()))
>::type
operator|(
    _LHS&& _lhs,
    _RHS&& _rhs
)
{
    return static_cast<_RHS&&>(_rhs)(static_cast<_LHS&&>(_lhs));
}


// operator| (closure | closure)
//   function: when both sides are closures, build a
// pipe_composition that applies the LHS first and the RHS second
// when invoked.
template<typename _LHS,
         typename _RHS>
D_CONSTEXPR
typename enable_if<
    is_range_adaptor_closure<_LHS>::value
        && is_range_adaptor_closure<_RHS>::value,
    pipe_composition<typename decay<_LHS>::type,
                     typename decay<_RHS>::type>
>::type
operator|(
    _LHS&& _lhs,
    _RHS&& _rhs
)
{
    return pipe_composition<typename decay<_LHS>::type,
                            typename decay<_RHS>::type>(
        static_cast<_LHS&&>(_lhs),
        static_cast<_RHS&&>(_rhs)
    );
}


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_RANGE_ADAPTOR_CLOSURE_
