/******************************************************************************
* djinterp [functional]                                        profunctor.hpp
*
* Profunctor protocol: dimap over a two-parameter arrow (C++).
*   A profunctor is a two-parameter type P<A, B> that is *contravariant* in its
* first parameter and *covariant* in its second -- the shape of a computation
* "from A to B". The one operation, dimap(p, pre, post), pre-composes pre : A'
* -> A onto the input and post-composes post : B -> B' onto the output, turning
* a P<A, B> into a P<A', B'>. For the canonical instance -- a function arrow --
* dimap(f, pre, post) is exactly post . f . pre. Where Bifunctor mapped both
* parameters covariantly, Profunctor flips the first: you adapt the input by
* mapping *into* the original domain, and adapt the output by mapping *out* of
* the original codomain.
*
*   Because C++ has no native type classes -- and because a bare lambda does
* not expose its domain and codomain as type parameters -- this header ships a
* light arrow wrapper, profn<F>, holding a callable and itself callable. It is
* the canonical Profunctor: dimap composes a new arrow without unwrapping. A
* profunctor is recognized by specializing profunctor_traits<P> with dimap;
* lmap (adapt input only) and rmap (adapt output only) are derived once.
*
* USAGE:
*   using namespace djinterp;
*   auto length = make_profn([](const std::string& s){ return (int)s.size(); });
*   // adapt input (parse) and output (format) around the core arrow:
*   auto adapted = dimap(length,
*                        [](int n){ return std::string(n, 'x'); },   // pre:  int -> string
*                        [](int n){ return n * 10; });               // post: int -> int
*   int r = adapted(3);                       // length("xxx")*10 = 30
*
*   auto only_in  = lmap(length, [](int n){ return std::to_string(n); }); // pre only
*   auto only_out = rmap(length, [](int n){ return n + 1; });             // post only
*
* 
* path:      /inc/djinterp/core/functional/profunctor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    THE ARROW WRAPPER                            (profn<F> + make_profn)
II.   PROFUNCTOR PROTOCOL
      1.  profunctor_traits<P>                     (primary, undefined)
      2.  is_profunctor<T>                         (detection trait)
III.  GENERIC PROFUNCTOR OPERATIONS
      1.  dimap                                    (adapt input and output)
      2.  lmap                                     (adapt input only)
      3.  rmap                                     (adapt output only)
IV.   INSTANCE                                      (profn<F>)
*/


#ifndef DJINTERP_FUNCTIONAL_PROFUNCTOR_
#define DJINTERP_FUNCTIONAL_PROFUNCTOR_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    THE ARROW WRAPPER  (profn<F>)                         ///
///////////////////////////////////////////////////////////////////////////////

// profn
//   struct: a profunctor-wrapped callable -- the canonical Profunctor. Holds a
// callable F and forwards operator() to it, so a profn is used exactly like the
// function it wraps; dimap / lmap / rmap build a new profn around a composed
// callable without unwrapping.
template<typename _Fn>
struct profn
{
    _Fn fn;

    D_CONSTEXPR
    explicit profn(
        _Fn _f
    )
        : fn(_f)
    {}

    template<typename _Arg>
    D_CONSTEXPR
    auto operator()(
        _Arg&& _arg
    ) const
    -> decltype(fn(std::forward<_Arg>(_arg)))
    {
        return fn(std::forward<_Arg>(_arg));
    }
};


// make_profn
//   function: wraps a callable into a profn (deducing and decaying F).
template<typename _Fn>
D_NODISCARD
D_CONSTEXPR
profn<typename std::decay<_Fn>::type>
make_profn
(
    _Fn&& _f
)
{
    return profn<typename std::decay<_Fn>::type>(std::forward<_Fn>(_f));
}


///////////////////////////////////////////////////////////////////////////////
///             II.   PROFUNCTOR PROTOCOL                                   ///
///////////////////////////////////////////////////////////////////////////////

// profunctor_traits
//   trait: primary template, undefined by default. Each concrete profunctor
// specializes profunctor_traits<P> to expose:
//
//     - dimap(p, pre, post) : static -- P<A', B'> from P<A, B>,
//                            pre : A' -> A (contravariant), post : B -> B'
//                            (covariant)
//     - is_specialized       = true_type (marker)
//
//   dimap is the whole obligation; lmap and rmap are derived. The second
// template parameter is a SFINAE hook. The primary is left undefined so a use
// on a non-profunctor produces a clean resolution error.
template<typename _Profunctor,
         typename _Enable = void>
struct profunctor_traits;


