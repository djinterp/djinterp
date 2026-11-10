/*******************************************************************************
* djinterp [test]                                              functor_tests.hpp
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
*     DLazyView<Src, Fn>    is NOT a monad.  It carries an explicit functor_traits
*                           specialization, and it is the "context whose mapped
*                           type depends on the mapping function" the header
*                           describes: mapping it composes into a DLazyView over a
*                           DIFFERENT function type, so there is no single F<U> to
*                           name -- and accordingly it supplies NO rebind.  That
*                           the bridge (keyed on is_monad) does not overlap it is
*                           checked directly.
*
*   The has_rebind probe turns the header's design note into a test: the bridged
* functors expose rebind, DLazyView does not, and yet BOTH are functors and BOTH
* map -- which is exactly what "rebind is intentionally not part of the core
* protocol" means.  is_complete pins the other half of the protocol's contract:
* the primary functor_traits is left UNDEFINED, so it is an incomplete type for a
* non-functor rather than a silently-wrong answer.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/functor_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.12
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (two monads, a non-monad view functor, negatives, probes)
      -------------------------------------------------------------------

I.    FUNCTOR PROTOCOL        (functor_traits, is_functor)
      ----------------------------------------------------

      I.2   THE MONAD BRIDGE        (every monad is a functor)

      0'.   STRUCTURAL TRAITS       (functor_value_type, is_fmappable, concepts)

II.   GENERIC OPERATIONS      (functor_map)
      -------------------------------------
*/


#ifndef DJINTERP_TEST_FUNCTOR_TESTS_
#define DJINTERP_TEST_FUNCTOR_TESTS_ 1

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
// djinterp (module under test)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/functor.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// -- mapping functions ----------------------------------------------------

struct dbl
{
    constexpr int operator()(int _x) const { return _x * 2; }
};

struct DAdd1
{
    constexpr int operator()(int _x) const { return _x + 1; }
};

// DDblThenAdd1: the composite (add1 . dbl), written directly, for the
// composition law.
struct DDblThenAdd1
{
    constexpr int operator()(int _x) const { return (_x * 2) + 1; }
};

struct DIdentFn
{
    constexpr int operator()(int _x) const { return _x; }
};

struct is_even
{
    constexpr bool operator()(int _x) const { return (_x % 2) == 0; }
};

struct DToStr
{
    std::string operator()(int _x) const { return std::string(_x, 'x'); }
};

// DWantsString: a function that CANNOT be applied to an int functor -- the
// finer-grained question is_fmappable answers.
struct DWantsString
{
    std::string operator()(const std::string& _s) const { return _s; }
};


// -- MONAD fixture #1: tmaybe (specializes monad_traits ONLY) -------------

// tmaybe
//   fixture monad: a maybe-shaped context. It receives a monad_traits
// specialization and NO functor_traits of its own -- the blanket bridge is what
// makes it a functor.
template<typename _Type>
struct DTmaybe
{
    using value_type = _Type;

    _Type value;
    bool  has;
};

template<typename _Type>
constexpr
DTmaybe<_Type> just(
    _Type _x
)
{
    return DTmaybe<_Type>{ _x, true };
}

template<typename _Type>
constexpr
DTmaybe<_Type> nothing()
{
    return DTmaybe<_Type>{ _Type{}, false };
}


// -- MONAD fixture #2: tbox (a SECOND, unrelated monad) -------------------

// tbox
//   fixture monad: the identity context -- always holds a value. Present to show
// that a NEW monad becomes a functor with zero per-type functor wiring.
template<typename _Type>
struct DTbox
{
    using value_type = _Type;

    _Type value;
};

template<typename _Type>
constexpr
DTbox<_Type> boxed(
    _Type _x
)
{
    return DTbox<_Type>{ _x };
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
    constexpr
    auto operator()(
        _Arg _a
    ) const -> decltype(g(f(_a)))
    {
        return g(f(_a));
    }
};

