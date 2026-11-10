/*******************************************************************************
* djinterp [test]                                           profunctor_tests.hpp
*
*   Unit-test declarations for core/functional/profunctor.hpp.  One declaration
* group per section of the module under test:
*
*     profunctor_tests_arrow.cpp       -- I.   profn<F> + make_profn
*     profunctor_tests_protocol.cpp    -- II.  profunctor_traits, is_profunctor,
*                                               the internal helpers, the concept
*     profunctor_tests_operations.cpp  -- III. dimap / lmap / rmap and the laws
*     profunctor_tests_instance.cpp    -- IV.  the profn<F> instance
*
*   FIXTURES.  The generic operations dispatch through profunctor_traits, so to
* prove they are not hard-wired to profn the suite also defines a SECOND,
* distinct profunctor instance, DPfArrow<F> (its own carrier, make function, and
* profunctor_traits::dimap).  A handful of constexpr function objects (doubler,
* add_one, add_ten, DEvenP, b2i) drive the conditional-constexpr paths under
* static_assert -- including input/output type changes at compile time -- while
* runtime tests use lambdas.  Two negatives pin the detector: DNotProfunctor
* (no specialisation) and DPfNoMarker (a profunctor_traits WITHOUT the
* is_specialized marker, which must read as not-a-profunctor).
*
*   Profunctor equality is behavioural: two arrows are "the same" when they
* agree on sample inputs (a profn carries a callable and has no operator==), so
* the law tests evaluate both sides at several points.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/profunctor_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.11
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (constexpr functors, negatives, the second instance, traits)
      ----------------------------------------------------------------------

I.    ARROW WRAPPER
      -------------

II.   PROTOCOL + DETECTION
      --------------------

III.  OPERATIONS (dimap / lmap / rmap)
      --------------------------------

IV.   INSTANCE (profn<F>)
      -------------------
*/


#ifndef DJINTERP_TEST_PROFUNCTOR_TESTS_
#define DJINTERP_TEST_PROFUNCTOR_TESTS_ 1

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
// djinterp (module under test)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/profunctor.hpp"
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

// -- constexpr function objects (drive the constexpr paths) ---------------

// doubler / add_one / add_ten: int -> int, constexpr.
struct doubler
{
    constexpr int operator()(int _x) const { return _x * 2; }
};

struct add_one
{
    constexpr int operator()(int _x) const { return _x + 1; }
};

struct add_ten
{
    constexpr int operator()(int _x) const { return _x + 10; }
};

// DEvenP: int -> bool, constexpr -- a covariant (output) type change.
struct DEvenP
{
    constexpr bool operator()(int _x) const { return (_x % 2) == 0; }
};

// b2i: bool -> int, constexpr -- a contravariant (input) type change.
struct DB2i
{
    constexpr int operator()(bool _b) const { return _b ? 10 : 0; }
};


// -- negatives ------------------------------------------------------------

// DNotProfunctor: no profunctor_traits specialisation at all.
struct DNotProfunctor
{
};

// DPfNoMarker: a profunctor_traits specialisation WITHOUT is_specialized
// (defined below at djinterp scope) -- must read as not-a-profunctor.
struct DPfNoMarker
{
};


// -- a second, distinct profunctor instance (generic-dispatch witness) ----

// DPfArrow
//   fixture profunctor: a parallel arrow wrapper, unrelated to profn, so that
// dimap / lmap / rmap can be shown to dispatch through the trait rather than
// being wired to profn.
template<typename _Fn>
struct DPfArrow
{
    _Fn f;

    template<typename _Arg>
    D_CONSTEXPR
    auto operator()(
        _Arg&& _arg
    ) const -> decltype(f(std::forward<_Arg>(_arg)))
    {
        return f(std::forward<_Arg>(_arg));
    }
};

template<typename _Fn>
D_CONSTEXPR
DPfArrow<typename std::decay<_Fn>::type> make_pf_arrow(
    _Fn&& _f
)
{
    return DPfArrow<typename std::decay<_Fn>::type>{ std::forward<_Fn>(_f) };
}

// DPfCompose: the composed callable post . fn . pre behind DPfArrow's dimap.
template<typename _Pre,
         typename _Fn,
         typename _Post>
struct DPfCompose
{
    _Pre  pre;
    _Fn   fn;
    _Post post;

    template<typename _Arg>
    D_CONSTEXPR
    auto operator()(
        _Arg&& _arg
    ) const -> decltype(post(fn(pre(std::forward<_Arg>(_arg)))))
    {
        return post(fn(pre(std::forward<_Arg>(_arg))));
    }
};


