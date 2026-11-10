/*******************************************************************************
* djinterp [test]                                           polynomial_tests.hpp
*
*   Unit-test declarations for parse/grammar/polynomial.hpp.  One declaration
* group per section of the module under test (value-level vocabulary, type-
* level shapes, functor instances, traversable instances); every test is a
* niladic bool predicate returning true on pass, so the same functions
* are the unit tests the spec provider hands to run_module.  Definitions live
* in the per-section .cpp files:
*
*     polynomial_tests_values.cpp       -- I.   value-level poly_* vocabulary
*     polynomial_tests_shapes.cpp       -- II.  type-level shapes (poly_*_t)
*     polynomial_tests_functor.cpp      -- III. functor_traits + functor_map
*                                                (incl. cata/ana integration)
*     polynomial_tests_traversable.cpp  -- IV.  traversable_traits + traverse
*
*   TEST EFFECT.  The traversable instances need a concrete applicative F to
* thread.  The framework's maybe / result live in headers not required here, so
* this suite supplies its own minimal effect, DTestMaybe<T>: specialising
* monad_traits for it makes it a Functor and an Applicative for free (via the
* monad->functor and monad->applicative bridges), which is all traverse needs.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/polynomial_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.11
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    TEST HELPERS  (sentinels, float compare, the DTestMaybe effect, nat_f)
      ----------------------------------------------------------------------

I.    VALUE-LEVEL POLY VOCABULARY
      ---------------------------

II.   TYPE-LEVEL SHAPES
      -----------------

III.  FUNCTOR INSTANCES
      -----------------

IV.   TRAVERSABLE INSTANCES
      ---------------------
*/


#ifndef DJINTERP_TEST_POLYNOMIAL_TESTS_
#define DJINTERP_TEST_POLYNOMIAL_TESTS_ 1

// std
#include <cmath>
#include <type_traits>
// djinterp
// djinterp (module under test; pulls the whole functional + meta closure)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/polynomial.hpp"
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
///             0.    TEST HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// the poly_* vocabulary is djinterp's own (functional/polynomial.hpp), so
// djinterp::testing names it unqualified; it was djinterp::parse's.


// DNotAFunctor
//   type: a plain struct with no functor_traits / traversable_traits
// specialisation, used as the negative case in the detection-trait tests.
struct DNotAFunctor
{
};


// close_enough
//   function: tolerant floating-point equality for the double-valued checks.
inline
bool close_enough(
    double _a,
    double _b,
    double _epsilon = 1e-9
)
{
    return (std::fabs(_a - _b) <= _epsilon);
}


// DTestMaybe
//   type: the minimal applicative effect threaded by the traverse tests -- an
// optional value (has ? val : nothing).  A monad_traits specialisation (below,
// at djinterp scope) makes it a Functor and an Applicative via the protocol
// bridges, which is everything traverse / sequence require of the effect F.
template<typename _Value>
struct DTestMaybe
{
    bool    has;
    _Value  val;

    DTestMaybe()
        : has(false),
          val()
    {}

    DTestMaybe(
        bool          _has,
        const _Value& _val
    )
        : has(_has),
          val(_val)
    {}
};

// t_just / t_nothing
//   helpers: the two constructors of the test effect, named for readability.
template<typename _Value>
inline
DTestMaybe<_Value> t_just(
    const _Value& _v
)
{
    return DTestMaybe<_Value>(true, _v);
}

template<typename _Value>
inline
DTestMaybe<_Value> t_nothing()
{
    return DTestMaybe<_Value>();
}


// nat_f
//   alias: the Peano base functor 1 + X, assembled from the poly_* vocabulary
// (poly_unit is the Zero arm, poly_var the Succ arm).  A single-argument
// template, so mu<nat_f> is a well-formed fixed point whose fold/unfold the
// cata / ana integration test drives through the poly functor instances.
template<typename _X>
using nat_f = poly_sum<poly_unit<_X>, poly_var<_X> >;
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    VALUE-LEVEL POLY VOCABULARY                           ///
///////////////////////////////////////////////////////////////////////////////

// poly_var / poly_unit / poly_const / poly_sum / poly_product: construction,
// members, factories, and the shared-value_type contracts
bool tests_poly_var();
bool tests_poly_unit();
bool tests_poly_const();
bool tests_poly_sum();
bool tests_poly_product();
bool tests_value_level_value_types();


///////////////////////////////////////////////////////////////////////////////
///             II.   TYPE-LEVEL SHAPES                                     ///
///////////////////////////////////////////////////////////////////////////////

