/******************************************************************************
* djinterp [functional]                                           foldable.hpp
*
* Foldable protocol and the generic folds (C++).
*   A foldable here is a type constructor F<T> -- a context holding zero or
* more values of type T -- that can be collapsed to a single accumulated
* result by threading a binary reducer through its elements. Where Functor
* unified the per-type map, Foldable unifies the per-type fold: maybe<T> and
* result<T, E> fold over their zero-or-one carried value, a lazy view or a
* producer folds over its sequence, and one generic fold_left expresses all of
* them with a single call.
*
*   Because C++ has no native type classes, a foldable is recognized by
* specializing foldable_traits<F> with one operation, fold_left -- the strict
* left fold (acc, x) -> acc that matches reduce.hpp's runtime driver. That one
* obligation is enough: fold_right, fold_map, length, emptiness, and the
* any/all reductions are all derived once, generically, for every foldable.
* Specializations live in the concrete type's own header, exactly as the
* monad_traits / functor_traits specializations do.
*
*   Unlike Functor and Applicative, Foldable has no monad bridge: a fold is not
* expressible through unit and bind, so there is nothing to derive from
* is_monad. Each foldable supplies its own one-line fold_left instead -- maybe
* and result over their carried value, view and producer over their sequence.
* foldable.hpp therefore stands on the core header alone.
*
* USAGE:
*   maybe<int> m = just(21);
*   int n = fold_left(m, 0, [](int acc, int v){ return acc + v; });   // 21
*
*   // generic over ANY foldable: maybe, result, view, producer, ...
*   auto v   = fold_to_vector(some_view);          // std::vector<T>
*   auto len = fold_length(some_producer);         // std::size_t
*   bool ok  = fold_all(some_result,
*                       [](int x){ return x > 0; });
*
* 
* path:      /inc/djinterp/core/functional/foldable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
I.    FOLDABLE PROTOCOL
      1.  foldable_traits<F>                      (primary, undefined)
      2.  is_foldable<T>                          (detection trait)
II.   GENERIC FOLDABLE OPERATIONS
      1.  fold_left                               (the one obligation, delegated)
      2.  fold_right                              (derived; materialize + rev)
      3.  fold_map                                (map each elem then combine)
      4.  fold_to_vector                          (collect elements)
      5.  fold_length                             (element count)
      6.  fold_is_empty                           (no elements?)
      7.  fold_any / fold_all                     (existential / universal)
*/


#ifndef DJINTERP_FUNCTIONAL_FOLDABLE_
#define DJINTERP_FUNCTIONAL_FOLDABLE_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    FOLDABLE PROTOCOL                                     ///
///////////////////////////////////////////////////////////////////////////////

// foldable_traits
//   trait: primary template, undefined by default. Each concrete
// foldable specializes foldable_traits<F> to expose:
//
//     - value_type      : the inner type T of F<T>
//     - fold_left(fa, init, f) : static _Acc fold_left(const F&, _Acc, f)
//                         where f : (_Acc, const T&) -> _Acc, threading the
//                         accumulator left-to-right through the elements
//     - is_specialized  = true_type (marker)
//
//   fold_left is the whole obligation; every other fold below is derived
// from it generically. The second parameter is a SFINAE hook used by the
// type-family instances (view, producer) that key on a structural trait
// rather than a concrete template; concrete instances (maybe, result) leave
// it at the default. The primary is left undefined so a use on a
// non-foldable produces a clean resolution error.
template<typename _Foldable,
         typename _Enable = void>
struct foldable_traits;


NS_INTERNAL

    // is_foldable_helper
    //   helper: SFINAE detector for whether foldable_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization provides.
    template<typename _Type>
    struct is_foldable_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename foldable_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_foldable
//   trait: true if _Type has a specialization of foldable_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic foldable operations.
template<typename _Type>
struct is_foldable
    : internal::is_foldable_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_foldable_v
//   value: convenience alias for is_foldable<_Type>::value.
template<typename _Type>
static constexpr bool is_foldable_v = is_foldable<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the foldable protocol, built on
// the core ::void_t SFINAE sink declared in djinterp.hpp. is_foldable (above,
// kept where the protocol is introduced) answers "does a foldable_traits
// specialization exist?"; the trait here answers the finer-grained question
// generic code depends on: what is the inner value type. The C++20 concept
// closes the section. Internal helpers carry a unique foldable_ prefix so the
// umbrella build never collides them with the like-named helpers in
// monad.hpp / functor.hpp / applicative.hpp.

