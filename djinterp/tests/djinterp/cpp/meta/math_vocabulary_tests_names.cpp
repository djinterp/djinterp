/*******************************************************************************
* djinterp [cpp][meta]                           math_vocabulary_tests_names.cpp
*
*   Section III of math_vocabulary_tests.hpp: the old names in djinterp::math,
* each the same entity as its name in djinterp until 0.9.0-rc.1, and math's
* ordering enum beside container's.
*
*
* path:      /tests/djinterp/cpp/meta/math_vocabulary_tests_names.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "math_vocabulary_tests.hpp"  // the suite


NS_DJINTERP
NS_TESTING

/*
tests_names_old_types_are_the_new
  Each type's old names in djinterp::math are the type in djinterp.
  Tests the following:
  - the snake names: parity to algebraic_structure
  - the D names, which djinterp::math had from 2026.11.18
  - math's ordering_kind and DOrderingKind name DOrderStrength
  - arity_t and degree_t
*/
bool tests_names_old_types_are_the_new()
{
    D_MV_CHECK(std::is_same<math::parity, DParity>::value);
    D_MV_CHECK(std::is_same<math::sign, DSign>::value);
    D_MV_CHECK(std::is_same<math::handedness, DHandedness>::value);
    D_MV_CHECK(std::is_same<math::cardinality, DCardinality>::value);
    D_MV_CHECK(std::is_same<math::self_loop_policy, DSelfLoopPolicy>::value);
    D_MV_CHECK(std::is_same<math::directionality, DDirectionality>::value);
    D_MV_CHECK(std::is_same<math::orientation, DOrientation>::value);
    D_MV_CHECK(std::is_same<math::closure, DClosure>::value);
    D_MV_CHECK(std::is_same<math::strictness, DStrictness>::value);
    D_MV_CHECK(std::is_same<math::relation_property, DRelationProperty>::value);
    D_MV_CHECK(std::is_same<math::relation_kind, DRelationKind>::value);
    D_MV_CHECK(std::is_same<math::algebraic_structure,
                            DAlgebraicStructure>::value);

    D_MV_CHECK(std::is_same<math::DParity, DParity>::value);
    D_MV_CHECK(std::is_same<math::DDirectionality, DDirectionality>::value);
    D_MV_CHECK(std::is_same<math::DAlgebraicStructure,
                            DAlgebraicStructure>::value);

    D_MV_CHECK(std::is_same<math::ordering_kind, DOrderStrength>::value);
    D_MV_CHECK(std::is_same<math::DOrderingKind, DOrderStrength>::value);

    D_MV_CHECK(std::is_same<math::arity_t, arity_t>::value);
    D_MV_CHECK(std::is_same<math::degree_t, degree_t>::value);

    return true;
}

/*
tests_names_old_templates_are_the_new
  Each template's old name in djinterp::math.
  Tests the following:
  - type_pair, type_triple, value_pair and indexed_value give the same types
  - the seven traits' old names give the same values
*/
bool tests_names_old_templates_are_the_new()
{
    D_MV_CHECK(std::is_same<math::type_pair<int, char>,
                            DTypePair<int, char>>::value);
    D_MV_CHECK(std::is_same<math::type_triple<int, char, bool>,
                            DTypeTriple<int, char, bool>>::value);
    D_MV_CHECK(std::is_same<math::value_pair<int, 1, 2>,
                            DValuePair<int, 1, 2>>::value);
    D_MV_CHECK(std::is_same<math::indexed_value<0, int, 5>,
                            DIndexedValue<0, int, 5>>::value);

    D_MV_CHECK(std::is_same<
                   math::is_finite_cardinality<DCardinality::empty>,
                   is_finite_cardinality<DCardinality::empty>>::value);
    D_MV_CHECK(math::is_countable_cardinality<
                   DCardinality::uncountable>::value == false);
    D_MV_CHECK(math::closure_includes_left<DClosure::closed>::value);
    D_MV_CHECK(math::closure_includes_right<DClosure::left_open>::value);
    D_MV_CHECK(math::has_identity_element<DAlgebraicStructure::monoid>::value);
    D_MV_CHECK(math::has_inverses<DAlgebraicStructure::group>::value);
    D_MV_CHECK(math::is_commutative_structure<
                   DAlgebraicStructure::field>::value);

    return true;
}

/*
tests_names_old_constants_and_function
  math's constants and function are djinterp's.
  Tests the following:
  - kVariadicArity, kInvalidSize and kInvalidIndex are the same objects
  - has_relation_property is the same function
*/
bool tests_names_old_constants_and_function()
{
    const std::size_t* arity = &math::kVariadicArity;
    const std::size_t* size  = &math::kInvalidSize;
    const std::size_t* index = &math::kInvalidIndex;
    bool (*has)(DRelationProperty, DRelationProperty) =
        &math::has_relation_property;

    D_MV_CHECK(arity == &kVariadicArity);
    D_MV_CHECK(size == &kInvalidSize);
    D_MV_CHECK(index == &kInvalidIndex);
    D_MV_CHECK(has == &has_relation_property);

    return true;
}

/*
tests_names_order_strength_beside_ordering_kind
  math's ordering enum beside container's, in one unit.
  Tests the following:
  - DOrderStrength and DOrderingKind are different types
  - each keeps its own members: DOrderStrength's well_ordered, and
    DOrderingKind's ordered
*/
bool tests_names_order_strength_beside_ordering_kind()
{
    D_MV_CHECK(!std::is_same<DOrderStrength, DOrderingKind>::value);
    D_MV_CHECK(!std::is_same<math::ordering_kind, ordering_kind>::value);

    D_MV_CHECK(static_cast<int>(DOrderStrength::well_ordered) == 5);
    D_MV_CHECK(DOrderingKind::ordered != DOrderingKind::unordered);

    return true;
}

/*
tests_names_multiplicity_stays_in_math
  graph's multiplicity enum, in djinterp::math only.
  Tests the following:
  - math::multiplicity is math::DMultiplicity
  - its four members, unknown at 0
*/
bool tests_names_multiplicity_stays_in_math()
{
    using M = math::DMultiplicity;

    D_MV_CHECK(std::is_same<math::multiplicity, M>::value);
    D_MV_CHECK(static_cast<int>(M::unknown) == 0);
    D_MV_CHECK(static_cast<int>(M::unique) == 1);
    D_MV_CHECK(static_cast<int>(M::multi) == 2);
    D_MV_CHECK(static_cast<int>(M::bounded) == 3);

    return true;
}

NS_END  // testing
NS_END  // djinterp
