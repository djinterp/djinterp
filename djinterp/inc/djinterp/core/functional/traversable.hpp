/******************************************************************************
* djinterp [functional]                                       traversable.hpp
*
* Traversable protocol: traverse and sequence (C++).
*   A traversable is a structure T<A> that can be walked left-to-right while
* running an applicative effect at each element, collecting the results back
* into the same shape: traverse(ta, f) takes f : A -> F<B> over an applicative
* F and yields F<T<B>>. It is the capstone of this layer -- it needs a structure
* that is both a Functor (to rebuild the shape) and a Foldable (to walk the
* elements), and an Applicative F (to thread and combine the effects), all of
* which the surrounding headers provide. sequence is the special case
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
* specializing traversable_traits<T> with a single traverse. sequence is then
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
*   // sequence a vector of maybes (all-or-nothing):
*   std::vector<maybe<int> > v{ just(1), just(2), just(3) };
*   maybe<std::vector<int> > s = sequence(v);     // just({1,2,3})
*
* 
* path:      /inc/djinterp/core/functional/traversable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TRAVERSABLE PROTOCOL
      1.  traversable_traits<T>                   (primary, undefined)
      2.  is_traversable<T>                       (detection trait)
II.   GENERIC TRAVERSABLE OPERATIONS
      1.  traverse                                (the one obligation, delegated)
      2.  sequence                                (traverse with identity)
*/


#ifndef DJINTERP_FUNCTIONAL_TRAVERSABLE_
#define DJINTERP_FUNCTIONAL_TRAVERSABLE_ 1

// std
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
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
//   traverse is the whole obligation; sequence is derived from it. The second
// parameter is a SFINAE hook used by the family instances (view, producer)
// that key on a structural trait. The primary is left undefined so a use on a
// non-traversable produces a clean resolution error.
template<typename _Traversable,
         typename _Enable = void>
struct traversable_traits;


NS_INTERNAL

    // is_traversable_helper
    //   helper: SFINAE detector for whether traversable_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization provides.
    template<typename _Type>
    struct is_traversable_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename traversable_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_traversable
//   trait: true if _Type has a specialization of traversable_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic operations.
template<typename _Type>
struct is_traversable
    : internal::is_traversable_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_traversable_v
//   value: convenience alias for is_traversable<_Type>::value.
template<typename _Type>
static constexpr bool is_traversable_v = is_traversable<_Type>::value;
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
    template<typename _AlwaysVoid,
             typename _Traversable>
    struct traversable_value_type_helper
    {};

    template<typename _Traversable>
    struct traversable_value_type_helper<
        void_t<typename traversable_traits<_Traversable>::value_type>,
        _Traversable>
    {
        using type = typename traversable_traits<_Traversable>::value_type;
    };

    // traversable_identity_helper
    //   helper: the identity function used to derive sequence from traverse.
    // Returns its argument unchanged; a named functor (not a lambda) so it can
    // appear in trailing return types on every floor.
    struct traversable_identity_helper
    {
        template<typename _X>
        D_CONSTEXPR
        _X operator()(
            _X _x
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
    template<typename _Value>
    struct traversable_append_helper
    {
        D_CONSTEXPR20
        std::vector<_Value> operator()(
            std::vector<_Value> _acc,
            const _Value&       _element
        ) const
        {
            _acc.push_back(_element);

            return _acc;
        }
    };

NS_END  // internal


// traversable_value_type
//   trait: the inner value type A of a traversable T. SFINAE-friendly.
template<typename _Traversable>
struct traversable_value_type
{
    using type = typename internal::traversable_value_type_helper<
        void, typename std::decay<_Traversable>::type>::type;
};

// traversable_value_type_t
//   type: convenience alias for traversable_value_type<T>::type.
template<typename _Traversable>
using traversable_value_type_t =
    typename traversable_value_type<_Traversable>::type;


// traversable_traits<std::vector<_Type>>
//   instance: the canonical container traversable, companion to the
// std::vector foldable instance. A vector is walked left-to-right, f : A -> F<B>
// is run at each element, and the results are combined with lift_a2 into
// F<std::vector<B>>. Provided here because std::vector has no djinterp header.
// Written in the explicit two-argument `<T, void>` form.
template<typename _Type>
struct traversable_traits<std::vector<_Type>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // traverse
    //   F<std::vector<B>> from a std::vector<A> and f : A -> F<B>.
    template<typename _Function>
    static
    D_CONSTEXPR20
    typename monad_rebind<
        decltype(std::declval<_Function&>()(std::declval<const _Type&>())),
        std::vector<applicative_value_type_t<decltype(
            std::declval<_Function&>()(std::declval<const _Type&>()))> > >::type
    traverse(
        const std::vector<_Type>& _xs,
        _Function                 _function
    )
    {
        using effect_t = decltype(
            _function(std::declval<const _Type&>()));            // F<B>
        using inner_t  = applicative_value_type_t<effect_t>;     // B
        using vector_t = std::vector<inner_t>;
        using result_t = typename monad_rebind<effect_t, vector_t>::type;

        result_t _accumulator = ::djinterp::pure<result_t>(vector_t());

        for (typename std::vector<_Type>::const_iterator _it = _xs.begin();
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
    //   concept: satisfied when _Type is a specialized traversable. The
    // PascalCase typeclass face, alongside Functor / Applicative / Foldable.
    template<typename _Type>
    concept Traversable = is_traversable<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC TRAVERSABLE OPERATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   traverse delegates to traversable_traits<T>::traverse; sequence is
// traverse with the identity function. Both fold at compile time under C++20
// wherever the instance's traverse (and the applicative effect F) do, and run
// at runtime otherwise.

// traverse
//   function: walks the traversable ta left-to-right, applying f : A -> F<B>
// at each element and collecting the effects into F<T'<B>> via the applicative
// F. The result type is whatever the instance produces, so it is deduced. For
// maybe / result the shape is preserved (F<maybe<B>> / F<result<B,E>>); for the
// lazy sequences the elements are materialised (F<std::vector<B>>).
template<typename _Traversable,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto traverse
(
    _Traversable&& _ta,
    _Function      _function
)
-> decltype(traversable_traits<typename std::decay<_Traversable>::type>::traverse(
       std::forward<_Traversable>(_ta),
       _function))
{
    return traversable_traits<typename std::decay<_Traversable>::type>::traverse(
        std::forward<_Traversable>(_ta),
        _function);
}


// sequence
//   function: turns a traversable of effects inside out -- T<F<A>> -> F<T<A>>
// -- by traversing with the identity function. maybe<F<A>> becomes F<maybe<A>>;
// a vector<F<A>> becomes F<vector<A>>, succeeding only if every effect does.
template<typename _Traversable>
D_NODISCARD
D_CONSTEXPR
auto sequence
(
    _Traversable&& _ta
)
-> decltype(::djinterp::traverse(
       std::forward<_Traversable>(_ta),
       internal::traversable_identity_helper()))
{
    return ::djinterp::traverse(
        std::forward<_Traversable>(_ta),
        internal::traversable_identity_helper());
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_TRAVERSABLE_
