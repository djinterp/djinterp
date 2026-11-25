/*******************************************************************************
* djinterp [cpp][meta]                          math_vocabulary_tests_traits.cpp
*
*   Section II of math_vocabulary_tests.hpp: the traits over every member of
* their enums, and the relation properties' bit operations.
*
*
* path:      /tests/djinterp/cpp/meta/math_vocabulary_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "math_vocabulary_tests.hpp"  // the suite


NS_DJINTERP
NS_TESTING

/*
tests_traits_cardinality
  is_finite_cardinality and is_countable_cardinality over every cardinality.
  Tests the following:
  - empty, finite_static and finite_dynamic are finite and countable
  - countable_infinite is countable, not finite
  - uncountable and unknown are neither
*/
bool tests_traits_cardinality()
{
    using C = DCardinality;

    D_MV_CHECK(is_finite_cardinality<C::empty>::value);
    D_MV_CHECK(is_finite_cardinality<C::finite_static>::value);
    D_MV_CHECK(is_finite_cardinality<C::finite_dynamic>::value);
    D_MV_CHECK(!is_finite_cardinality<C::countable_infinite>::value);
    D_MV_CHECK(!is_finite_cardinality<C::uncountable>::value);
    D_MV_CHECK(!is_finite_cardinality<C::unknown>::value);

    D_MV_CHECK(is_countable_cardinality<C::empty>::value);
    D_MV_CHECK(is_countable_cardinality<C::finite_static>::value);
    D_MV_CHECK(is_countable_cardinality<C::finite_dynamic>::value);
    D_MV_CHECK(is_countable_cardinality<C::countable_infinite>::value);
    D_MV_CHECK(!is_countable_cardinality<C::uncountable>::value);
    D_MV_CHECK(!is_countable_cardinality<C::unknown>::value);

    return true;
}

/*
tests_traits_closure
  closure_includes_left and closure_includes_right over every closure.
  Tests the following:
  - [a, b] includes both ends; (a, b) neither
  - [a, b) includes its left end only; (a, b] its right end only
  - unknown includes neither
*/
bool tests_traits_closure()
{
    D_MV_CHECK(closure_includes_left<DClosure::closed>::value);
    D_MV_CHECK(closure_includes_right<DClosure::closed>::value);
    D_MV_CHECK(!closure_includes_left<DClosure::open>::value);
    D_MV_CHECK(!closure_includes_right<DClosure::open>::value);

    D_MV_CHECK(closure_includes_left<DClosure::right_open>::value);
    D_MV_CHECK(!closure_includes_right<DClosure::right_open>::value);
    D_MV_CHECK(!closure_includes_left<DClosure::left_open>::value);
    D_MV_CHECK(closure_includes_right<DClosure::left_open>::value);

    D_MV_CHECK(!closure_includes_left<DClosure::unknown>::value);
    D_MV_CHECK(!closure_includes_right<DClosure::unknown>::value);

    return true;
}

/*
tests_traits_algebraic_structure
  The three structure traits over every structure.
  Tests the following:
  - an identity from monoid up; none for magma, semigroup or unknown
  - inverses from group up; none for monoid
  - commutativity for the abelian group, commutative ring, field and
    ordered field only
*/
bool tests_traits_algebraic_structure()
{
    using A = DAlgebraicStructure;

    D_MV_CHECK(!has_identity_element<A::unknown>::value);
    D_MV_CHECK(!has_identity_element<A::magma>::value);
    D_MV_CHECK(!has_identity_element<A::semigroup>::value);
    D_MV_CHECK(has_identity_element<A::monoid>::value);
    D_MV_CHECK(has_identity_element<A::field>::value);

    D_MV_CHECK(!has_inverses<A::monoid>::value);
    D_MV_CHECK(has_inverses<A::group>::value);
    D_MV_CHECK(has_inverses<A::ordered_field>::value);

    D_MV_CHECK(!is_commutative_structure<A::group>::value);
    D_MV_CHECK(is_commutative_structure<A::abelian_group>::value);
    D_MV_CHECK(!is_commutative_structure<A::ring>::value);
    D_MV_CHECK(!is_commutative_structure<A::unital_ring>::value);
    D_MV_CHECK(is_commutative_structure<A::commutative_ring>::value);
    D_MV_CHECK(is_commutative_structure<A::field>::value);
    D_MV_CHECK(is_commutative_structure<A::ordered_field>::value);

    return true;
}

/*
tests_traits_relation_property_bits
  DRelationProperty's bit operations and has_relation_property.
  Tests the following:
  - | combines, & intersects, ^ differs, ~ complements, within 16 bits
  - has_relation_property asks for every bit of its query, not any
  - every set has none
*/
bool tests_traits_relation_property_bits()
{
    using R = DRelationProperty;
    using U = std::underlying_type<R>::type;
    const R order = R::reflexive | R::antisymmetric | R::transitive;

    D_MV_CHECK(has_relation_property(order, R::reflexive));
    D_MV_CHECK(has_relation_property(order, R::antisymmetric | R::transitive));
    D_MV_CHECK(!has_relation_property(order, R::symmetric));
    D_MV_CHECK(!has_relation_property(order, R::reflexive | R::symmetric));
    D_MV_CHECK(has_relation_property(order, R::none));

    D_MV_CHECK((order & R::transitive) == R::transitive);
    D_MV_CHECK((order & R::symmetric) == R::none);
    D_MV_CHECK((order ^ R::reflexive) == (R::antisymmetric | R::transitive));
    D_MV_CHECK(static_cast<U>(~R::none) == static_cast<U>(0xFFFFu));
    D_MV_CHECK((~order & order) == R::none);

    return true;
}

NS_END  // testing
NS_END  // djinterp