// -- helper trait: is the result of dimap still a profn? ------------------

// is_profn_type
//   trait: true exactly for profn<F> -- used to confirm dimap returns a fresh
// arrow WITHOUT unwrapping.
template<typename _Type>
struct is_profn_type : std::false_type
{
};

template<typename _Fn>
struct is_profn_type<DProfn<_Fn> > : std::true_type
{
};

// DFwdProbe
//   fixture callable: reports which reference category it was called with, so
// profn's perfect-forwarding of the argument can be observed.
struct DFwdProbe
{
    int operator()(int& ) const       { return 1; }   // lvalue
    int operator()(int&&) const       { return 2; }   // rvalue
    int operator()(const int&) const  { return 3; }   // const lvalue
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    ARROW WRAPPER                                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_profn_construct();
bool tests_profn_call();
bool tests_make_profn_decay();
bool tests_profn_forwarding();
bool tests_profn_constexpr();
bool tests_profn_return_type();


///////////////////////////////////////////////////////////////////////////////
///             II.   PROTOCOL + DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_profunctor_positive();
bool tests_is_profunctor_negative();
bool tests_is_profunctor_cvref();
bool tests_is_profunctor_requires_marker();
bool tests_profunctor_traits_members();
bool tests_profunctor_concept();
bool tests_internal_helpers();


///////////////////////////////////////////////////////////////////////////////
///             III.  OPERATIONS (dimap / lmap / rmap)                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_dimap_both_ends();
bool tests_dimap_type_change();
bool tests_lmap_input_only();
bool tests_rmap_output_only();
bool tests_law_identity();
bool tests_law_composition();
bool tests_rmap_covariant_composition();
bool tests_lmap_contravariant_composition();
bool tests_dimap_equals_lmap_rmap();
bool tests_operations_constexpr();


///////////////////////////////////////////////////////////////////////////////
///             IV.   INSTANCE (profn<F>)                                   ///
///////////////////////////////////////////////////////////////////////////////

bool tests_instance_composition_order();
bool tests_instance_returns_profn();
bool tests_instance_composable();
bool tests_instance_preserves_original();
bool tests_instance_is_specialized();
bool tests_instance_generic_dispatch();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// profunctor_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
profunctor_spec()
{
    return dt::module_spec{
        "profunctor",
        "construction, call forwarding, make_profn decay, constexpr",
        {
            dt::block_spec{
                "arrow",
                "Arrow",
                {
                    { "tests_profn_construct",
                      "A profn wraps a callable and exposes it as the fn "
                      "member",
                      &tests_profn_construct },
                    { "tests_profn_call",
                      "operator() forwards to the wrapped callable, so a "
                      "profn is used exactly like the function it wraps",
                      &tests_profn_call },
                    { "tests_make_profn_decay",
                      "make_profn deduces and DECAYS the callable type, "
                      "stripping references and cv-qualifiers",
                      &tests_make_profn_decay },
                    { "tests_profn_forwarding",
                      "operator() perfectly forwards its argument, preserving "
                      "its reference category",
                      &tests_profn_forwarding },
                    { "tests_profn_constexpr",
                      "profn's constructor and call operator are usable in a "
                      "constant expression",
                      &tests_profn_constexpr },
                    { "tests_profn_return_type",
                      "The call operator's result type is exactly the wrapped "
                      "callable's return type",
                      &tests_profn_return_type },
                }
            },
            dt::block_spec{
                "protocol",
                "Protocol",
                {
                    { "tests_is_profunctor_positive",
                      "Both the canonical arrow and the second fixture "
                      "instance are recognised",
                      &tests_is_profunctor_positive },
                    { "tests_is_profunctor_negative",
                      "Non-profunctors are rejected -- including a bare "
                      "callable, which is precisely why the arrow wrapper "
                      "exists",
                      &tests_is_profunctor_negative },
                    { "tests_is_profunctor_cvref",
                      "Detection strips cv-qualifiers and references before "
                      "testing",
                      &tests_is_profunctor_cvref },
                    { "tests_is_profunctor_requires_marker",
                      "Detection keys on the is_specialized marker, not on "
                      "the mere presence of a profunctor_traits specialisation",
                      &tests_is_profunctor_requires_marker },
                    { "tests_profunctor_traits_members",
                      "A specialisation exposes exactly the protocol surface",
                      &tests_profunctor_traits_members },
                    { "tests_profunctor_concept",
                      "The Profunctor concept mirrors is_profunctor (C++20 "
                      "only)",
                      &tests_profunctor_concept },
                    { "tests_internal_helpers",
                      "The two named internal functors behave as specified, "
                      "on several floors and in a constant expression",
                      &tests_internal_helpers },
                }
            },
            dt::block_spec{
                "operations",
                "Operations",
                {
                    { "tests_dimap_both_ends",
                      "dimap pre-composes pre onto the input and "
                      "post-composes post onto the output",
                      &tests_dimap_both_ends },
                    { "tests_dimap_type_change",
                      "dimap can change BOTH parameters -- the input from A "
                      "to A' and the output from B to B'",
                      &tests_dimap_type_change },
                    { "tests_lmap_input_only",
                      "lmap adapts only the input (the contravariant map): "
                      "pre : A' -> A, output untouched",
                      &tests_lmap_input_only },
                    { "tests_rmap_output_only",
                      "rmap adapts only the output (the covariant map / the "
                      "profunctor's fmap): post : B -> B', input untouched",
                      &tests_rmap_output_only },
                    { "tests_law_identity",
                      "The identity law: adapting with identities changes "
                      "nothing",
                      &tests_law_identity },
                    { "tests_law_composition",
                      "The dimap composition law: dimap(dimap(p, pre1, "
                      "post1), pre2, post2) equals dimap(p, pre1 ",
                      &tests_law_composition },
                    { "tests_rmap_covariant_composition",
                      "rmap satisfies the functor composition law: "
                      "rmap(rmap(p, post1), post2) == rmap(p, post2 ",
                      &tests_rmap_covariant_composition },
                    { "tests_lmap_contravariant_composition",
                      "lmap composes contravariantly: lmap(lmap(p, pre1), "
                      "pre2) == lmap(p, pre1 ",
                      &tests_lmap_contravariant_composition },
                    { "tests_dimap_equals_lmap_rmap",
                      "dimap factors through lmap and rmap either way round",
                      &tests_dimap_equals_lmap_rmap },
                    { "tests_operations_constexpr",
                      "dimap / lmap / rmap fold inside a constant expression, "
                      "including when they change the input or output type",
                      &tests_operations_constexpr },
                }
            },
            dt::block_spec{
                "instance",
                "Instance",
                {
                    { "tests_instance_composition_order",
                      "The profn instance's dimap builds exactly post ",
                      &tests_instance_composition_order },
                    { "tests_instance_returns_profn",
                      "dimap composes a NEW arrow without unwrapping -- the "
                      "result is again a profn",
                      &tests_instance_returns_profn },
                    { "tests_instance_composable",
                      "A dimapped arrow is itself a profunctor, so the "
                      "operations chain",
                      &tests_instance_composable },
                    { "tests_instance_preserves_original",
                      "dimap builds a fresh arrow and leaves the original "
                      "untouched (a profn holds its callable by value)",
                      &tests_instance_preserves_original },
                    { "tests_instance_is_specialized",
                      "The instance carries the marker, through the explicit "
                      "<T, void> form",
                      &tests_instance_is_specialized },
                    { "tests_instance_generic_dispatch",
                      "The generic operations dispatch through "
                      "profunctor_traits, so a SECOND, unrelated instance "
                      "(DPfArrow) works identically -- and its results are "
                      "its own carrier, not profn",
                      &tests_instance_generic_dispatch },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE PROFUNCTOR INSTANCES  (djinterp scope)              ///
///////////////////////////////////////////////////////////////////////////////

// profunctor_traits<DPfArrow<_Fn>>  -- the second instance: dimap composes a
// fresh DPfArrow around post . fn . pre, exactly as profn does for itself.
#ifndef DTEST_SPEC_MODE  // fixture instances - normal mode only
template<typename _Fn>
struct profunctor_traits<testing::DPfArrow<_Fn>, void>
{
    using is_specialized = std::true_type;

    template<typename _Pre,
             typename _Post>
    static D_CONSTEXPR
    testing::DPfArrow<testing::DPfCompose<_Pre, _Fn, _Post> > dimap(
        const testing::DPfArrow<_Fn>& _p,
        _Pre                          _pre,
        _Post                         _post
    )
    {
        return testing::make_pf_arrow(
            testing::DPfCompose<_Pre, _Fn, _Post>{ _pre, _p.f, _post });
    }
};

// profunctor_traits<DPfNoMarker>  -- present but WITHOUT the is_specialized
// marker, so is_profunctor<DPfNoMarker> must be false.
template<>
struct profunctor_traits<testing::DPfNoMarker, void>
{
};
#endif  // !DTEST_SPEC_MODE  (fixture instances)


NS_END  // djinterp


#endif  // DJINTERP_TEST_PROFUNCTOR_TESTS_
