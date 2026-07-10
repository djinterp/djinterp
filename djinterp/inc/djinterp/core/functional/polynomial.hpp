/******************************************************************************
* djinterp [functional]                                         polynomial.hpp
*
* Polynomial functors as first-class, foldable protocol citizens.
*   A polynomial functor F is built from the variable position (recursion),
* the unit, constants, sums, and products.  This header carries the
* value-level inhabitants of F -- the C++ things downstream code folds and
* traverses -- together with their Functor and Traversable instances, and the
* type-level shapes that document F's structure without a value payload.
*
*   WHY IT LIVES IN functional/.  These poly_* functors are the canonical base
* functors for recursion.hpp's cata / ana / hylo: `compose = cata[phi]`,
* `evaluate = cata[phi_eval]`, and any other interpretation are one
* catamorphism differing only in the algebra -- the structural recursion is
* written once, in recursion.hpp, not per carrier.  They also serve as the F
* parameter to functional::free.  Nothing here depends on a particular carrier
* (a grammar, an expression IR, an annotated AST): the parse layer's grammar
* headers build their productions on these, but so can any recursive datatype.
*
*   The Functor instances are what cata calls to rebuild a layer at the result
* type; the Traversable instances lift that to effectful folds (an F-algebra
* into a monad, e.g. accumulating diagnostics).  Both are specialised at
* djinterp:: scope, matching the protocol primaries.
*
*
* path:      /inc/djinterp/core/functional/polynomial.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    poly_var<X>                        the recursive position  F(X) = X
      poly_unit<X>                       the unit variant        F(X) = 1
      poly_const<C, X>                   a constant variant      F(X) = K_C
      poly_sum<L, R>                     binary sum              F    = L+R
      poly_product<F, G>                 binary product          F = First x
                                                                     Second
II.   type-level shapes (documentation only)
        poly_constant_t / poly_recursion_t /
        poly_sum_t / poly_product_t / poly_compose_t / poly_mu_t
III.  functor_traits specialisations     (djinterp:: scope)
IV.   traversable_traits specialisations  (djinterp:: scope)
*/

#ifndef DJINTERP_FUNCTIONAL_POLYNOMIAL_
#define DJINTERP_FUNCTIONAL_POLYNOMIAL_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/member_traits.hpp"
#include "./functor.hpp"
#include "./applicative.hpp"
#include "./monad.hpp"
#include "./traversable.hpp"
#include "./recursion.hpp"


NS_DJINTERP


// ================================================================
//  I.   value-level polynomial-functor vocabulary
// ================================================================
//   F is a polynomial functor: built from the variable position
// (recursion), the unit, constants, sums, and products.  The five
// types below are the value-level inhabitants of those builders:
// each takes its X parameter and carries the payload appropriate
// to its variant.  They are the C++ counterparts of the formal
// builders Var, 1, K_c, +, × — and they participate in
// functor_traits and traversable_traits (specialisations in
// sections IX and X), so generic algorithms like
// `compose = cata[φ]` and `functional::free<F, A>` work over them
// without per-carrier glue.

// poly_var
//   struct: the recursive position.  F(X) = X.  When F is applied
// to μF, an inhabitant of poly_var<μF> *is* the recursive subtree.
template<typename _X>
struct poly_var
{
    using value_type = _X;

    _X content;

    poly_var()
        : content()
    {}

    explicit poly_var(
        const _X& _content
    )
        : content(_content)
    {}
};


// poly_unit
//   struct: the unit variant.  F(X) = 1.  Carries no payload; the
// X parameter is phantom and exists only to satisfy the functor
// protocol's X-parameterisation.  Used to mark ε-productions or
// nullary constructors.
template<typename _X>
struct poly_unit
{
    using value_type = _X;

    poly_unit()
    {}
};


// poly_const
//   struct: a constant variant.  F(X) = K_C, a fixed value of type
// C with X phantom.  Used to embed terminal payloads (string
// fragments, token kinds, numeric literals) into F.
template<typename _C,
         typename _X>
