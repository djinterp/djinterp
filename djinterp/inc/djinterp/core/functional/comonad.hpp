/******************************************************************************
* djinterp [functional]                                           comonad.hpp
*
* Comonad protocol: extract, extend, duplicate -- the dual of Monad (C++).
*   Where a monad lets you put a value into a context (unit) and sequence
* context-producing steps (bind), a comonad lets you take a value *out* of a
* context (extract) and extend a context-consuming step across the whole
* structure (extend). A comonad is a context W<A> from which a focus value can
* always be read -- so, unlike maybe or result, it is never empty. The three
* operations mirror the monad's three with the arrows reversed:
*
*     monad:    unit   : A -> M<A>        bind   : M<A> -> (A -> M<B>) -> M<B>
*     comonad:  extract: W<A> -> A        extend : W<A> -> (W<A> -> B) -> W<B>
*               (and duplicate : W<A> -> W<W<A>>, the dual of join)
*
*   Because C++ has no native type classes, a comonad is recognized by
* specializing comonad_traits<W> with extract and extend. duplicate is derived
* once, generically, as extend with the identity function.
*
*   The instances here are the Env (co-reader) comonad over the two standard
* pair types: std::pair<E, A> and kv_pair<K, V>. The focus is the second
* component (the value); the first (the environment / key) rides along
* untouched. extract reads the focus; extend recomputes the focus from the
* whole pair, keeping the environment. These reuse existing types -- no empty
* case to worry about, since a pair always has a focus.
*
* USAGE:
*   using namespace djinterp;
*   std::pair<std::string, int> w("ctx", 10);
*   int a = extract(w);                              // 10
*   // extend: recompute the focus from the whole context
*   auto w2 = extend(w, [](const std::pair<std::string,int>& c){
*                          return c.second * 2;       // sees env + focus
*                      });                            // ("ctx", 20)
*   auto ww = duplicate(w);                           // ("ctx", ("ctx", 10))
*
* 
* path:      /inc/djinterp/core/functional/comonad.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS
I.    COMONAD PROTOCOL
      1.  comonad_traits<W>                       (primary, undefined)
      2.  is_comonad<T>                           (detection trait)
II.   GENERIC COMONAD OPERATIONS
      1.  extract                                 (W<A> -> A)
      2.  extend                                  (cobind: W<A> -> (W<A>->B) -> W<B>)
      3.  duplicate                               (extend with identity)
III.  INSTANCES                                    (the Env comonad)
      1.  std::pair<E, A>
      2.  kv_pair<K, V>
*/


#ifndef DJINTERP_FUNCTIONAL_COMONAD_
#define DJINTERP_FUNCTIONAL_COMONAD_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/kv_pair.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    COMONAD PROTOCOL                                      ///
///////////////////////////////////////////////////////////////////////////////

// comonad_traits
//   trait: primary template, undefined by default. Each concrete comonad
// specializes comonad_traits<W> to expose:
//
//     - value_type      : the focus type A of W<A>
//     - extract(w)       : static -- read the focus, W<A> -> A
//     - extend(w, f)      : static -- W<B> from W<A> and f : W<A> -> B
//                         (the co-bind: f sees the whole context)
//     - is_specialized   = true_type (marker)
//
//   extract and extend are the obligations; duplicate is derived. The second
// template parameter is a SFINAE hook used by the family instances that key on
// a structural trait. The primary is left undefined so a use on a non-comonad
// produces a clean resolution error.
template<typename _Comonad,
         typename _Enable = void>
struct comonad_traits;


NS_INTERNAL

    // is_comonad_helper
    //   helper: SFINAE detector for whether comonad_traits<T> is specialized.
    // Looks for the is_specialized marker that every specialization provides.
    template<typename _Type>
    struct is_comonad_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename comonad_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

NS_END  // internal


// is_comonad
//   trait: true if _Type has a specialization of comonad_traits (after cv-ref
// stripping). Used to SFINAE-constrain generic operations.
template<typename _Type>
struct is_comonad
    : internal::is_comonad_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_comonad_v
//   value: convenience alias for is_comonad<_Type>::value.
template<typename _Type>
static constexpr bool is_comonad_v = is_comonad<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   The focus type of a comonad, the identity helper that derives duplicate,
// and the C++20 concept. Internal helpers carry a unique comonad_ prefix to
// keep the umbrella build collision-free.

NS_INTERNAL

    // comonad_value_type_helper
    //   helper: SFINAE extractor for comonad_traits<W>::value_type.
    template<typename _AlwaysVoid,
             typename _Comonad>
    struct comonad_value_type_helper
    {};

    template<typename _Comonad>
    struct comonad_value_type_helper<
        void_t<typename comonad_traits<_Comonad>::value_type>,
        _Comonad>
    {
        using type = typename comonad_traits<_Comonad>::value_type;
    };

    // comonad_identity_helper
    //   helper: identity, used to derive duplicate from extend (extend with
    // identity duplicates the context). A named functor (not a lambda) so it
    // can appear in trailing return types on every floor. Takes the comonad by
    // value and returns it, so extend sees f : W<A> -> W<A>.
    struct comonad_identity_helper
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


