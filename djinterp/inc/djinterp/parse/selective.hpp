/******************************************************************************
* djinterp [functional]                                          selective.hpp
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
* <F, _Enable = void>` with the SFINAE hook so a monad bridge picks
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
*   II.   selective_traits<F, _Enable>       protocol
*   III.  is_selective + Selective concept
*   IV.   selective_traits<M>  monad bridge  every monad is selective
*   V.    selective_select                   protocol entry point
*   VI.   selective_branch / selective_if_s / selective_when_s
*                                            derived combinators
*
*
* path:      /inc/djinterp/parse/selective.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_SELECTIVE_
#define DJINTERP_FUNCTIONAL_SELECTIVE_ 1

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
// default _L.  Comparison is by tag-then-arm.
template<typename _L,
         typename _R>
class either
{
public:
    using left_type  = _L;
    using right_type = _R;

    either()
        : m_is_left(true),
          m_left   (),
          m_right  ()
    {}

    D_NODISCARD
    static either
    left(
        const _L& _l
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
        const _R& _r
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
    const _L& left_value() const
    {
        return m_left;
    }

    D_NODISCARD
    const _R& right_value() const
    {
        return m_right;
    }

private:
    bool m_is_left;
    _L   m_left;
    _R   m_right;
};

template<typename _L,
         typename _R>
inline bool
operator==(
    const either<_L, _R>& _a,
    const either<_L, _R>& _b
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

template<typename _L,
         typename _R>
inline bool
operator!=(
    const either<_L, _R>& _a,
    const either<_L, _R>& _b
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
//     template<typename _U> using rebind = F<_U>;
//
//     // select : F<either<L, R>> -> F<L -> R> -> F<R>
//     template<typename _L, typename _R, typename _FunctionEffect>
//     static F<_R> select(
//         const F<either<_L, _R>>& fab,
//         const _FunctionEffect&   ff
//     );
//
//   The second template parameter is a SFINAE hook so a monad-to-
// selective bridge can supply the protocol for every Monad without
// per-type specialisation.  The primary is left undefined so use
// on a non-selective produces a clean resolution error.
template<typename _Selective,
         typename _Enable = void>
struct selective_traits;


// ================================================================
//  III. is_selective + Selective concept
// ================================================================

NS_INTERNAL

    // is_selective_helper
    //   helper: SFINAE detector for whether selective_traits<T> is
    // specialised.  Looks for the is_specialized marker every
    // specialisation provides.
    template<typename _Type>
    struct is_selective_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename selective_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal

// is_selective
//   trait: true iff _Type has a specialised selective_traits.
template<typename _Type>
struct is_selective
    : internal::is_selective_helper<
          typename std::decay<_Type>::type>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    static constexpr bool is_selective_v =
        is_selective<_Type>::value;
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Selective
    //   concept: satisfied when _Type is a specialised selective.
    template<typename _Type>
    concept Selective = is_selective<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// ================================================================
//  IV.  selective_traits<_M>  monad bridge
// ================================================================

// selective_traits<_Monad>
//   specialisation: every monad is a selective.  select is bind
// followed by case-analysis: on Right return pure(r), on Left bind
// the handler and apply it.  Provided here so maybe, result,
// parser, and any future monad participate as selectives with no
// per-type code.  Concrete carriers wishing a non-monadic selective
// instance supply a non-overlapping specialisation.
template<typename _Monad>
struct selective_traits<
    _Monad,
    typename std::enable_if<is_monad<_Monad>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename monad_value_type<_Monad>::type;

    template<typename _U>
    using rebind = typename monad_rebind<_Monad, _U>::type;

    // select
    //   selective select via monadic bind.
    template<typename _L,
             typename _R,
             typename _FunctionEffect>
    static
    typename monad_rebind<_Monad, _R>::type
    select(
        const typename monad_rebind<
            _Monad, either<_L, _R>>::type& _disc,
        const _FunctionEffect&             _handler
    )
    {
        using r_effect = typename monad_rebind<_Monad, _R>::type;

        return ::djinterp::monad_bind(
            _disc,
            [_handler](const either<_L, _R>& _e) -> r_effect
            {
                if (_e.is_right())
                {
                    return ::djinterp::pure<r_effect>(
                        _e.right_value());
                }

                _L l_val = _e.left_value();

                return ::djinterp::monad_bind(
                    _handler,
                    [l_val](const std::function<_R(_L)>& _fn)
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
template<typename _DiscEffect,
         typename _FunctionEffect>
D_NODISCARD
auto selective_select(
    const _DiscEffect&     _disc,
    const _FunctionEffect& _handler
)
-> decltype(selective_traits<_DiscEffect>::select(
       _disc, _handler))
{
    static_assert(is_selective<_DiscEffect>::value,
                  "selective_select: _DiscEffect must be a "
                  "registered selective");

    return selective_traits<_DiscEffect>::select(_disc, _handler);
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
template<typename _DiscEffect,
         typename _LeftEffect,
         typename _RightEffect>
D_NODISCARD
auto selective_branch(
    const _DiscEffect&  _disc,
    const _LeftEffect&  _fl,
    const _RightEffect& _fr
)
-> typename monad_rebind<
       _DiscEffect,
       typename std::decay<decltype(
           std::declval<typename _LeftEffect::value_type>()(
               std::declval<typename _DiscEffect::value_type
                   ::left_type>()))>::type>::type
{
    using disc_value = typename _DiscEffect::value_type;
    using L = typename disc_value::left_type;
    using R = typename disc_value::right_type;
    using C = typename std::decay<decltype(
        std::declval<typename _LeftEffect::value_type>()(
            std::declval<L>()))>::type;
    using either_RC  = either<R, C>;
    using mid_effect =
        typename monad_rebind<_DiscEffect, either_RC>::type;
    using r_effect =
        typename monad_rebind<_DiscEffect, C>::type;

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
template<typename _CondEffect,
         typename _ThenEffect,
         typename _ElseEffect>
D_NODISCARD
auto selective_if_s(
    const _CondEffect& _cond,
    const _ThenEffect& _then,
    const _ElseEffect& _else
)
-> _ThenEffect
{
    static_assert(is_selective<_CondEffect>::value,
                  "selective_if_s: _CondEffect must be selective");

    return ::djinterp::monad_bind(
        _cond,
        [_then, _else](bool _b) -> _ThenEffect
        {
            return _b ? _then : _else;
        });
}


// selective_when_s
//   function: whenS — run the effect only when the condition is
// true; otherwise leave it.  Returns a bool indicating whether the
// effect was run.
template<typename _CondEffect,
         typename _Effect>
D_NODISCARD
auto selective_when_s(
    const _CondEffect& _cond,
    const _Effect&     _eff
)
-> typename monad_rebind<_CondEffect, bool>::type
{
    using r_effect = typename monad_rebind<_CondEffect, bool>::type;

    return ::djinterp::monad_bind(
        _cond,
        [_eff](bool _b) -> r_effect
        {
            if (_b)
            {
                return ::djinterp::monad_bind(
                    _eff,
                    [](const typename _Effect::value_type& /*_v*/)
                        -> r_effect
                    {
                        return ::djinterp::pure<r_effect>(true);
                    });
            }

            return ::djinterp::pure<r_effect>(false);
        });
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_SELECTIVE_
