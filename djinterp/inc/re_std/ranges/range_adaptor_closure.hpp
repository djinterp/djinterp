/*******************************************************************************
* djinterp [re_std]                                    range_adaptor_closure.hpp
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
* path:      /inc/re_std/ranges/range_adaptor_closure.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_RANGE_ADAPTOR_CLOSURE_HPP
#define RE_STD_RANGES_RANGE_ADAPTOR_CLOSURE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"


namespace re_std
{


// ===========================================================================
// I.   RANGE_ADAPTOR_CLOSURE  (CRTP marker base)
// ===========================================================================

// range_adaptor_closure<Derived>
//   class: empty CRTP base. Closures derive from this to be
// recognised by the pipe-detection trait below.
// note: the C++23 standard adds a single helper member
// 'operator()' to this base that lets `closure(range)` work uniformly;
// re_std's closures implement operator() directly on the derived
// class, so the base is purely a marker.
template<typename Derived>
struct range_adaptor_closure
{
};


// ===========================================================================
// II.  IS_RANGE_ADAPTOR_CLOSURE  (SFINAE detection trait)
// ===========================================================================

namespace internal
{

// is_rac_helper
//   trait: SFINAE detection — test() is overloaded so that a
// pointer to a publicly-derived range_adaptor_closure<U> base is
// preferred; otherwise the catch-all overload kicks in. Result
// captured as a static bool.
template<typename T>
class is_rac_helper
{
private:
    template<typename U>
    static RE_STD_CONSTEXPR true_type
    test(range_adaptor_closure<U> const*);

    static RE_STD_CONSTEXPR false_type
    test(...);

public:
    static const bool value =
        decltype(test(
            static_cast<typename decay<T>::type*>(RE_STD_NULLPTR)
        ))::value;
};

}  // internal


// is_range_adaptor_closure
//   trait: true when T (after decay) publicly inherits from
// range_adaptor_closure<decay_t<T>>.
template<typename T>
struct is_range_adaptor_closure
    : integral_constant<bool, internal::is_rac_helper<T>::value>
{};


#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

// is_range_adaptor_closure_v
//   variable: convenience constexpr accessor.
template<typename T>
RE_STD_CONSTEXPR bool is_range_adaptor_closure_v =
    is_range_adaptor_closure<T>::value;

#endif  // variable templates


// ===========================================================================
// III. PIPE_COMPOSITION  (itself a closure)
// ===========================================================================

// pipe_composition<C1, C2>
//   class: holds two closures and applies them in sequence —
// operator()(R) -> C2(C1(R)). Deriving from
// range_adaptor_closure allows compositions to chain naturally:
// (r | a | b | c) parses as ((r | a) | b) | c, building a final
// view by binding from the left.
template<typename C1,
         typename C2>
struct pipe_composition : range_adaptor_closure<pipe_composition<C1, C2> >
{
    C1 first;
    C2 second;


    RE_STD_CONSTEXPR
    pipe_composition()
        : first(),
          second()
    {}

    RE_STD_CONSTEXPR
    pipe_composition(
        C1  _c1,
        C2  _c2
    )
        : first(static_cast<C1&&>(_c1)),
          second(static_cast<C2&&>(_c2))
    {}


    // operator()
    //   function: applies first then second. Trailing return type
    // is decltype of the actual chain so SFINAE applies on
    // invocation of an incompatible range.
    template<typename R>
    RE_STD_CONSTEXPR
    auto
    operator()(R&& _r) const
        -> decltype(second(first(static_cast<R&&>(_r))))
    {
        return second(first(static_cast<R&&>(_r)));
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
template<typename LHS,
         typename RHS>
RE_STD_CONSTEXPR
typename enable_if<
    is_range_adaptor_closure<RHS>::value
        && !is_range_adaptor_closure<LHS>::value,
    decltype(declval<RHS>()(declval<LHS>()))
>::type
operator|(
    LHS&& _lhs,
    RHS&& _rhs
)
{
    return static_cast<RHS&&>(_rhs)(static_cast<LHS&&>(_lhs));
}


// operator| (closure | closure)
//   function: when both sides are closures, build a
// pipe_composition that applies the LHS first and the RHS second
// when invoked.
template<typename LHS,
         typename RHS>
RE_STD_CONSTEXPR
typename enable_if<
    is_range_adaptor_closure<LHS>::value
        && is_range_adaptor_closure<RHS>::value,
    pipe_composition<typename decay<LHS>::type,
                     typename decay<RHS>::type>
>::type
operator|(
    LHS&& _lhs,
    RHS&& _rhs
)
{
    return pipe_composition<typename decay<LHS>::type,
                            typename decay<RHS>::type>(
        static_cast<LHS&&>(_lhs),
        static_cast<RHS&&>(_rhs)
    );
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_RANGE_ADAPTOR_CLOSURE_HPP