NS_INTERNAL

    // foldable_value_type_helper
    //   helper: SFINAE extractor for foldable_traits<F>::value_type
    // (primary: no `type`, soft failure).
    template<typename _AlwaysVoid,
             typename _Foldable>
    struct foldable_value_type_helper
    {};

    // foldable_value_type_helper (well-formed specialization)
    //   helper: yields foldable_traits<F>::value_type when present.
    template<typename _Foldable>
    struct foldable_value_type_helper<
        void_t<typename foldable_traits<_Foldable>::value_type>,
        _Foldable>
    {
        using type = typename foldable_traits<_Foldable>::value_type;
    };

NS_END  // internal


// foldable_value_type
//   trait: the inner value type T of a foldable F, i.e.
// foldable_traits<F>::value_type. SFINAE-friendly: has a `::type` only
// when F is a specialized foldable.
template<typename _Foldable>
struct foldable_value_type
{
    using type = typename internal::foldable_value_type_helper<
        void, typename std::decay<_Foldable>::type>::type;
};

// foldable_value_type_t
//   type: convenience alias for foldable_value_type<F>::type.
template<typename _Foldable>
using foldable_value_type_t = typename foldable_value_type<_Foldable>::type;


// foldable_traits<std::vector<_Type>>
//   instance: the canonical container foldable. A std::vector folds over its
// elements in order. This is provided here (rather than in a type header)
// because std::vector is a standard type with no djinterp header of its own,
// and it is the natural materialized foldable -- the target of fold_to_vector
// and the usual argument to mconcat. Written in the explicit two-argument
// `<T, void>` form against the SFINAE-hooked primary.
template<typename _Type>
struct foldable_traits<std::vector<_Type>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // fold_left
    //   strict left fold over the vector's elements; the accumulator is
    // threaded by move so collecting folds stay O(n). D_CONSTEXPR --
    // std::vector is a literal type only from C++20.
    template<typename _Acc,
             typename _Function>
    static
    D_CONSTEXPR
    _Acc fold_left(
        const std::vector<_Type>& _xs,
        _Acc                      _init,
        _Function                 _function
    )
    {
        for (typename std::vector<_Type>::const_iterator _it = _xs.begin();
             _it != _xs.end();
             ++_it)
        {
            _init = _function(std::move(_init), *_it);
        }

        return _init;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC FOLDABLE OPERATIONS                           ///
///////////////////////////////////////////////////////////////////////////////
//   DUAL DOMAIN. fold_left delegates to foldable_traits<F>::fold_left; under
// C++20 it folds at compile time over a foldable whose elements live in a
// constant expression, and runs at runtime otherwise -- the same conditional-
// constexpr behavior as monad_map / functor_map. The materializing folds
// (fold_right, fold_to_vector) build a std::vector and so are D_CONSTEXPR
// (std::vector is a literal type only from C++20). Every operation past
// fold_left is derived from it; none is a per-type obligation.

// fold_left
//   function: strict left fold. Threads _init through the elements of _fa
// left-to-right via _function : (_Acc, const T&) -> _Acc, returning the final
// accumulator. The result type is whatever the instance's fold_left produces,
// so it is deduced.
template<typename _Foldable,
         typename _Acc,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto fold_left
(
    _Foldable&& _fa,
    _Acc        _init,
    _Function&& _function
)
-> decltype(foldable_traits<typename std::decay<_Foldable>::type>::fold_left(
       std::forward<_Foldable>(_fa),
       std::move(_init),
       std::forward<_Function>(_function)))
{
    return foldable_traits<typename std::decay<_Foldable>::type>::fold_left(
        std::forward<_Foldable>(_fa),
        std::move(_init),
        std::forward<_Function>(_function));
}


NS_INTERNAL

    // foldable_push_helper
    //   helper: the reducer behind fold_to_vector -- appends each element to
    // the accumulating vector and threads it on by move (so the whole
    // collection is O(n)). A named functor (not a lambda) keeps the reducer
    // usable on every floor.
    template<typename _Value>
    struct foldable_push_helper
    {
        D_CONSTEXPR
        std::vector<_Value>
        operator()(
            std::vector<_Value> _acc,
            const _Value&       _element
        ) const
        {
            _acc.push_back(_element);

            return _acc;
        }
    };

    // foldable_count_helper
    //   helper: the reducer behind fold_length -- ignores the element and
    // increments the running count.
    template<typename _Value>
    struct foldable_count_helper
    {
        D_CONSTEXPR
        std::size_t
        operator()(
            std::size_t   _acc,
            const _Value& /*_element*/
        ) const
        {
            return _acc + 1;
        }
    };

    // foldable_emptiness_helper
    //   helper: the reducer behind fold_is_empty -- the first element seen
    // flips the accumulator to false.
    template<typename _Value>
    struct foldable_emptiness_helper
    {
        D_CONSTEXPR
        bool
        operator()(
            bool          /*_acc*/,
            const _Value& /*_element*/
        ) const
        {
            return false;
        }
    };

NS_END  // internal


// fold_right
//   function: right fold. Folds the elements with _function : (const T&,
// _Acc) -> _Acc, associating to the right. Implemented by materializing the
// elements (via fold_to_vector) and folding the buffer in reverse, so it is
// well-defined for any finite foldable; an infinite view / producer must be
// bounded first. D_CONSTEXPR because it builds a std::vector.
template<typename _Foldable,
         typename _Acc,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
typename std::decay<_Acc>::type
fold_right
(
    const _Foldable& _fa,
    _Acc&&           _init,
    _Function        _function
)
{
    using value_t = foldable_value_type_t<_Foldable>;
    using acc_t   = typename std::decay<_Acc>::type;

    std::vector<value_t> buffer =
        ::djinterp::fold_left(
            _fa,
            std::vector<value_t>(),
            internal::foldable_push_helper<value_t>());

    acc_t accumulator = std::forward<_Acc>(_init);

    for (typename std::vector<value_t>::const_reverse_iterator it =
             buffer.rbegin();
         it != buffer.rend();
         ++it)
    {
        accumulator = _function(*it, std::move(accumulator));
    }

    return accumulator;
}


// fold_map
//   function: maps each element to a monoid value via _function : T -> M and
// combines them with _combine : (M, M) -> M, starting from the identity
// _empty. The monoid (its identity and combine) is supplied explicitly, since
// the framework has no Monoid protocol yet. Left-associated, threaded by move.
template<typename _Foldable,
         typename _Function,
         typename _Monoid,
         typename _Combine>
D_NODISCARD
D_CONSTEXPR
_Monoid
fold_map
(
    const _Foldable& _fa,
    _Function        _function,
    _Monoid          _empty,
    _Combine         _combine
)
{
    using value_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        std::move(_empty),
        [_function, _combine](_Monoid _acc, const value_t& _element) -> _Monoid
        {
            return _combine(std::move(_acc), _function(_element));
        });
}