struct poly_const
{
    using constant_type = _C;
    using value_type    = _X;

    _C content;

    poly_const()
        : content()
    {}

    explicit poly_const(
        const _C& _content
    )
        : content(_content)
    {}
};


// poly_sum
//   struct: a binary sum.  F(X) = L(X) + R(X).  Tagged union of two
// value-level polynomial functors evaluated at the same X.  Used to
// fold a multi-production nonterminal's alternatives into one F.
template<typename _Left,
         typename _Right>
struct poly_sum
{
    // value_type
    //   alias: the shared X variable of the two arms.  Compile
    // error if the arms disagree.
    using value_type = typename _Left::value_type;

    static_assert(
        std::is_same<
            typename _Left::value_type,
            typename _Right::value_type>::value,
        "poly_sum: left and right arms must agree on value_type");

    bool   is_left;
    _Left  left;
    _Right right;

    poly_sum()
        : is_left(true),
          left   (),
          right  ()
    {}

    static poly_sum
    inj_left(
        const _Left& _l
    )
    {
        poly_sum _r;
        _r.is_left = true;
        _r.left    = _l;
        return _r;
    }

    static poly_sum
    inj_right(
        const _Right& _r
    )
    {
        poly_sum _x;
        _x.is_left = false;
        _x.right   = _r;
        return _x;
    }
};


// poly_product
//   struct: a binary product.  F(X) = First(X) × Second(X).
// Encodes a production's RHS string (or a constructor's argument
// list) when that string has two components; n-ary products
// nest as right-associated pairs, matching the RHS-tuple
// presentation in `production`.
template<typename _First,
         typename _Second>
struct poly_product
{
    using value_type = typename _First::value_type;

    static_assert(
        std::is_same<
            typename _First::value_type,
            typename _Second::value_type>::value,
        "poly_product: first and second components must agree "
        "on value_type");

    _First  first;
    _Second second;

    poly_product()
        : first (),
          second()
    {}

    poly_product(
        const _First&  _first,
        const _Second& _second
    )
        : first (_first),
          second(_second)
    {}
};


// ================================================================
//  II.  type-level shapes  (documentation only)
// ================================================================
//   The builders below carry no value payload — they describe F's
// shape at the type level only.  Useful when generic code wants to
// pattern-match on F's structure (e.g. "is this a sum-of-products
// nonterminal?") without instantiating value-level inhabitants.
// Their value-level companions in section III are the carriers
// downstream code actually folds and traverses.

// poly_constant_t
//   meta: the type-level constant.  F(X) = K_C.
template<typename _Constant>
struct poly_constant_t
{
    using constant_type = _Constant;
};

// poly_recursion_t
//   meta: the type-level recursion marker.  F(X) = X.
struct poly_recursion_t
{};

// poly_sum_t
//   meta: the type-level sum.  F(X) = L(X) + R(X).
template<typename _L,
         typename _R>
struct poly_sum_t
{
    using left  = _L;
    using right = _R;
};

// poly_product_t
//   meta: the type-level product.  F(X) = F_1(X) × F_2(X) × … .
template<typename... _Variants>
struct poly_product_t
{
    using children = std::tuple<_Variants...>;

    D_STATIC_CONSTEXPR std::size_t arity = sizeof...(_Variants);
};

// poly_compose_t
//   meta: the type-level composition.  F(X) = Outer(Inner(X)).
template<typename _Outer,
         typename _Inner>
struct poly_compose_t
{
    using outer = _Outer;
    using inner = _Inner;
};

// poly_mu_t
//   meta: the initial algebra μF.  Documents that a carrier _D is
// ≅ μF for the named F; the isomorphism witnesses live elsewhere
// (parser leg + compose leg of the prism in prism.hpp).
template<typename _F>
struct poly_mu_t
{
    using functor = _F;
};




