/******************************************************************************
* djinterp [test]                                         polynomial_tests.hpp
*
*   Unit-test declarations for parse/grammar/polynomial.hpp.  One declaration
* group per section of the module under test (value-level vocabulary, type-
* level shapes, functor instances, traversable instances); every test is a
* niladic bool predicate returning true on pass, so the same functions drive
* both the report_builder runner and any standalone harness.  Definitions live
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
* this suite supplies its own minimal effect, test_maybe<T>: specialising
* monad_traits for it makes it a Functor and an Applicative for free (via the
* monad->functor and monad->applicative bridges), which is all traverse needs.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/parse/polynomial_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    TEST HELPERS  (sentinels, float compare, the test_maybe effect, nat_f)
I.    VALUE-LEVEL POLY VOCABULARY
II.   TYPE-LEVEL SHAPES
III.  FUNCTOR INSTANCES
IV.   TRAVERSABLE INSTANCES
*/


#ifndef DJINTERP_TEST_POLYNOMIAL_TESTS_
#define DJINTERP_TEST_POLYNOMIAL_TESTS_ 1

// std
#include <cmath>
#include <type_traits>
// djinterp (module under test; pulls the whole functional + meta closure)
#include "../../parse/grammar/polynomial.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    TEST HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// bring the poly_* vocabulary into djinterp::testing so the tests can name it
// unqualified.
using parse::poly_var;
using parse::poly_unit;
using parse::poly_const;
using parse::poly_sum;
using parse::poly_product;
using parse::poly_constant_t;
using parse::poly_recursion_t;
using parse::poly_sum_t;
using parse::poly_product_t;
using parse::poly_compose_t;
using parse::poly_mu_t;


// not_a_functor
//   type: a plain struct with no functor_traits / traversable_traits
// specialisation, used as the negative case in the detection-trait tests.
struct not_a_functor
{
};


// close_enough
//   function: tolerant floating-point equality for the double-valued checks.
inline bool
close_enough(
    double _a,
    double _b,
    double _epsilon = 1e-9
)
{
    return (std::fabs(_a - _b) <= _epsilon);
}


// test_maybe
//   type: the minimal applicative effect threaded by the traverse tests -- an
// optional value (has ? val : nothing).  A monad_traits specialisation (below,
// at djinterp scope) makes it a Functor and an Applicative via the protocol
// bridges, which is everything traverse / sequence require of the effect F.
template<typename _Value>
struct test_maybe
{
    bool    has;
    _Value  val;

    test_maybe()
        : has(false),
          val()
    {}

    test_maybe(
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
inline test_maybe<_Value>
t_just(
    const _Value& _v
)
{
    return test_maybe<_Value>(true, _v);
}

template<typename _Value>
inline test_maybe<_Value>
t_nothing()
{
    return test_maybe<_Value>();
}


// nat_f
//   alias: the Peano base functor 1 + X, assembled from the poly_* vocabulary
// (poly_unit is the Zero arm, poly_var the Succ arm).  A single-argument
// template, so mu<nat_f> is a well-formed fixed point whose fold/unfold the
// cata / ana integration test drives through the poly functor instances.
template<typename _X>
using nat_f = poly_sum<poly_unit<_X>, poly_var<_X> >;


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


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             TEST-EFFECT MONAD INSTANCE  (djinterp scope)                ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<testing::test_maybe<_Value>>
//   specialisation: makes the test effect a monad -- and therefore, via the
// protocol bridges, a Functor and an Applicative.  unit injects a value (just);
// bind runs the Kleisli arrow on a present value and short-circuits an absent
// one (nothing propagates).  Written at djinterp scope to match the primary.
template<typename _Value>
struct monad_traits<testing::test_maybe<_Value> >
{
    using is_specialized = std::true_type;
    using value_type     = _Value;

    template<typename _To>
    using rebind = testing::test_maybe<_To>;

    static
    testing::test_maybe<_Value>
    unit(
        const _Value& _value
    )
    {
        return testing::test_maybe<_Value>(true, _value);
    }

    template<typename _Function>
    static
    auto bind(
        const testing::test_maybe<_Value>& _m,
        _Function                          _function
    )
    -> decltype(_function(_m.val))
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


NS_END  // djinterp


#endif  // DJINTERP_TEST_POLYNOMIAL_TESTS_