// fold_to_vector
//   function: collects the elements of a foldable into a std::vector, in fold
// (left-to-right) order. The materialized counterpart of the lazy views /
// producers, and the bridge any foldable can use to reach the eager helpers.
// D_CONSTEXPR because it builds a std::vector.
template<typename _Foldable>
D_NODISCARD
D_CONSTEXPR
std::vector<foldable_value_type_t<_Foldable>>
fold_to_vector
(
    const _Foldable& _fa
)
{
    using value_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        std::vector<value_t>(),
        internal::foldable_push_helper<value_t>());
}


// fold_length
//   function: the number of elements a foldable yields (0 or 1 for
// maybe / result; the sequence length for a view / producer). An infinite
// source must be bounded first.
template<typename _Foldable>
D_NODISCARD
D_CONSTEXPR
std::size_t
fold_length
(
    const _Foldable& _fa
)
{
    using value_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        std::size_t(0),
        internal::foldable_count_helper<value_t>());
}


// fold_is_empty
//   function: true when a foldable yields no elements (nothing / err, or an
// empty sequence). Does not short-circuit; an infinite source must be bounded.
template<typename _Foldable>
D_NODISCARD
D_CONSTEXPR
bool
fold_is_empty
(
    const _Foldable& _fa
)
{
    using value_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        true,
        internal::foldable_emptiness_helper<value_t>());
}


// fold_any
//   function: true when at least one element satisfies _predicate. Folds the
// disjunction; does not short-circuit (every element is visited).
template<typename _Foldable,
         typename _Predicate>
D_NODISCARD
D_CONSTEXPR
bool
fold_any
(
    const _Foldable& _fa,
    _Predicate       _predicate
)
{
    using value_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        false,
        [_predicate](bool _acc, const value_t& _element) -> bool
        {
            return _acc || static_cast<bool>(_predicate(_element));
        });
}


// fold_all
//   function: true when every element satisfies _predicate (vacuously true
// for an empty foldable). Folds the conjunction; does not short-circuit.
template<typename _Foldable,
         typename _Predicate>
D_NODISCARD
D_CONSTEXPR
bool
fold_all
(
    const _Foldable& _fa,
    _Predicate       _predicate
)
{
    using value_t = foldable_value_type_t<_Foldable>;

    return ::djinterp::fold_left(
        _fa,
        true,
        [_predicate](bool _acc, const value_t& _element) -> bool
        {
            return _acc && static_cast<bool>(_predicate(_element));
        });
}


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Foldable
    //   concept: satisfied when _Type is a specialized foldable. The
    // PascalCase typeclass face, alongside Functor / Applicative.
    template<typename _Type>
    concept Foldable = is_foldable<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FOLDABLE_
