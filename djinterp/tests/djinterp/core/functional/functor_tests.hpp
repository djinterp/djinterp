/******************************************************************************
* djinterp [test]                                           functor_tests.hpp
*
*   Unit-test declarations for core/functional/functor.hpp -- the Functor
* protocol and the generic functorial map.  One declaration group per section of
* the module under test:
*
*     functor_tests_protocol.cpp    -- I.  functor_traits, is_functor (+ _v)
*     functor_tests_bridge.cpp      -- I.2 the blanket monad bridge
*     functor_tests_structural.cpp  -- 0.  functor_value_type, is_fmappable,
*                                           and the C++20 concept faces
*     functor_tests_map.cpp         -- II. functor_map and the functor laws
*
*   FIXTURES.  The header makes two distinct claims about how a type becomes a
* functor, and the suite supplies one instance of each so the two can be played
* against one another.
*
*     tmaybe<T> / tbox<T>   are MONADS: each specializes monad_traits and NOTHING
*                           else -- no functor_traits of its own.  If the blanket
*                           bridge works, they are functors anyway, and their map
*                           is monad_map.  tbox is a second, unrelated monad,
*                           present to show the bridge costs a NEW monad zero
*                           wiring ("any future monad").
*
*     lazy_view<Src, Fn>    is NOT a monad.  It carries an explicit functor_traits
*                           specialization, and it is the "context whose mapped
*                           type depends on the mapping function" the header
*                           describes: mapping it composes into a lazy_view over a
*                           DIFFERENT function type, so there is no single F<U> to
*                           name -- and accordingly it supplies NO rebind.  That
*                           the bridge (keyed on is_monad) does not overlap it is
*                           checked directly.
*
*   The has_rebind probe turns the header's design note into a test: the bridged
* functors expose rebind, lazy_view does not, and yet BOTH are functors and BOTH
* map -- which is exactly what "rebind is intentionally not part of the core
* protocol" means.  is_complete pins the other half of the protocol's contract:
* the primary functor_traits is left UNDEFINED, so it is an incomplete type for a
* non-functor rather than a silently-wrong answer.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/functional/functor_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (two monads, a non-monad view functor, negatives, probes)
I.    FUNCTOR PROTOCOL        (functor_traits, is_functor)
I.2   THE MONAD BRIDGE        (every monad is a functor)
0'.   STRUCTURAL TRAITS       (functor_value_type, is_fmappable, concepts)
II.   GENERIC OPERATIONS      (functor_map)
*/


#ifndef DJINTERP_TEST_FUNCTOR_TESTS_
#define DJINTERP_TEST_FUNCTOR_TESTS_ 1

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp (module under test)
#include "../../core/functional/functor.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// -- mapping functions ----------------------------------------------------

struct dbl
{
    constexpr int operator()(int _x) const { return _x * 2; }
};

struct add1
{
    constexpr int operator()(int _x) const { return _x + 1; }
};

// dbl_then_add1: the composite (add1 . dbl), written directly, for the
// composition law.
struct dbl_then_add1
{
    constexpr int operator()(int _x) const { return (_x * 2) + 1; }
};

struct ident_fn
{
    constexpr int operator()(int _x) const { return _x; }
};

struct is_even
{
    constexpr bool operator()(int _x) const { return (_x % 2) == 0; }
};

struct to_str
{
    std::string operator()(int _x) const { return std::string(_x, 'x'); }
};

// wants_string: a function that CANNOT be applied to an int functor -- the
// finer-grained question is_fmappable answers.
struct wants_string
{
    std::string operator()(const std::string& _s) const { return _s; }
};


// -- MONAD fixture #1: tmaybe (specializes monad_traits ONLY) -------------

// tmaybe
//   fixture monad: a maybe-shaped context. It receives a monad_traits
// specialization and NO functor_traits of its own -- the blanket bridge is what
// makes it a functor.
template<typename _Type>
struct tmaybe
{
    using value_type = _Type;

    _Type value;
    bool  has;
};

template<typename _Type>
constexpr tmaybe<_Type>
just(_Type _x)
{
    return tmaybe<_Type>{ _x, true };
}

template<typename _Type>
constexpr tmaybe<_Type>
nothing()
{
    return tmaybe<_Type>{ _Type{}, false };
}


// -- MONAD fixture #2: tbox (a SECOND, unrelated monad) -------------------

// tbox
//   fixture monad: the identity context -- always holds a value. Present to show
// that a NEW monad becomes a functor with zero per-type functor wiring.
template<typename _Type>
struct tbox
{
    using value_type = _Type;