// DLazyView
//   fixture functor: a source and a pending function. Mapping it composes into a
// DLazyView over a DIFFERENT function type, so its mapped type follows from the
// call -- there is no single F<U> to name, and so it supplies NO rebind. It is
// NOT a monad, so the blanket bridge cannot claim it.
template<typename _Src,
         typename _Fn>
struct DLazyView
{
    _Src src;
    _Fn  fn;

    constexpr auto get() const -> decltype(fn(src)) { return fn(src); }
};

template<typename _Src,
         typename _Fn>
constexpr
DLazyView<_Src, _Fn> make_view(
    _Src _s,
    _Fn  _f
)
{
    return DLazyView<_Src, _Fn>{ _s, _f };
}


// -- negatives ------------------------------------------------------------

// DNotAFunctor: no functor_traits specialization at all.
struct DNotAFunctor
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
using vt_helper = internal::DFunctorValueTypeHelper<void, _Functor>;

// bump
//   the header's own USAGE example: generic over ANY functor, named once.
template<typename _Functor,
         typename _Function>
constexpr
auto bump(
    _Functor&& _fa,
    _Function  _fn
) -> decltype(::djinterp::functor_map(std::forward<_Functor>(_fa), _fn))
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
#endif  // !DTEST_SPEC_MODE  (fixtures)



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


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// functor_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
functor_spec()
{
    return dt::module_spec{
        "functor",
        "functor_traits, is_functor (+ _v), and the absent rebind",
        {
            dt::block_spec{
                "protocol",
                "Protocol",
                {
                    { "tests_is_functor_positive",
                      "Both roads into the protocol are recognised: a type "
                      "that becomes a functor through the blanket monad "
                      "bridge, and one carrying its own explicit "
                      "specialization",
                      &tests_is_functor_positive },
                    { "tests_is_functor_negative",
                      "Everything else is refused -- including a bare lambda, "
                      "which is a mapping FUNCTION, not a context to map over",
                      &tests_is_functor_negative },
                    { "tests_is_functor_decay",
                      "Detection applies std::decay, so cv-qualifiers and "
                      "references are stripped first",
                      &tests_is_functor_decay },
                    { "tests_is_functor_requires_marker",
                      "Detection keys on the is_specialized marker, not on "
                      "the mere presence of a functor_traits specialization "
                      "or of a map",
                      &tests_is_functor_requires_marker },
                    { "tests_functor_traits_primary_is_undefined",
                      "The primary template is declared but left UNDEFINED, "
                      "so a use on a non-functor is a clean resolution error "
                      "rather than a silently wrong answer",
                      &tests_functor_traits_primary_is_undefined },
                    { "tests_functor_traits_surface",
                      "A specialization exposes exactly the protocol surface: "
                      "the marker, the inner value type, and map",
                      &tests_functor_traits_surface },
                    { "tests_is_functor_v_agrees",
                      "The _v shorthand is exactly the trait's value",
                      &tests_is_functor_v_agrees },
                    { "tests_rebind_is_not_core_protocol",
                      "The design point, made checkable",
                      &tests_rebind_is_not_core_protocol },
                }
            },
            dt::block_spec{
                "bridge",
                "Bridge",
                {
                    { "tests_bridge_every_monad_is_a_functor",
                      "The headline claim",
                      &tests_bridge_every_monad_is_a_functor },
                    { "tests_bridge_is_keyed_on_is_monad",
                      "The bridge is keyed on is_monad, so monad-ness IMPLIES "
                      "functor-ness -- but not the reverse",
                      &tests_bridge_is_keyed_on_is_monad },
                    { "tests_bridge_derives_map_from_monad_map",
                      "The bridge derives the functor operation from the "
                      "monad's own bind + unit, via monad_map",
                      &tests_bridge_derives_map_from_monad_map },
                    { "tests_bridge_value_type_from_monad",
                      "The bridge takes the functor's inner type straight "
                      "from monad_value_type",
                      &tests_bridge_value_type_from_monad },
                    { "tests_bridge_rebind_from_monad",
                      "Where the monad names an F<U>, the bridge passes it "
                      "through as the functor's rebind -- even though rebind "
                      "is not part of the core functor protocol",
                      &tests_bridge_rebind_from_monad },
                    { "tests_bridge_zero_wiring_for_a_new_monad",
                      "\"Any future monad participates with no per-type "
                      "specialization.\" tbox is that future monad: it is "
                      "unrelated to tmaybe, was given only a monad_traits, "
                      "and is a fully working functor",
                      &tests_bridge_zero_wiring_for_a_new_monad },
                    { "tests_bridge_does_not_overlap_explicit",
                      "\"A view / producer is not a monad, so its explicit "
                      "specialization never overlaps this one.\" The bridge's "
                      "enable_if<is_monad<F>> simply does not match a "
                      "non-monad, so there is no ambiguity to resolve",
                      &tests_bridge_does_not_overlap_explicit },
                    { "tests_bridge_preserves_monad_context",
                      "The bridge routes through bind + unit, so the "
                      "surrounding context is untouched: mapping an absent "
                      "value leaves it absent, and the function is never "
                      "applied",
                      &tests_bridge_preserves_monad_context },
                }
            },
            dt::block_spec{
                "structural",
                "Structural",
                {
                    { "tests_functor_value_type",
                      "The inner value type T of a functor F -- the first of "
                      "the finer-grained questions generic code depends on",
                      &tests_functor_value_type },
                    { "tests_functor_value_type_t_alias",
                      "The _t alias is exactly the trait's type",
                      &tests_functor_value_type_t_alias },
                    { "tests_functor_value_type_helper_sfinae",
                      "The extraction is built on a SFINAE helper whose "
                      "primary is a COMPLETE but MEMBERLESS struct -- a soft "
                      "failure, yielding no ::type rather than an error",
                      &tests_functor_value_type_helper_sfinae },
                    { "tests_is_fmappable_positive",
                      "The second finer-grained question: is a given "
                      "(functor, function) PAIR mappable? It is, when "
                      "functor_map(F, Fn) is a well-formed expression",
                      &tests_is_fmappable_positive },
                    { "tests_is_fmappable_negative",
                      "It is strictly finer-grained than is_functor: a "
                      "perfectly good functor paired with a function that "
                      "cannot be applied to its inner type is NOT mappable",
                      &tests_is_fmappable_negative },
                    { "tests_is_fmappable_does_not_require_the_marker",
                      "A divergence worth pinning",
                      &tests_is_fmappable_does_not_require_the_marker },
                    { "tests_is_fmappable_v_agrees",
                      "The _v shorthand is exactly the trait's value",
                      &tests_is_fmappable_v_agrees },
                    { "tests_functor_concept",
                      "Functor is the PascalCase typeclass face of "
                      "is_functor, and it really GATES overload resolution",
                      &tests_functor_concept },
                    { "tests_fmappable_with_concept",
                      "fmappable_with is the concept face of is_fmappable "
                      "(mirroring monad's mappable_with), and it gates on the "
                      "PAIR, not merely on the functor",
                      &tests_fmappable_with_concept },
                    { "tests_concepts_gating",
                      "The concept faces are gated to C++20, leaving the "
                      "traits as the always-present floor beneath them",
                      &tests_concepts_gating },
                }
            },
            dt::block_spec{
                "map",
                "Map",
                {
                    { "tests_functor_map_over_a_monad",
                      "The header's own example: map a plain function over a "
                      "context, without disturbing the context",
                      &tests_functor_map_over_a_monad },
                    { "tests_functor_map_preserves_context",
                      "map applies the function to the value(s) INSIDE the "
                      "context and leaves the surrounding shape alone -- it "
                      "is not a fold, and it cannot create or destroy a "
                      "context",
                      &tests_functor_map_preserves_context },
                    { "tests_functor_map_over_a_view",
                      "For a context whose mapped type depends on the mapping "
                      "function, functor_map is simply the lazy transform "
                      "that context already provides -- named uniformly",
                      &tests_functor_map_over_a_view },
                    { "tests_functor_map_changes_inner_type",
                      "The mapping function is T -> U, so the context comes "
                      "back over U",
                      &tests_functor_map_changes_inner_type },
                    { "tests_functor_map_law_identity",
                      "The first functor law: mapping the identity function "
                      "changes nothing",
                      &tests_functor_map_law_identity },
                    { "tests_functor_map_law_composition",
                      "The second functor law: mapping f then g is mapping (g ",
                      &tests_functor_map_law_composition },
                    { "tests_functor_map_result_type",
                      "The result type is whatever the INSTANCE's map "
                      "produces -- F<U> for a concrete context, the "
                      "instance's transformed view otherwise -- so it is "
                      "deduced, never named by the protocol",
                      &tests_functor_map_result_type },
                    { "tests_functor_map_forwarding",
                      "functor_map takes the functor and the function by "
                      "forwarding reference and forwards both on to the "
                      "instance's map",
                      &tests_functor_map_forwarding },
                    { "tests_functor_map_constexpr",
                      "DUAL DOMAIN",
                      &tests_functor_map_constexpr },
                    { "tests_functor_map_generic_over_any_functor",
                      "The point of the protocol: one generic function, "
                      "written once against functor_map, works over EVERY "
                      "functor -- a monad reached through the bridge, a "
                      "second unrelated monad, and a view with its own "
                      "specialization",
                      &tests_functor_map_generic_over_any_functor },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE INSTANCES  (djinterp scope)                         ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<tmaybe<T>>  -- the ONLY wiring tmaybe receives. No functor_traits
// specialization exists for it anywhere: the blanket bridge supplies that.
#ifndef DTEST_SPEC_MODE  // fixture instances - normal mode only
template<typename _Type>
struct monad_traits<testing::DTmaybe<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = testing::DTmaybe<_To>;

    static D_CONSTEXPR
    testing::DTmaybe<_Type> unit(
        _Type _x
    )
    {
        return testing::just(_x);
    }

    template<typename _Function>
    static D_CONSTEXPR
    auto bind(
        const testing::DTmaybe<_Type>& _m,
        _Function                      _f
    ) -> decltype(_f(_m.value))
    {
        using result_t = decltype(_f(_m.value));
        using inner_t  = typename result_t::value_type;
        return _m.has ? _f(_m.value) : result_t{ inner_t{}, false };
    }
};

// monad_traits<tbox<T>>  -- a SECOND monad, wired identically and no further.
template<typename _Type>
struct monad_traits<testing::DTbox<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = testing::DTbox<_To>;

    static D_CONSTEXPR
    testing::DTbox<_Type> unit(
        _Type _x
    )
    {
        return testing::boxed(_x);
    }

    template<typename _Function>
    static D_CONSTEXPR
    auto bind(
        const testing::DTbox<_Type>& _m,
        _Function                    _f
    ) -> decltype(_f(_m.value))
    {
        return _f(_m.value);
    }
};

// functor_traits<DLazyView<Src, Fn>>  -- an EXPLICIT specialization for a type
// that is NOT a monad. Its mapped type depends on the mapping function, so it
// names no rebind; map's result follows from the call alone.
template<typename _Src,
         typename _Fn>
struct functor_traits<testing::DLazyView<_Src, _Fn>, void>
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
    testing::DLazyView<_Src, testing::composed<_Fn, _Gn> >
    map(
        const testing::DLazyView<_Src, _Fn>& _v,
        _Gn                                  _g
    )
    {
        return testing::DLazyView<_Src, testing::composed<_Fn, _Gn> >{
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
#endif  // !DTEST_SPEC_MODE  (fixture instances)


NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTOR_TESTS_
