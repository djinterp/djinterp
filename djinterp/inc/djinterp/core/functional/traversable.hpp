/*******************************************************************************
* djinterp [core]                                                traversable.hpp
*
* Traversable protocol: traverse and traverse_sequence (C++).
*   A traversable is a structure T<A> that can be walked left-to-right while
* running an applicative effect at each element, collecting the results back
* into the same shape: traverse(ta, f) takes f : A -> F<B> over an applicative
* F and yields F<T<B>>. It is the capstone of this layer -- it needs a structure
* that is both a Functor (to rebuild the shape) and a Foldable (to walk the
* elements), and an Applicative F (to thread and combine the effects), all of
* which the surrounding headers provide. traverse_sequence is the special case
* traverse(ta, identity): it turns T<F<A>> inside out into F<T<A>>.
*
*   The whole point is to run effects and invert the nesting. Traversing a
* maybe with a result-producing function turns maybe<A> into result<maybe<B>,E>
* -- the error is hoisted out, the optional preserved. Sequencing a vector of
* maybes, vector<maybe<A>>, yields maybe<vector<A>> that is just(...) only if
* every element was a value. For the effect F this layer ships maybe and result
* (the applicatives, via the monad bridge); the traversable structures are
* maybe, result, and -- materialising their elements -- the lazy sequences view
* and producer.
*
*   Because C++ has no native type classes, a traversable is recognized by
* specializing traversable_traits<T> with a single traverse.
* traverse_sequence is then
* derived once, generically. Specializations live in each structure's own
* header (maybe / result over their zero-or-one element; view / producer by
* folding the sequence and materialising into F<std::vector<B>>).
*
*   F (the effect) cannot be deduced from the value of an empty structure --
* a nothing or an empty sequence never calls f -- so it is recovered from the
* *type* of f's result, decltype(f(declval<A>())) = F<B>; the empty case then
* uses pure to inject the empty shape. This is why traverse works even when f
* is never invoked.
*
* USAGE:
*   using namespace djinterp;
*   // traverse a maybe with a result-producing function (hoist the error):
*   maybe<int> m = just(4);
*   result<maybe<int>, const char*> r =
*       traverse(m, [](int x) -> result<int, const char*> {
*           return x > 0 ? ok<int, const char*>(x * 2)
*                        : err<int, const char*>("neg");
*       });                                   // ok(just(8))
*
*   // traverse_sequence over a vector of maybes (all-or-nothing):
*   std::vector<maybe<int> > v{ just(1), just(2), just(3) };
*   maybe<std::vector<int> > s = traverse_sequence(v);     // just({1,2,3})
*
*
* path:      /inc/djinterp/core/functional/traversable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.11
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TRAVERSABLE PROTOCOL
      --------------------
      1.    traversable_traits<T>                   (primary, undefined)
      2.    is_traversable<T>                       (detection trait)

II.   GENERIC TRAVERSABLE OPERATIONS
      ------------------------------
      1.    traverse                                (the one obligation, delegated)
      2.    traverse_sequence                       (traverse with identity)
*/


#ifndef DJINTERP_FUNCTIONAL_TRAVERSABLE_HPP
#define DJINTERP_FUNCTIONAL_TRAVERSABLE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // void_t
#include "./functor.hpp"
#include "./applicative.hpp"
#include "./foldable.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    TRAVERSABLE PROTOCOL                                  ///
///////////////////////////////////////////////////////////////////////////////

// traversable_traits
//   trait: primary template, undefined by default. Each concrete
// traversable specializes traversable_traits<T> to expose:
//
//     - value_type      : the inner type A of T<A>
//     - traverse(ta, f)  : static -- walk T<A> running f : A -> F<B> and
//                         collect into F<T'<B>> (T' the rebuilt shape; for the
//                         lazy sequences, F<std::vector<B>>)
//     - is_specialized   = true_type (marker)
//
//   traverse is the whole obligation; traverse_sequence is derived from it.
// The second
// parameter is a SFINAE hook used by the family instances (view, producer)
// that key on a structural trait. The primary is left undefined so a use on a
// non-traversable produces a clean resolution error.
template<typename Traversable,
         typename Enable = void>
struct traversable_traits;


NS_INTERNAL

    // is_traversable_helper
    //   helper: SFINAE detector for whether traversable_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization provides.
    template<typename Type>
    struct is_traversable_helper
    {
    private:
        template<typename T>
        static auto test(int)
            -> decltype(
                typename traversable_traits<T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Type>(0));
    };

NS_END  // internal


// is_traversable
//   trait: true if Type has a specialization of traversable_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic operations.
template<typename Type>
struct is_traversable
    : internal::is_traversable_helper<typename std::decay<Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_traversable_v
//   value: convenience alias for is_traversable<Type>::value.
template<typename Type>
static constexpr bool is_traversable_v = is_traversable<Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   The inner value type of a traversable, mirroring functor_value_type /
// foldable_value_type, plus the C++20 concept. Internal helpers carry a unique
// traversable_ prefix to keep the umbrella build collision-free.