    _Type value;
};

template<typename _Type>
constexpr tbox<_Type>
boxed(_Type _x)
{
    return tbox<_Type>{ _x };
}


// -- NON-MONAD functor: a lazy view --------------------------------------

// composed: the composite g . f behind the view's map.
template<typename _Fn,
         typename _Gn>
struct composed
{
    _Fn f;
    _Gn g;

    template<typename _Arg>
    constexpr auto operator()(
        _Arg _a
    ) const
    -> decltype(g(f(_a)))
    {
        return g(f(_a));
    }
};

// lazy_view
//   fixture functor: a source and a pending function. Mapping it composes into a
// lazy_view over a DIFFERENT function type, so its mapped type follows from the
// call -- there is no single F<U> to name, and so it supplies NO rebind. It is
// NOT a monad, so the blanket bridge cannot claim it.
template<typename _Src,
         typename _Fn>
struct lazy_view
{
    _Src src;
    _Fn  fn;

    constexpr auto get() const -> decltype(fn(src)) { return fn(src); }
};

template<typename _Src,
         typename _Fn>
constexpr lazy_view<_Src, _Fn>
make_view(
    _Src _s,
    _Fn  _f
)
{
    return lazy_view<_Src, _Fn>{ _s, _f };
}


// -- negatives ------------------------------------------------------------

// not_a_functor: no functor_traits specialization at all.
struct not_a_functor
{
};

// no_marker: a functor_traits specialization WITH a map but WITHOUT the
// is_specialized marker -- must read as not-a-functor.
struct no_marker
{
};


// -- probes ---------------------------------------------------------------

// is_complete
//   probe: true when _Type is a COMPLETE type. The primary functor_traits is
// declared but left UNDEFINED, so this reads that contract SFINAE-safely.
template<typename _Type,
         typename = void>
struct is_complete : std::false_type
{
};

template<typename _Type>
struct is_complete<_Type, decltype(void(sizeof(_Type)))> : std::true_type
{
};

// has_type
//   probe: true when _Type exposes a nested ::type. Applied to the INTERNAL
// value-type helper, whose primary soft-fails by having none.
template<typename _Type,
         typename = void>
struct has_type : std::false_type
{
};

template<typename _Type>
struct has_type<_Type, void_t<typename _Type::type> > : std::true_type
{
};

// has_rebind
//   probe: true when a functor_traits specialization supplies rebind<U>. The
// monad bridge does; a view does NOT -- and both are functors, which is what
// "rebind is not part of the core protocol" means.
template<typename _Type,
         typename = void>
struct has_rebind : std::false_type
{
};

template<typename _Type>
struct has_rebind<_Type,
                  void_t<typename _Type::template rebind<int> > >
    : std::true_type
{
};

// vt_helper: shorthand for the internal value-type helper under test.
template<typename _Functor>
using vt_helper = internal::functor_value_type_helper<void, _Functor>;

// bump
//   the header's own USAGE example: generic over ANY functor, named once.
template<typename _Functor,
         typename _Function>
constexpr auto bump(
    _Functor&& _fa,
    _Function  _fn
)
-> decltype(::djinterp::functor_map(std::forward<_Functor>(_fa), _fn))
{
    return ::djinterp::functor_map(std::forward<_Functor>(_fa), _fn);
}


///////////////////////////////////////////////////////////////////////////////
///             CONCEPT-FACING HELPERS  (C++20)                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// which_functor / which_fmappable: constrained overload + unconstrained
// fallback, proving the concepts GATE resolution rather than merely evaluating
// to a bool.
template<typename _Type>
    requires Functor<_Type>
constexpr int which_functor(_Type) { return 1; }

template<typename _Type>
constexpr int which_functor(_Type) { return 0; }

template<typename _Function,
         typename _Type>
    requires fmappable_with<_Type, _Function>
constexpr int which_fmappable(_Type, _Function) { return 1; }

template<typename _Function,
         typename _Type>
constexpr int which_fmappable(_Type, _Function) { return 0; }

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             I.    FUNCTOR PROTOCOL                                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_functor_positive();
bool tests_is_functor_negative();
bool tests_is_functor_decay();
bool tests_is_functor_requires_marker();
bool tests_functor_traits_primary_is_undefined();
bool tests_functor_traits_surface();
bool tests_is_functor_v_agrees();
bool tests_rebind_is_not_core_protocol();


