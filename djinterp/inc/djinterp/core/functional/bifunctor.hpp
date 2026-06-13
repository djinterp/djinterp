/******************************************************************************
* djinterp [functional]                                         bifunctor.hpp
*
* Bifunctor protocol: map over both type parameters at once (C++).
*   A bifunctor is a two-parameter type constructor F<A, B> that is a functor in
* each parameter independently: bimap(fab, f, g) applies f : A -> C to the first
* and g : B -> D to the second, yielding F<C, D>. Where Functor unified the
* per-type map of a one-parameter context, Bifunctor unifies the per-type "map
* both sides" of a two-parameter one -- most visibly result, which already has
* map (success side) and map_err (error side): bimap is exactly the two run
* together, and the one-sided map_first / map_second recover each.
*
*   Because C++ has no native type classes, a bifunctor is recognized by
* specializing bifunctor_traits<F> with a single bimap. map_first (map only the
* first parameter) and map_second (map only the second) are then derived once,
* generically, as bimap with identity on the other side.
*
*   The standard instances ship here, each over a two-parameter type: std::pair
* and kv_pair (which have no functor-aware header of their own). result -- whose
* bimap unifies its own map / map_err -- carries its instance in result.hpp.
*
* USAGE:
*   using namespace djinterp;
*   // map both sides of a result: double the value, tag the error
*   result<int, const char*> r = ok<int, const char*>(21);
*   auto r2 = bimap(r,
*                   [](int x){ return x * 2; },
*                   [](const char* e){ return std::string("err: ") + e; });
*                                                 // ok(42)  (result<int,std::string>)
*
*   // map just one side of a pair:
*   std::pair<int, int> p(3, 4);
*   auto p2 = map_second(p, [](int y){ return y + 100; });   // (3, 104)
*
* 
* path:      /inc/djinterp/core/functional/bifunctor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
I.    BIFUNCTOR PROTOCOL
      1.  bifunctor_traits<F>                     (primary, undefined)
      2.  is_bifunctor<T>                         (detection trait)
II.   GENERIC BIFUNCTOR OPERATIONS
      1.  bimap                                   (the one obligation, delegated)
      2.  map_first                               (bimap with identity right)
      3.  map_second                              (bimap with identity left)
III.  INSTANCES
      1.  std::pair<A, B>
      2.  kv_pair<K, V>
*/


#ifndef DJINTERP_FUNCTIONAL_BIFUNCTOR_
#define DJINTERP_FUNCTIONAL_BIFUNCTOR_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/kv_pair.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    BIFUNCTOR PROTOCOL                                    ///
///////////////////////////////////////////////////////////////////////////////

// bifunctor_traits
//   trait: primary template, undefined by default. Each concrete bifunctor
// specializes bifunctor_traits<F> to expose:
//
//     - first_type      : the first parameter A of F<A, B>
//     - second_type     : the second parameter B of F<A, B>
//     - bimap(fab, f, g) : static -- F<C, D> from F<A, B> and f : A -> C,
//                         g : B -> D
//     - is_specialized   = true_type (marker)
//
//   bimap is the whole obligation; map_first and map_second are derived. The
// second template parameter is a SFINAE hook used by the family instances
// that key on a structural trait. The primary is left undefined so a use on a
// non-bifunctor produces a clean resolution error.
template<typename _Bifunctor,
         typename _Enable = void>
struct bifunctor_traits;


NS_INTERNAL

    // is_bifunctor_helper
    //   helper: SFINAE detector for whether bifunctor_traits<T> is
    // specialized. Looks for the is_specialized marker that every
    // specialization provides.
    template<typename _Type>
    struct is_bifunctor_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename bifunctor_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_bifunctor
//   trait: true if _Type has a specialization of bifunctor_traits (after
// cv-ref stripping). Used to SFINAE-constrain generic operations.
template<typename _Type>
struct is_bifunctor
    : internal::is_bifunctor_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_bifunctor_v
//   value: convenience alias for is_bifunctor<_Type>::value.
template<typename _Type>
static constexpr bool is_bifunctor_v = is_bifunctor<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   The two parameter types of a bifunctor, plus the identity helper that
// derives the one-sided maps, and the C++20 concept. Internal helpers carry a
// unique bifunctor_ prefix to keep the umbrella build collision-free.

NS_INTERNAL

    // bifunctor_first_type_helper / bifunctor_second_type_helper
    //   helpers: SFINAE extractors for the two parameter types.
    template<typename _AlwaysVoid,
             typename _Bifunctor>
    struct bifunctor_first_type_helper
    {};

    template<typename _Bifunctor>
    struct bifunctor_first_type_helper<
        void_t<typename bifunctor_traits<_Bifunctor>::first_type>,
        _Bifunctor>
    {
        using type = typename bifunctor_traits<_Bifunctor>::first_type;
    };

    template<typename _AlwaysVoid,
             typename _Bifunctor>
    struct bifunctor_second_type_helper
    {};

    template<typename _Bifunctor>
    struct bifunctor_second_type_helper<
        void_t<typename bifunctor_traits<_Bifunctor>::second_type>,
        _Bifunctor>
    {
        using type = typename bifunctor_traits<_Bifunctor>::second_type;
    };

    // bifunctor_identity_helper
    //   helper: identity, used to derive map_first / map_second from bimap by
    // mapping the untouched side through it. A named functor (not a lambda) so
    // it can appear in trailing return types on every floor.
    struct bifunctor_identity_helper
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

NS_END  // internal