NS_INTERNAL

    // traversable_value_type_helper
    //   helper: SFINAE extractor for traversable_traits<T>::value_type.
    template<typename AlwaysVoid,
             typename Traversable>
    struct traversable_value_type_helper
    {};

    template<typename Traversable>
    struct traversable_value_type_helper<
        void_t<typename traversable_traits<Traversable>::value_type>,
        Traversable>
    {
        using type = typename traversable_traits<Traversable>::value_type;
    };

    // traversable_identity_helper
    //   helper: the identity function used to derive traverse_sequence from
    // traverse.
    // Returns its argument unchanged; a named functor (not a lambda) so it can
    // appear in trailing return types on every floor.
    struct traversable_identity_helper
    {
        template<typename X>
        D_CONSTEXPR
        X operator()(
            X _x
        ) const
        {
            return _x;
        }
    };

    // traversable_append_helper
    //   helper: the reducer used by the sequence traversables (view,
    // producer) to grow the materialised result vector inside the applicative
    // -- appends one element and returns the vector. Passed to lift_a2, so its
    // signature is (std::vector<B>, const B&) -> std::vector<B>. The vector is
    // threaded by value (lift_a2 lifts a pure binary function over the effect).
    template<typename Value>
    struct traversable_append_helper
    {
        D_CONSTEXPR_CPP14
        std::vector<Value> operator()(
            std::vector<Value> _acc,
            const Value&       _element
        ) const
        {
            _acc.push_back(_element);

            return _acc;
        }
    };

NS_END  // internal


// traversable_value_type
//   trait: the inner value type A of a traversable T. SFINAE-friendly.
template<typename Traversable>
struct traversable_value_type
{
    using type = typename internal::traversable_value_type_helper<
        void, typename std::decay<Traversable>::type>::type;
};

// traversable_value_type_t
//   type: convenience alias for traversable_value_type<T>::type.
template<typename Traversable>
using traversable_value_type_t =
    typename traversable_value_type<Traversable>::type;


// traversable_traits<std::vector<Type>>
//   instance: the canonical container traversable, companion to the
// std::vector foldable instance. A vector is walked left-to-right, f : A -> F<B>
// is run at each element, and the results are combined with lift_a2 into
// F<std::vector<B>>. Provided here because std::vector has no djinterp header.
// Written in the explicit two-argument `<T, void>` form.
template<typename Type>
struct traversable_traits<std::vector<Type>, void>
{
    using is_specialized = std::true_type;
    using value_type     = Type;

    // traverse
    //   F<std::vector<B>> from a std::vector<A> and f : A -> F<B>.
    template<typename Function>
    static
    D_CONSTEXPR_CPP14
    typename monad_rebind<
        decltype(std::declval<Function&>()(std::declval<const Type&>())),
        std::vector<applicative_value_type_t<decltype(
            std::declval<Function&>()(std::declval<const Type&>()))> > >::type
    traverse(
        const std::vector<Type>& _xs,
        Function                  _function
    )
    {
        using effect_t = decltype(
            _function(std::declval<const Type&>()));            // F<B>
        using inner_t  = applicative_value_type_t<effect_t>;     // B
        using vector_t = std::vector<inner_t>;
        using result_t = typename monad_rebind<effect_t, vector_t>::type;

        result_t _accumulator = ::djinterp::pure<result_t>(vector_t());

        for (typename std::vector<Type>::const_iterator _it = _xs.begin();
             _it != _xs.end();
             ++_it)
        {
            _accumulator = ::djinterp::lift_a2(
                _accumulator,
                _function(*_it),
                internal::traversable_append_helper<inner_t>());
        }

        return _accumulator;
    }
};


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Traversable
    //   concept: satisfied when Type is a specialized traversable. The
    // PascalCase typeclass face, alongside Functor / Applicative / Foldable.
    template<typename Type>
    concept Traversable = is_traversable<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC TRAVERSABLE OPERATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   traverse delegates to traversable_traits<T>::traverse; traverse_sequence is
// traverse with the identity function. Both fold at compile time under C++20
// wherever the instance's traverse (and the applicative effect F) do, and run
// at runtime otherwise.

// traverse
//   function: walks the traversable ta left-to-right, applying f : A -> F<B>
// at each element and collecting the effects into F<T'<B>> via the applicative
// F. The result type is whatever the instance produces, so it is deduced. For
// maybe / result the shape is preserved (F<maybe<B>> / F<result<B,E>>); for the
// lazy sequences the elements are materialised (F<std::vector<B>>).
template<typename Traversable,
         typename Function>
D_NODISCARD
D_CONSTEXPR
auto traverse
(
    Traversable&& _ta,
    Function       _function
)
-> decltype(traversable_traits<typename std::decay<Traversable>::type>::traverse(
       std::forward<Traversable>(_ta),
       _function))
{
    return traversable_traits<typename std::decay<Traversable>::type>::traverse(
        std::forward<Traversable>(_ta),
        _function);
}


// traverse_sequence
//   function: turns a traversable of effects inside out -- T<F<A>> -> F<T<A>>
// -- by traversing with the identity function. maybe<F<A>> becomes F<maybe<A>>;
// a vector<F<A>> becomes F<vector<A>>, succeeding only if every effect does.
template<typename Traversable>
D_NODISCARD
D_CONSTEXPR
auto traverse_sequence
(
    Traversable&& _ta
)
-> decltype(::djinterp::traverse(
       std::forward<Traversable>(_ta),
       internal::traversable_identity_helper()))
{
    return ::djinterp::traverse(
        std::forward<Traversable>(_ta),
        internal::traversable_identity_helper());
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_TRAVERSABLE_HPP