///////////////////////////////////////////////////////////////////////////////
///             I.2   THE MONAD BRIDGE                                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_bridge_every_monad_is_a_functor();
bool tests_bridge_is_keyed_on_is_monad();
bool tests_bridge_derives_map_from_monad_map();
bool tests_bridge_value_type_from_monad();
bool tests_bridge_rebind_from_monad();
bool tests_bridge_zero_wiring_for_a_new_monad();
bool tests_bridge_does_not_overlap_explicit();
bool tests_bridge_preserves_monad_context();


///////////////////////////////////////////////////////////////////////////////
///             0'.   STRUCTURAL TRAITS & CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

bool tests_functor_value_type();
bool tests_functor_value_type_t_alias();
bool tests_functor_value_type_helper_sfinae();
bool tests_is_fmappable_positive();
bool tests_is_fmappable_negative();
bool tests_is_fmappable_does_not_require_the_marker();
bool tests_is_fmappable_v_agrees();
bool tests_functor_concept();
bool tests_fmappable_with_concept();
bool tests_concepts_gating();


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC OPERATIONS (functor_map)                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_functor_map_over_a_monad();
bool tests_functor_map_preserves_context();
bool tests_functor_map_over_a_view();
bool tests_functor_map_changes_inner_type();
bool tests_functor_map_law_identity();
bool tests_functor_map_law_composition();
bool tests_functor_map_result_type();
bool tests_functor_map_forwarding();
bool tests_functor_map_constexpr();
bool tests_functor_map_generic_over_any_functor();


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE INSTANCES  (djinterp scope)                         ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<tmaybe<T>>  -- the ONLY wiring tmaybe receives. No functor_traits
// specialization exists for it anywhere: the blanket bridge supplies that.
template<typename _Type>
struct monad_traits<testing::tmaybe<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = testing::tmaybe<_To>;

    static D_CONSTEXPR testing::tmaybe<_Type>
    unit(
        _Type _x
    )
    {
        return testing::just(_x);
    }

    template<typename _Function>
    static D_CONSTEXPR auto
    bind(
        const testing::tmaybe<_Type>& _m,
        _Function                     _f
    )
    -> decltype(_f(_m.value))
    {
        using result_t = decltype(_f(_m.value));
        using inner_t  = typename result_t::value_type;
        return _m.has ? _f(_m.value) : result_t{ inner_t{}, false };
    }
};

// monad_traits<tbox<T>>  -- a SECOND monad, wired identically and no further.
template<typename _Type>
struct monad_traits<testing::tbox<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = testing::tbox<_To>;

    static D_CONSTEXPR testing::tbox<_Type>
    unit(
        _Type _x
    )
    {
        return testing::boxed(_x);
    }

    template<typename _Function>
    static D_CONSTEXPR auto
    bind(
        const testing::tbox<_Type>& _m,
        _Function                   _f
    )
    -> decltype(_f(_m.value))
    {
        return _f(_m.value);
    }
};

// functor_traits<lazy_view<Src, Fn>>  -- an EXPLICIT specialization for a type
// that is NOT a monad. Its mapped type depends on the mapping function, so it
// names no rebind; map's result follows from the call alone.
template<typename _Src,
         typename _Fn>
struct functor_traits<testing::lazy_view<_Src, _Fn>, void>
{
    using is_specialized = std::true_type;
    using value_type     = decltype(std::declval<const _Fn&>()(
                               std::declval<const _Src&>()));

    // NOTE: deliberately NO rebind -- there is no single F<U> to name.

    // map is SFINAE-constrained on _Gn being applicable to what this view
    // currently yields, so an unusable function makes the pair unmappable --
    // as it would for any real view.
    template<typename _Gn,
             typename = decltype(std::declval<_Gn&>()(
                 std::declval<const _Fn&>()(std::declval<const _Src&>())))>
    static D_CONSTEXPR
    testing::lazy_view<_Src, testing::composed<_Fn, _Gn> >
    map(
        const testing::lazy_view<_Src, _Fn>& _v,
        _Gn                                  _g
    )
    {
        return testing::lazy_view<_Src, testing::composed<_Fn, _Gn> >{
            _v.src, testing::composed<_Fn, _Gn>{ _v.fn, _g } };
    }
};

// functor_traits<no_marker>  -- a map, but NO is_specialized marker, so
// detection must refuse it.
template<>
struct functor_traits<testing::no_marker, void>
{
    using value_type = int;

    template<typename _Function>
    static int map(const testing::no_marker&, _Function) { return 0; }
};


NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTOR_TESTS_