// poly_constant_t / poly_recursion_t / poly_sum_t / poly_product_t /
// poly_compose_t / poly_mu_t: member typedefs and the product arity
bool tests_poly_constant_t();
bool tests_poly_recursion_t();
bool tests_poly_sum_t();
bool tests_poly_product_t();
bool tests_poly_compose_t();
bool tests_poly_mu_t();


///////////////////////////////////////////////////////////////////////////////
///             III.  FUNCTOR INSTANCES                                     ///
///////////////////////////////////////////////////////////////////////////////

// functor_traits specialisations for every poly_* carrier, functor_map, the
// functor laws, and the cata / ana integration those instances exist to serve
bool tests_is_functor_poly();
bool tests_is_functor_negative();
bool tests_functor_value_type();
bool tests_functor_rebind();
bool tests_functor_map_poly_var();
bool tests_functor_map_poly_unit();
bool tests_functor_map_poly_const();
bool tests_functor_map_poly_sum();
bool tests_functor_map_poly_product();
bool tests_functor_law_identity();
bool tests_functor_law_composition();
bool tests_functor_cata_integration();


///////////////////////////////////////////////////////////////////////////////
///             IV.   TRAVERSABLE INSTANCES                                 ///
///////////////////////////////////////////////////////////////////////////////

// traversable_traits specialisations (only poly_var / poly_unit / poly_const --
// poly_sum / poly_product are Functor-only), traverse, and sequence
bool tests_is_traversable_poly();
bool tests_is_traversable_negative();
bool tests_traversable_value_type();
bool tests_traverse_poly_var();
bool tests_traverse_poly_unit();
bool tests_traverse_poly_const();
bool tests_sequence_poly_var();
bool tests_sequence_poly_unit_const();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// polynomial_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
polynomial_spec()
{
    return dt::module_spec{
        "polynomial",
        "poly_var / poly_unit / poly_const / poly_sum / poly_product",
        {
            dt::block_spec{
                "values",
                "Values",
                {
                    { "tests_poly_var",
                      "Exercises the recursive-position carrier poly_var<X> "
                      "(F(X) = X)",
                      &tests_poly_var },
                    { "tests_poly_unit",
                      "Exercises the unit carrier poly_unit<X> (F(X) = 1, X "
                      "phantom)",
                      &tests_poly_unit },
                    { "tests_poly_const",
                      "Exercises the constant carrier poly_const<C, X> (F(X) "
                      "= K_C, X phantom)",
                      &tests_poly_const },
                    { "tests_poly_sum",
                      "Exercises the binary-sum carrier poly_sum<L, R> (F = L "
                      "+ R)",
                      &tests_poly_sum },
                    { "tests_poly_product",
                      "Exercises the binary-product carrier poly_product<F, "
                      "G> (F = First x Second)",
                      &tests_poly_product },
                }
            },
            dt::block_spec{
                "shapes",
                "Shapes",
                {
                    { "tests_poly_constant_t",
                      "Exercises the type-level constant shape "
                      "poly_constant_t<C> (F(X) = K_C)",
                      &tests_poly_constant_t },
                    { "tests_poly_recursion_t",
                      "Exercises the type-level recursion marker "
                      "poly_recursion_t (F(X) = X)",
                      &tests_poly_recursion_t },
                    { "tests_poly_sum_t",
                      "Exercises the type-level sum shape poly_sum_t<L, R> "
                      "(F(X) = L(X) + R(X))",
                      &tests_poly_sum_t },
                    { "tests_poly_product_t",
                      "Exercises the type-level product shape "
                      "poly_product_t<Variants...>",
                      &tests_poly_product_t },
                    { "tests_poly_compose_t",
                      "Exercises the type-level composition shape "
                      "poly_compose_t<Outer, Inner>",
                      &tests_poly_compose_t },
                    { "tests_poly_mu_t",
                      "Exercises the type-level initial-algebra shape "
                      "poly_mu_t<F>",
                      &tests_poly_mu_t },
                }
            },
            dt::block_spec{
                "functor",
                "Functor",
                {
                    { "tests_is_functor_poly",
                      "is functor poly",
                      &tests_is_functor_poly },
                    { "tests_is_functor_negative",
                      "Confirms is_functor is false for non-functors, and "
                      "resolves cleanly (no hard error) on the undefined "
                      "primary",
                      &tests_is_functor_negative },
                    { "tests_functor_value_type",
                      "functor value type",
                      &tests_functor_value_type },
                    { "tests_functor_rebind",
                      "Confirms functor_traits<F>::rebind<U> re-parameterises "
                      "each carrier over U, recursing structurally through "
                      "the composites",
                      &tests_functor_rebind },
                    { "tests_functor_map_poly_var",
                      "Exercises functor_map over the recursive-position "
                      "carrier",
                      &tests_functor_map_poly_var },
                    { "tests_functor_map_poly_unit",
                      "Exercises functor_map over the unit carrier",
                      &tests_functor_map_poly_unit },
                    { "tests_functor_map_poly_const",
                      "Exercises functor_map over the constant carrier",
                      &tests_functor_map_poly_const },
                    { "tests_functor_map_poly_sum",
                      "Exercises functor_map over the binary sum",
                      &tests_functor_map_poly_sum },
                    { "tests_functor_map_poly_product",
                      "Exercises functor_map over the binary product",
                      &tests_functor_map_poly_product },
                    { "tests_functor_law_identity",
                      "Confirms the first functor law: map(id) is "
                      "structurally the identity",
                      &tests_functor_law_identity },
                    { "tests_functor_law_composition",
                      "Confirms the second functor law: map(g ",
                      &tests_functor_law_composition },
                    { "tests_functor_cata_integration",
                      "The payoff test: the poly functor instances are what "
                      "recursion.hpp's cata / ana / hylo call to rebuild a "
                      "layer, so folding a poly-built fixed point validates "
                      "those instances end-to-end",
                      &tests_functor_cata_integration },
                }
            },
            dt::block_spec{
                "traversable",
                "Traversable",
                {
                    { "tests_is_traversable_poly",
                      "Confirms the carriers that ARE Traversable",
                      &tests_is_traversable_poly },
                    { "tests_is_traversable_negative",
                      "Confirms the functor / traversable asymmetry: poly_sum "
                      "and poly_product are Functors (Section III) but were "
                      "deliberately NOT given traversable_traits, so they are "
                      "not Traversable",
                      &tests_is_traversable_negative },
                    { "tests_traversable_value_type",
                      "Confirms traversable_value_type_t reports the inner X "
                      "of each Traversable carrier",
                      &tests_traversable_value_type },
                    { "tests_traverse_poly_var",
                      "Exercises traverse over the recursive position, "
                      "threading DTestMaybe",
                      &tests_traverse_poly_var },
                    { "tests_traverse_poly_unit",
                      "Exercises traverse over the unit carrier",
                      &tests_traverse_poly_unit },
                    { "tests_traverse_poly_const",
                      "Exercises traverse over the constant carrier",
                      &tests_traverse_poly_const },
                    { "tests_sequence_poly_var",
                      "Exercises sequence = traverse(., id) over the "
                      "recursive position, inverting poly_var<F<A>> into "
                      "F<poly_var<A>>",
                      &tests_sequence_poly_var },
                    { "tests_sequence_poly_unit_const",
                      "Exercises sequence over the effect-free carriers",
                      &tests_sequence_poly_unit_const },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             TEST-EFFECT MONAD INSTANCE  (djinterp scope)                ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<testing::DTestMaybe<_Value>>
//   specialisation: makes the test effect a monad -- and therefore, via the
// protocol bridges, a Functor and an Applicative.  unit injects a value (just);
// bind runs the Kleisli arrow on a present value and short-circuits an absent
// one (nothing propagates).  Written at djinterp scope to match the primary.
#ifndef DTEST_SPEC_MODE  // fixture instances - normal mode only
template<typename _Value>
struct monad_traits<testing::DTestMaybe<_Value> >
{
    using is_specialized = std::true_type;
    using value_type     = _Value;

    template<typename _To>
    using rebind = testing::DTestMaybe<_To>;

    static
    testing::DTestMaybe<_Value> unit(
        const _Value& _value
    )
    {
        return testing::DTestMaybe<_Value>(true, _value);
    }

    template<typename _Function>
    static
    auto bind(
        const testing::DTestMaybe<_Value>& _m,
        _Function                          _function
    ) -> decltype(_function(_m.val))
    {
        using result_monad = decltype(_function(_m.val));

        if (_m.has)
        {
            return _function(_m.val);
        }

        result_monad _absent;
        _absent.has = false;

        return _absent;
    }
};
#endif  // !DTEST_SPEC_MODE  (fixture instances)


NS_END  // djinterp


#endif  // DJINTERP_TEST_POLYNOMIAL_TESTS_