// comonad_value_type
//   trait: the focus type A of a comonad W. SFINAE-friendly.
template<typename _Comonad>
struct comonad_value_type
{
    using type = typename internal::comonad_value_type_helper<
        void, typename std::decay<_Comonad>::type>::type;
};

// comonad_value_type_t
//   type: convenience alias for comonad_value_type<W>::type.
template<typename _Comonad>
using comonad_value_type_t = typename comonad_value_type<_Comonad>::type;


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Comonad
    //   concept: satisfied when _Type is a specialized comonad. The PascalCase
    // typeclass face, alongside Functor / Monad / Foldable.
    template<typename _Type>
    concept Comonad = is_comonad<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC COMONAD OPERATIONS                            ///
///////////////////////////////////////////////////////////////////////////////
//   extract delegates to comonad_traits<W>::extract; extend to ::extend;
// duplicate is extend with identity. All are D_CONSTEXPR and fold at compile
// time wherever the instance's operations do.

// extract
//   function: reads the focus value out of a comonad, W<A> -> A. The dual of
// the monad's unit.
template<typename _Comonad>
D_NODISCARD
D_CONSTEXPR
auto extract
(
    _Comonad&& _w
)
-> decltype(comonad_traits<typename std::decay<_Comonad>::type>::extract(
       std::forward<_Comonad>(_w)))
{
    return comonad_traits<typename std::decay<_Comonad>::type>::extract(
        std::forward<_Comonad>(_w));
}


// extend
//   function: the co-bind. Applies f : W<A> -> B to the whole comonad and to
// every sub-context, producing W<B>. The dual of the monad's bind.
template<typename _Comonad,
         typename _Function>
D_NODISCARD
D_CONSTEXPR
auto extend
(
    _Comonad&& _w,
    _Function  _function
)
-> decltype(comonad_traits<typename std::decay<_Comonad>::type>::extend(
       std::forward<_Comonad>(_w),
       _function))
{
    return comonad_traits<typename std::decay<_Comonad>::type>::extend(
        std::forward<_Comonad>(_w),
        _function);
}


// duplicate
//   function: nests a comonad one level, W<A> -> W<W<A>> -- extend with the
// identity. The dual of the monad's join.
template<typename _Comonad>
D_NODISCARD
D_CONSTEXPR
auto duplicate
(
    _Comonad&& _w
)
-> decltype(::djinterp::extend(
       std::forward<_Comonad>(_w),
       internal::comonad_identity_helper()))
{
    return ::djinterp::extend(
        std::forward<_Comonad>(_w),
        internal::comonad_identity_helper());
}


///////////////////////////////////////////////////////////////////////////////
///             III.  INSTANCES  (the Env comonad)                          ///
///////////////////////////////////////////////////////////////////////////////
//   std::pair and kv_pair as the Env (co-reader) comonad: the focus is the
// second component, the first rides along as the environment. Written in the
// explicit two-argument `<T, void>` form against the SFINAE-hooked primary.

// comonad_traits<std::pair<_Env, _Focus>>
//   instance: focus is .second; .first is the environment.
template<typename _Env,
         typename _Focus>
struct comonad_traits<std::pair<_Env, _Focus>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _Focus;

    static
    D_CONSTEXPR
    _Focus extract(
        const std::pair<_Env, _Focus>& _w
    )
    {
        return _w.second;
    }

    template<typename _Function>
    static
    D_CONSTEXPR
    std::pair<_Env,
              typename std::decay<decltype(std::declval<_Function&>()(
                  std::declval<const std::pair<_Env, _Focus>&>()))>::type>
    extend(
        const std::pair<_Env, _Focus>& _w,
        _Function                       _function
    )
    {
        return std::make_pair(_w.first, _function(_w));
    }
};


// comonad_traits<kv_pair<_Key, _Value>>
//   instance: focus is the value; the key is the environment.
template<typename _Key,
         typename _Value>
struct comonad_traits<kv_pair<_Key, _Value>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _Value;

    static
    D_CONSTEXPR
    _Value extract(
        const kv_pair<_Key, _Value>& _w
    )
    {
        return _w.m_value;
    }

    template<typename _Function>
    static
    D_CONSTEXPR
    kv_pair<_Key,
            typename std::decay<decltype(std::declval<_Function&>()(
                std::declval<const kv_pair<_Key, _Value>&>()))>::type>
    extend(
        const kv_pair<_Key, _Value>& _w,
        _Function                    _function
    )
    {
        using mapped_t = typename std::decay<decltype(
            std::declval<_Function&>()(
                std::declval<const kv_pair<_Key, _Value>&>()))>::type;

        return kv_pair<_Key, mapped_t>(_w.m_key, _function(_w));
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_COMONAD_