// ================================================================
//  III. functor_traits specialisations
//  IV.  traversable_traits specialisations
// ================================================================
//   The poly_* value-level types and the functor_traits /
// traversable_traits primary templates all live at djinterp:: scope,
// so the specialisations below are written directly.
//
//   For map / traverse, the X parameter is recovered by probing
// the supplied function's signature with decltype — the function
// is never invoked on a phantom or unit value.  This is the same
// pattern the maybe/result instances use for the empty / err arms.

// --- poly_var --------------------------------------------------

// functor_traits<poly_var<_X>>
//   specialisation: the identity functor.  map applies f to the
// stored value.
template<typename _X>
struct functor_traits<poly_var<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _U>
    using rebind = poly_var<_U>;

    template<typename _Function>
    static
    auto map(
        const poly_var<_X>& _fa,
        _Function                  _f
    )
    -> poly_var<
           typename std::decay<decltype(_f(_fa.content))>::type>
    {
        using out_t =
            typename std::decay<decltype(_f(_fa.content))>::type;

        return poly_var<out_t>(_f(_fa.content));
    }
};


// traversable_traits<poly_var<_X>>
//   specialisation: the identity functor as a Traversable.
// traverse(fa, f) runs f on the single value and rebuilds.
template<typename _X>
struct traversable_traits<poly_var<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _Function>
    static
    auto traverse(
        const poly_var<_X>& _fa,
        _Function                  _function
    )
    -> typename monad_rebind<
           decltype(_function(_fa.content)),
           poly_var<applicative_value_type_t<
               decltype(_function(_fa.content))>>
       >::type
    {
        using effect_t =
            decltype(_function(_fa.content));        // F<B>
        using inner_t  =
            applicative_value_type_t<effect_t>;      // B
        using shape_t  = poly_var<inner_t>;

        return ::djinterp::functor_map(
            _function(_fa.content),
            [](const inner_t& _x) -> shape_t
            {
                return shape_t(_x);
            });
    }
};


// --- poly_unit -------------------------------------------------

// functor_traits<poly_unit<_X>>
//   specialisation: the constant-at-1 functor.  map probes f for
// its return type and returns the unit at the new X.
template<typename _X>
struct functor_traits<poly_unit<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _U>
    using rebind = poly_unit<_U>;

    template<typename _Function>
    static
    auto map(
        const poly_unit<_X>& /*_fa*/,
        _Function                   _f
    )
    -> poly_unit<
           typename std::decay<decltype(
               _f(std::declval<_X>()))>::type>
    {
        using out_t =
            typename std::decay<decltype(
                _f(std::declval<_X>()))>::type;

        return poly_unit<out_t>();
    }
};


// traversable_traits<poly_unit<_X>>
//   specialisation: traverse never invokes _function (no X
// inhabitants); the effect is recovered from the function's
// declared return shape and the empty unit lifted via pure.
template<typename _X>
struct traversable_traits<poly_unit<_X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _Function>
    static
    auto traverse(
        const poly_unit<_X>& /*_fa*/,
        _Function                   /*_function*/
    )
    -> typename monad_rebind<
           decltype(std::declval<_Function&>()(
                                std::declval<const _X&>())),
           poly_unit<applicative_value_type_t<decltype(
               std::declval<_Function&>()(
                   std::declval<const _X&>()))>>
       >::type
    {
        using effect_t = decltype(
            std::declval<_Function&>()(
                std::declval<const _X&>()));
        using inner_t  = applicative_value_type_t<effect_t>;
        using shape_t  = poly_unit<inner_t>;
        using result_t =
            typename monad_rebind<effect_t, shape_t>::type;

        return ::djinterp::pure<result_t>(shape_t());
    }
};


// --- poly_const ------------------------------------------------

// functor_traits<poly_const<_C, _X>>
//   specialisation: the constant-at-C functor.  map preserves the
// constant payload; X is phantom.
template<typename _C,
         typename _X>