NS_INTERNAL

    // is_profunctor_helper
    //   helper: SFINAE detector for whether profunctor_traits<T> is
    // specialized.
    template<typename _Type>
    struct is_profunctor_helper
    {
    private:
        template<typename _T>
        static auto test(int)
            -> decltype(
                typename profunctor_traits<_T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Type>(0));
    };

    // profunctor_identity_helper
    //   helper: identity, used to derive lmap / rmap from dimap (identity on
    // the untouched side). A named functor so it can appear in trailing return
    // types on every floor.
    struct profunctor_identity_helper
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

    // profunctor_dimap_helper
    //   helper: the composed callable behind a function arrow's dimap --
    //   post . fn . pre. Stored inside a fresh profn. A named functor (not a
    // lambda) so the composition works on every floor, including C++11.
    template<typename _Pre,
             typename _Fn,
             typename _Post>
    struct profunctor_dimap_helper
    {
        _Pre  pre;
        _Fn   fn;
        _Post post;

        template<typename _Arg>
        D_CONSTEXPR
        auto operator()(
            _Arg&& _arg
        ) const
        -> decltype(post(fn(pre(std::forward<_Arg>(_arg)))))
        {
            return post(fn(pre(std::forward<_Arg>(_arg))));
        }
    };

NS_END  // internal


// is_profunctor
//   trait: true if _Type has a specialization of profunctor_traits (after
// cv-ref stripping).
template<typename _Type>
struct is_profunctor
    : internal::is_profunctor_helper<typename std::decay<_Type>::type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_profunctor_v
//   value: convenience alias for is_profunctor<_Type>::value.
template<typename _Type>
static constexpr bool is_profunctor_v = is_profunctor<_Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Profunctor
    //   concept: satisfied when _Type is a specialized profunctor.
    template<typename _Type>
    concept Profunctor = is_profunctor<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             III.  GENERIC PROFUNCTOR OPERATIONS                         ///
///////////////////////////////////////////////////////////////////////////////
//   dimap delegates to profunctor_traits<P>::dimap; lmap / rmap are dimap with
// identity on the other side. All are D_CONSTEXPR.

// dimap
//   function: adapts both ends -- pre : A' -> A onto the input, post : B -> B'
// onto the output -- turning a P<A, B> into a P<A', B'>.
template<typename _Profunctor,
         typename _Pre,
         typename _Post>
D_NODISCARD
D_CONSTEXPR
auto dimap
(
    _Profunctor&& _p,
    _Pre          _pre,
    _Post         _post
)
-> decltype(profunctor_traits<typename std::decay<_Profunctor>::type>::dimap(
       std::forward<_Profunctor>(_p),
       _pre,
       _post))
{
    return profunctor_traits<typename std::decay<_Profunctor>::type>::dimap(
        std::forward<_Profunctor>(_p),
        _pre,
        _post);
}


// lmap
//   function: adapts only the input -- pre : A' -> A -- leaving the output.
// dimap with identity on the right (the contravariant map).
template<typename _Profunctor,
         typename _Pre>
D_NODISCARD
D_CONSTEXPR
auto lmap
(
    _Profunctor&& _p,
    _Pre          _pre
)
-> decltype(::djinterp::dimap(
       std::forward<_Profunctor>(_p),
       _pre,
       internal::profunctor_identity_helper()))
{
    return ::djinterp::dimap(
        std::forward<_Profunctor>(_p),
        _pre,
        internal::profunctor_identity_helper());
}


// rmap
//   function: adapts only the output -- post : B -> B' -- leaving the input.
// dimap with identity on the left (the covariant map; the profunctor's fmap).
template<typename _Profunctor,
         typename _Post>
D_NODISCARD
D_CONSTEXPR
auto rmap
(
    _Profunctor&& _p,
    _Post         _post
)
-> decltype(::djinterp::dimap(
       std::forward<_Profunctor>(_p),
       internal::profunctor_identity_helper(),
       _post))
{
    return ::djinterp::dimap(
        std::forward<_Profunctor>(_p),
        internal::profunctor_identity_helper(),
        _post);
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   INSTANCE  (profn<F>)                                  ///
///////////////////////////////////////////////////////////////////////////////

// profunctor_traits<profn<_Fn>>
//   instance: a function arrow is the canonical profunctor. dimap returns a
// fresh profn wrapping post . fn . pre. Written in the explicit two-argument
// `<T, void>` form against the SFINAE-hooked primary.
template<typename _Fn>
struct profunctor_traits<profn<_Fn>, void>
{
    using is_specialized = std::true_type;

    template<typename _Pre,
             typename _Post>
    static
    D_CONSTEXPR
    profn<internal::profunctor_dimap_helper<_Pre, _Fn, _Post> >
    dimap(
        const profn<_Fn>& _p,
        _Pre              _pre,
        _Post             _post
    )
    {
        return ::djinterp::make_profn(
            internal::profunctor_dimap_helper<_Pre, _Fn, _Post>{
                _pre, _p.fn, _post});
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PROFUNCTOR_
