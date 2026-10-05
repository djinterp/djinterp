/*******************************************************************************
* djinterp [parse]                                                 selective.hpp
*
* Selective protocol: select, branch, ifS — between Applicative and Monad.
*   A Selective applicative sits between Applicative and Monad in the
* power hierarchy.  Applicative composes effects in lock-step; Monad
* lets later effects depend on earlier values; Selective allows
* *static* branching — choosing between two effects based on the
* shape (Left vs Right) of an earlier result, while keeping both
* branches statically visible.  The single obligation is
*
*     select :: f (either L R) -> f (L -> R) -> f R
*
* — given an effectful Either and an effectful handler that knows
* how to turn an L into an R, run the discriminant first; if it is
* Right r, keep r; if it is Left l, run the handler and apply it.
* Crucially the handler remains a static F-action regardless of
* which arm is taken: both arms remain in the program structure
* even when only one is executed, which is what makes Selective the
* maximal-inspection branching layer.
*
*   `branch` and `ifS` are derived once from `select`; the protocol
* trait specialises like the other algebra protocols (functor,
* applicative, monad, alternative, traversable) — `selective_traits
* <F, Enable = void>` with the SFINAE hook so a monad bridge picks
* up every Monad as a Selective without per-type code.
*
*   NOTE on the free construction.  The formal `free_selective<F,
* A>` (= the free Selective from any Functor F) requires hiding an
* existential X behind the Select constructor, which in C++ needs
* a virtual-node-dispatch pattern that fully type-erases X while
* still supporting downstream map / ap operations.  That elaboration
* is intentionally deferred: this header carries the protocol and
* generic operations only, and downstream code wanting a free
* construction over F can instead express the same selective
* programs via this header's `selective_select` on any Monad
* (e.g. `parser<R, E>`).  When `free_selective<F, A>` lands as a
* follow-up, no call site that uses the protocol surface here will
* change.
*
* CONTENTS
*   I.    either<L, R>                       discriminated union
*   II.   selective_traits<F, Enable>       protocol
*   III.  is_selective + Selective concept
*   IV.   selective_traits<M>  monad bridge  every monad is selective
*   V.    selective_select                   protocol entry point
*   VI.   selective_branch / selective_if_s / selective_when_s
*                                            derived combinators
*
*
* path:      /inc/djinterp/parse/selective.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSE_SELECTIVE_HPP
#define DJINTERP_PARSE_SELECTIVE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <functional>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../core/functional/functor.hpp"
#include "../core/functional/applicative.hpp"
#include "../core/functional/monad.hpp"


NS_DJINTERP


// ================================================================
//  I.   either<L, R>
// ================================================================

// either
//   class: a discriminated union with a distinguished Left and a
// Right arm.  The Selective protocol's discriminant — select
// chooses behaviour based on whether the value is Left or Right.
//
//   Value-semantic, copyable, default-constructs to a Left of a
// default L.  Comparison is by tag-then-arm.
template<typename L,
         typename R>
class either
{
public:
    using left_type  = L;
    using right_type = R;

    either()
        : m_is_left(true),
          m_left   (),
          m_right  ()
    {}

    D_NODISCARD
    static either
    left(
        const L& _l
    )
    {
        either e;
        e.m_is_left = true;
        e.m_left    = _l;
        return e;
    }

    D_NODISCARD
    static either
    right(
        const R& _r
    )
    {
        either e;
        e.m_is_left = false;
        e.m_right   = _r;
        return e;
    }

    D_NODISCARD
    bool is_left() const D_NOEXCEPT
    {
        return m_is_left;
    }

    D_NODISCARD
    bool is_right() const D_NOEXCEPT
    {
        return !m_is_left;
    }

    D_NODISCARD
    const L& left_value() const
    {
        return m_left;
    }

    D_NODISCARD
    const R& right_value() const
    {
        return m_right;
    }

private:
    bool m_is_left;
    L    m_left;
    R    m_right;
};

template<typename L,
         typename R>
inline bool
operator==(
    const either<L, R>& _a,
    const either<L, R>& _b
)
{
    if (_a.is_left() != _b.is_left())
    {
        return false;
    }

    if (_a.is_left())
    {
        return (_a.left_value() == _b.left_value());
    }

    return (_a.right_value() == _b.right_value());
}

template<typename L,
         typename R>
inline bool
operator!=(
    const either<L, R>& _a,
    const either<L, R>& _b
)
{
    return (!(_a == _b));
}


// ================================================================
//  II.  selective_traits  (primary)
// ================================================================

// selective_traits
//   trait: protocol for the Selective applicative.  A conforming
// specialisation provides
//
//     using is_specialized = std::true_type;
//     using value_type     = ...                       inner A
//     template<typename U> using rebind = F<U>;
//
//     // select : F<either<L, R>> -> F<L -> R> -> F<R>
//     template<typename L, typename R, typename FunctionEffect>
//     static F<R> select(
//         const F<either<L, R>>& fab,
//         const FunctionEffect&   ff
//     );
//
//   The second template parameter is a SFINAE hook so a monad-to-
// selective bridge can supply the protocol for every Monad without
// per-type specialisation.  The primary is left undefined so use
// on a non-selective produces a clean resolution error.
template<typename Selective,
         typename Enable = void>
struct selective_traits;


// ================================================================
//  III. is_selective + Selective concept
// ================================================================

NS_INTERNAL

    // is_selective_helper
    //   helper: SFINAE detector for whether selective_traits<T> is
    // specialised.  Looks for the is_specialized marker every
    // specialisation provides.
    template<typename Type>
    struct is_selective_helper
    {
    private:
        template<typename T>
        static auto test(int)
            -> decltype(
                typename selective_traits<T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Type>(0));
    };

NS_END  // internal

// is_selective
//   trait: true iff Type has a specialised selective_traits.
template<typename Type>
struct is_selective
    : internal::is_selective_helper<
          typename std::decay<Type>::type>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    static constexpr bool is_selective_v =
        is_selective<Type>::value;
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Selective
    //   concept: satisfied when Type is a specialised selective.
    template<typename Type>
    concept Selective = is_selective<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// ================================================================
//  IV.  selective_traits<M>  monad bridge
// ================================================================

// selective_traits<Monad>
//   specialisation: every monad is a selective.  select is bind
// followed by case-analysis: on Right return pure(r), on Left bind
// the handler and apply it.  Provided here so maybe, result,
// parser, and any future monad participate as selectives with no
// per-type code.  Concrete carriers wishing a non-monadic selective
// instance supply a non-overlapping specialisation.
template<typename Monad>
struct selective_traits<
    Monad,
    typename std::enable_if<is_monad<Monad>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename monad_value_type<Monad>::type;

    template<typename U>
    using rebind = typename monad_rebind<Monad, U>::type;

    // select
    //   selective select via monadic bind.
    template<typename L,
             typename R,
             typename FunctionEffect>
    static
    typename monad_rebind<Monad, R>::type
    select(
        const typename monad_rebind<
            Monad, either<L, R>>::type& _disc,
        const FunctionEffect&             _handler
    )
    {
        using r_effect = typename monad_rebind<Monad, R>::type;

        return ::djinterp::monad_bind(
            _disc,
            [_handler](const either<L, R>& _e) -> r_effect
            {
                if (_e.is_right())
                {
                    return ::djinterp::pure<r_effect>(
                        _e.right_value());
                }

                L l_val = _e.left_value();

                return ::djinterp::monad_bind(
                    _handler,
                    [l_val](const std::function<R(L)>& _fn)
                        -> r_effect
                    {
                        return ::djinterp::pure<r_effect>(
                            _fn(l_val));
                    });
            });
    }
};


// ================================================================
//  V.   selective_select
// ================================================================

// selective_select
//   function: the protocol entry point.  Delegates to
// selective_traits<F>::select after verifying participation.
template<typename DiscEffect,
         typename FunctionEffect>
D_NODISCARD
auto selective_select(
    const DiscEffect&     _disc,
    const FunctionEffect& _handler
)
-> decltype(selective_traits<DiscEffect>::select(
       _disc, _handler))
{
    static_assert(is_selective<DiscEffect>::value,
                  "selective_select: DiscEffect must be a "
                  "registered selective");

    return selective_traits<DiscEffect>::select(_disc, _handler);
}


// ================================================================
//  VI.  selective_branch / selective_if_s / selective_when_s
// ================================================================

// selective_branch
//   function: dual face of select — given a discriminant F<either<
// L, R>> and two handler effects F<L → C> and F<R → C>, run the
// appropriate handler depending on the arm.  Implemented by two
// nested selects: the first reshapes the Either so the second can
// route the surviving arm through the Right handler.
template<typename DiscEffect,
         typename LeftEffect,
         typename RightEffect>
D_NODISCARD
auto selective_branch(
    const DiscEffect&  _disc,
    const LeftEffect&  _fl,
    const RightEffect& _fr
)
-> typename monad_rebind<
       DiscEffect,
       typename std::decay<decltype(
           std::declval<typename LeftEffect::value_type>()(
               std::declval<typename DiscEffect::value_type
                   ::left_type>()))>::type>::type
{
    using disc_value = typename DiscEffect::value_type;
    using L = typename disc_value::left_type;
    using R = typename disc_value::right_type;
    using C = typename std::decay<decltype(
        std::declval<typename LeftEffect::value_type>()(
            std::declval<L>()))>::type;
    using either_RC  = either<R, C>;
    using mid_effect =
        typename monad_rebind<DiscEffect, either_RC>::type;
    using r_effect =
        typename monad_rebind<DiscEffect, C>::type;

    // First pass: bind on _disc, route Left through _fl into
    // Right(C), and Right(r) into Left(r) for the next select.
    mid_effect mid = ::djinterp::monad_bind(
        _disc,
        [_fl](const disc_value& _e) -> mid_effect
        {
            if (_e.is_left())
            {
                L l_val = _e.left_value();

                return ::djinterp::monad_bind(
                    _fl,
                    [l_val](const std::function<C(L)>& _fn)
                        -> mid_effect
                    {
                        return ::djinterp::pure<mid_effect>(
                            either_RC::right(_fn(l_val)));
                    });
            }

            return ::djinterp::pure<mid_effect>(
                either_RC::left(_e.right_value()));
        });

    // Second pass: select on mid with _fr.
    return selective_select(mid, _fr);
}


// selective_if_s
//   function: ifS — a Boolean-discriminated branch.  Given F<bool>,
// F<A>, F<A>, return whichever branch the bool selects.  Unlike
// the pure-applicative `if`, only the chosen branch is run.
template<typename CondEffect,
         typename ThenEffect,
         typename ElseEffect>
D_NODISCARD
auto selective_if_s(
    const CondEffect& _cond,
    const ThenEffect& _then,
    const ElseEffect& _else
)
-> ThenEffect
{
    static_assert(is_selective<CondEffect>::value,
                  "selective_if_s: CondEffect must be selective");

    return ::djinterp::monad_bind(
        _cond,
        [_then, _else](bool _b) -> ThenEffect
        {
            return _b ? _then : _else;
        });
}


// selective_when_s
//   function: whenS — run the effect only when the condition is
// true; otherwise leave it.  Returns a bool indicating whether the
// effect was run.
template<typename CondEffect,
         typename Effect>
D_NODISCARD
auto selective_when_s(
    const CondEffect& _cond,
    const Effect&     _eff
)
-> typename monad_rebind<CondEffect, bool>::type
{
    using r_effect = typename monad_rebind<CondEffect, bool>::type;

    return ::djinterp::monad_bind(
        _cond,
        [_eff](bool _b) -> r_effect
        {
            if (_b)
            {
                return ::djinterp::monad_bind(
                    _eff,
                    [](const typename Effect::value_type& /*_v*/)
                        -> r_effect
                    {
                        return ::djinterp::pure<r_effect>(true);
                    });
            }

            return ::djinterp::pure<r_effect>(false);
        });
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_SELECTIVE_HPP