struct functor_traits<poly_const<_C, _X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _U>
    using rebind = poly_const<_C, _U>;

    template<typename _Function>
    static
    auto map(
        const poly_const<_C, _X>& _fa,
        _Function                        _f
    )
    -> poly_const<_C,
           typename std::decay<decltype(
               _f(std::declval<_X>()))>::type>
    {
        using out_t =
            typename std::decay<decltype(
                _f(std::declval<_X>()))>::type;

        return poly_const<_C, out_t>(_fa.content);
    }
};


// traversable_traits<poly_const<_C, _X>>
//   specialisation: traverse never invokes _function (X is
// phantom); the constant payload threads through unchanged.
template<typename _C,
         typename _X>
struct traversable_traits<poly_const<_C, _X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _X;

    template<typename _Function>
    static
    auto traverse(
        const poly_const<_C, _X>& _fa,
        _Function                        /*_function*/
    )
    -> typename monad_rebind<
           decltype(std::declval<_Function&>()(
                                std::declval<const _X&>())),
           poly_const<_C, applicative_value_type_t<decltype(
               std::declval<_Function&>()(
                   std::declval<const _X&>()))>>
       >::type
    {
        using effect_t = decltype(
            std::declval<_Function&>()(
                std::declval<const _X&>()));
        using inner_t  = applicative_value_type_t<effect_t>;
        using shape_t  = poly_const<_C, inner_t>;
        using result_t =
            typename monad_rebind<effect_t, shape_t>::type;

        return ::djinterp::pure<result_t>(
            shape_t(_fa.content));
    }
};


// --- poly_sum --------------------------------------------------

// functor_traits<poly_sum<_L, _R>>
//   specialisation: maps through the active arm.  Requires both
// arms to be Functors (asserted by the rebind requirements).
template<typename _L,
         typename _R>
struct functor_traits<poly_sum<_L, _R>, void>
{
    using is_specialized = std::true_type;
    using value_type     = typename _L::value_type;

    template<typename _U>
    using rebind = poly_sum<
        typename functor_traits<_L>::template rebind<_U>,
        typename functor_traits<_R>::template rebind<_U>>;

    template<typename _Function>
    static
    auto map(
        const poly_sum<_L, _R>& _fa,
        _Function                      _f
    )
    -> poly_sum<
           decltype(functor_traits<_L>::map(_fa.left,  _f)),
           decltype(functor_traits<_R>::map(_fa.right, _f))>
    {
        using new_l =
            decltype(functor_traits<_L>::map(_fa.left,  _f));
        using new_r =
            decltype(functor_traits<_R>::map(_fa.right, _f));
        using out_t = poly_sum<new_l, new_r>;

        if (_fa.is_left)
        {
            return out_t::inj_left(
                functor_traits<_L>::map(_fa.left, _f));
        }

        return out_t::inj_right(
            functor_traits<_R>::map(_fa.right, _f));
    }
};


// --- poly_product ----------------------------------------------

// functor_traits<poly_product<_F, _G>>
//   specialisation: maps both components.
template<typename _F,
         typename _G>
struct functor_traits<poly_product<_F, _G>, void>
{
    using is_specialized = std::true_type;
    using value_type     = typename _F::value_type;

    template<typename _U>
    using rebind = poly_product<
        typename functor_traits<_F>::template rebind<_U>,
        typename functor_traits<_G>::template rebind<_U>>;

    template<typename _Function>
    static
    auto map(
        const poly_product<_F, _G>& _fa,
        _Function                          _f
    )
    -> poly_product<
           decltype(functor_traits<_F>::map(_fa.first,  _f)),
           decltype(functor_traits<_G>::map(_fa.second, _f))>
    {
        using new_f =
            decltype(functor_traits<_F>::map(_fa.first,  _f));
        using new_g =
            decltype(functor_traits<_G>::map(_fa.second, _f));

        return poly_product<new_f, new_g>(
            functor_traits<_F>::map(_fa.first,  _f),
            functor_traits<_G>::map(_fa.second, _f));
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_POLYNOMIAL_