// bifunctor_first_type / bifunctor_second_type
//   traits: the first and second parameter types of a bifunctor F.
// SFINAE-friendly.
template<typename _Bifunctor>
struct bifunctor_first_type
{
    using type = typename internal::bifunctor_first_type_helper<
        void, typename std::decay<_Bifunctor>::type>::type;
};

template<typename _Bifunctor>
struct bifunctor_second_type
{
    using type = typename internal::bifunctor_second_type_helper<
        void, typename std::decay<_Bifunctor>::type>::type;
};

// bifunctor_first_type_t / bifunctor_second_type_t
//   types: convenience aliases.
template<typename _Bifunctor>
using bifunctor_first_type_t = typename bifunctor_first_type<_Bifunctor>::type;

template<typename _Bifunctor>
using bifunctor_second_type_t = typename bifunctor_second_type<_Bifunctor>::type;


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Bifunctor
    //   concept: satisfied when _Type is a specialized bifunctor. The
    // PascalCase typeclass face, alongside Functor / Applicative / Foldable.
    template<typename _Type>
    concept Bifunctor = is_bifunctor<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC BIFUNCTOR OPERATIONS                          ///
///////////////////////////////////////////////////////////////////////////////
//   bimap delegates to bifunctor_traits<F>::bimap; map_first / map_second are
// bimap with identity on the other side. All are D_CONSTEXPR and fold at
// compile time wherever the instance's bimap does.

// bimap
//   function: maps both parameters at once -- f : A -> C over the first, g :
//   B -> D over the second -- yielding F<C, D>. The result type is whatever the
// instance produces, so it is deduced.
template<typename _Bifunctor,
         typename _First,
         typename _Second>
D_NODISCARD
D_CONSTEXPR
auto bimap
(
    _Bifunctor&& _fab,
    _First       _f,
    _Second      _g
)
-> decltype(bifunctor_traits<typename std::decay<_Bifunctor>::type>::bimap(
       std::forward<_Bifunctor>(_fab),
       _f,
       _g))
{
    return bifunctor_traits<typename std::decay<_Bifunctor>::type>::bimap(
        std::forward<_Bifunctor>(_fab),
        _f,
        _g);
}


// map_first
//   function: maps only the first parameter (f : A -> C), leaving the second
// untouched -- bimap with identity on the right. For result this is its map.
template<typename _Bifunctor,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto map_first
(
    _Bifunctor&& _fab,
    _Function    _f
)
-> decltype(::djinterp::bimap(
       std::forward<_Bifunctor>(_fab),
       _f,
       internal::bifunctor_identity_helper()))
{
    return ::djinterp::bimap(
        std::forward<_Bifunctor>(_fab),
        _f,
        internal::bifunctor_identity_helper());
}


// map_second
//   function: maps only the second parameter (g : B -> D), leaving the first
// untouched -- bimap with identity on the left. For result this is its map_err.
template<typename _Bifunctor,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto map_second
(
    _Bifunctor&& _fab,
    _Function    _g
)
-> decltype(::djinterp::bimap(
       std::forward<_Bifunctor>(_fab),
       internal::bifunctor_identity_helper(),
       _g))
{
    return ::djinterp::bimap(
        std::forward<_Bifunctor>(_fab),
        internal::bifunctor_identity_helper(),
        _g);
}


///////////////////////////////////////////////////////////////////////////////
///             III.  INSTANCES                                             ///
///////////////////////////////////////////////////////////////////////////////
//   std::pair and kv_pair, the two standard two-parameter product types with
// no functor-aware header of their own. result's instance lives in result.hpp.
// Written in the explicit two-argument `<T, void>` form against the
// SFINAE-hooked primary.

// bifunctor_traits<std::pair<_A, _B>>
//   instance: maps each component of the pair.
template<typename _A,
         typename _B>
struct bifunctor_traits<std::pair<_A, _B>, void>
{
    using is_specialized = std::true_type;
    using first_type     = _A;
    using second_type    = _B;

    template<typename _First,
             typename _Second>
    static
    D_CONSTEXPR
    std::pair<
        typename std::decay<decltype(std::declval<_First&>()(
            std::declval<const _A&>()))>::type,
        typename std::decay<decltype(std::declval<_Second&>()(
            std::declval<const _B&>()))>::type>
    bimap(
        const std::pair<_A, _B>& _p,
        _First                   _f,
        _Second                  _g
    )
    {
        return std::make_pair(_f(_p.first), _g(_p.second));
    }
};


// bifunctor_traits<kv_pair<_Key, _Value>>
//   instance: maps the key and the value. Note kv_pair's equality / ordering
// compare keys only, so mapping the value preserves identity while mapping the
// key may change it -- expected, and the caller's concern.
template<typename _Key,
         typename _Value>
struct bifunctor_traits<kv_pair<_Key, _Value>, void>
{
    using is_specialized = std::true_type;
    using first_type     = _Key;
    using second_type    = _Value;

    template<typename _First,
             typename _Second>
    static
    D_CONSTEXPR
    kv_pair<
        typename std::decay<decltype(std::declval<_First&>()(
            std::declval<const _Key&>()))>::type,
        typename std::decay<decltype(std::declval<_Second&>()(
            std::declval<const _Value&>()))>::type>
    bimap(
        const kv_pair<_Key, _Value>& _kv,
        _First                       _f,
        _Second                      _g
    )
    {
        using mapped_key_t = typename std::decay<decltype(
            std::declval<_First&>()(std::declval<const _Key&>()))>::type;
        using mapped_value_t = typename std::decay<decltype(
            std::declval<_Second&>()(std::declval<const _Value&>()))>::type;

        return kv_pair<mapped_key_t, mapped_value_t>(
            _f(_kv.m_key), _g(_kv.m_value));
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_BIFUNCTOR_
