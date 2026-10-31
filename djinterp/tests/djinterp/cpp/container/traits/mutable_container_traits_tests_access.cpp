/*******************************************************************************
* djinterp [cpp]                       mutable_container_traits_tests_access.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/mutable_container_traits_tests_access.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "mutable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// IV.  The access dimension  (the 2-D model)
// ===========================================================================
//   The grade answers WHICH KINDS of change a container admits; this dimension
// answers, for the element-mutation capability, WHICH COORDINATE the discipline
// leaves settable.  The two are read by separate traits from separate evidence,
// and the last test here proves that separation by exhibiting a witness for
// every one of the twelve grade x access combinations.


/*
tests_access_internal_marks
  Verifies the two internal detectors the dimension is read from, directly.
  access_restriction_of only publishes their combination, so a fault in either
  one shows up there as a single wrong answer; testing them separately says
  WHICH mark misfired.  Both live in djinterp::internal by design - they are
  the local signals that keep this module free of the overlay include graph.
  Tests the following:
  - has_equivalence_helper fires on a nested key_type: every associative
    discipline, ordered and unordered, and nothing else
  - has_keyed_value_helper fires on a value_type carrying BOTH first_type and
    second_type: the map family, and any other type whose element is a pair
  - the two are independent: a set has the equivalence mark without the keyed
    one, a bare pair-valued type has the keyed mark without the equivalence
    one, and a map carries both - which is why access_restriction_of must
    order them rather than treat them as exclusive
  - a bare std::pair fires neither: the keyed probe looks INSIDE value_type,
    and a pair has no value_type of its own
  - both marks respect the clean_t contract
*/
bool tests_access_internal_marks()
{
    bool ok = true;

    // --- the equivalence mark: a nested key_type ---
    D_MC_CHECK(internal::has_equivalence_helper<DMcSetInt>::value == true);
    D_MC_CHECK(
        internal::has_equivalence_helper<DMcMultisetInt>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<DMcUsetInt>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<DMcMapIi>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<DMcMultimapIi>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<DMcUmapIi>::value == true);

    D_MC_CHECK(internal::has_equivalence_helper<DMcVectorInt>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<DMcListInt>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<DMcString>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<DMcArray4>::value == false);
    D_MC_CHECK(
        internal::has_equivalence_helper<DMcVectorPair>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<int>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<void>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<DMcBare>::value == false);

    // --- the keyed mark: value_type::first_type AND ::second_type ---
    D_MC_CHECK(internal::has_keyed_value_helper<DMcMapIi>::value      == true);
    D_MC_CHECK(
        internal::has_keyed_value_helper<DMcMultimapIi>::value == true);
    D_MC_CHECK(internal::has_keyed_value_helper<DMcUmapIi>::value     == true);
    D_MC_CHECK(internal::has_keyed_value_helper<DMcMapSi>::value      == true);

    // any pair-valued type presents it, map or not
    D_MC_CHECK(internal::has_keyed_value_helper<DMcVectorPair>::value == true);

    D_MC_CHECK(internal::has_keyed_value_helper<DMcSetInt>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<DMcUsetInt>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<DMcVectorInt>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<DMcString>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<int>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<void>::value == false);

    // a bare pair has first_type and second_type but no value_type
    D_MC_CHECK(internal::has_keyed_value_helper<DMcPairIi>::value  == false);
    D_MC_CHECK(internal::has_keyed_value_helper<DMcTupleI>::value  == false);

    // the AND-shape: one half of the pair protocol is not enough
    D_MC_CHECK(internal::has_keyed_value_helper<mc_half_pair_first>::value
                   == false);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_half_pair_second>::value
                   == false);

    // --- the two marks are independent ---
    D_MC_CHECK(   ( internal::has_equivalence_helper<DMcSetInt>::value
                        == true )
               && ( internal::has_keyed_value_helper<DMcSetInt>::value
                        == false ));
    D_MC_CHECK(   ( internal::has_equivalence_helper<mc_pair_value_only>::value
                        == false )
               && ( internal::has_keyed_value_helper<mc_pair_value_only>::value
                        == true ));
    D_MC_CHECK(   ( internal::has_equivalence_helper<DMcKeyAndPair>::value
                        == true )
               && ( internal::has_keyed_value_helper<DMcKeyAndPair>::value
                        == true ));
    D_MC_CHECK(   ( internal::has_equivalence_helper<DMcVectorInt>::value
                        == false )
               && ( internal::has_keyed_value_helper<DMcVectorInt>::value
                        == false ));

    // --- the clean_t contract ---
    D_MC_CV_REF_AGREE(internal::has_equivalence_helper, DMcSetInt,    true);
    D_MC_CV_REF_AGREE(internal::has_equivalence_helper, DMcVectorInt, false);
    D_MC_CV_REF_AGREE(internal::has_keyed_value_helper, DMcMapIi,     true);
    D_MC_CV_REF_AGREE(internal::has_keyed_value_helper, DMcSetInt,    false);

    return ok;
}

/*
tests_access_restriction_enum
  Verifies the access enumeration itself.
  Tests the following:
  - it is an enumeration, and a SCOPED one
  - the three enumerators are pairwise distinct
  - a restriction is usable as a non-type template argument, which is what lets
    access_restriction_of publish an integral_constant ::type
*/
bool tests_access_restriction_enum()
{
    bool ok = true;

    // an enumeration, and scoped
    D_MC_STATIC_CHECK(std::is_enum<access_restriction>::value == true);
    D_MC_STATIC_CHECK(
        std::is_convertible<access_restriction, int>::value == false);

    // pairwise distinct
    D_MC_CHECK(access_restriction::none != access_restriction::element_const);
    D_MC_CHECK(access_restriction::none != access_restriction::key_const);
    D_MC_CHECK(access_restriction::element_const
                   != access_restriction::key_const);

    // usable as a non-type template argument
    D_MC_STATIC_CHECK(
        std::integral_constant<access_restriction,
                               access_restriction::key_const>::value
            == access_restriction::key_const);

    return ok;
}

/*
tests_access_restriction_name
  Verifies the stable spelling of each restriction.
  Tests the following:
  - all three restrictions render their own name, exactly
  - the names are pairwise distinct
  - the function is usable in a constant expression, which is what makes it
    legal as the initialiser of DMutableContainerClass::access_name
  - the trailing conditional is the catch-all, reached here by key_const
*/
bool tests_access_restriction_name()
{
    bool ok = true;

    // constant-expression usability, and first-character distinctness
    D_MC_STATIC_CHECK(
        access_restriction_name(access_restriction::none)[0] == 'n');
    D_MC_STATIC_CHECK(
        access_restriction_name(access_restriction::element_const)[0] == 'e');
    D_MC_STATIC_CHECK(
        access_restriction_name(access_restriction::key_const)[0] == 'k');

    // exact spellings
    D_MC_CHECK(std::strcmp(access_restriction_name(access_restriction::none),
                           "none") == 0);
    D_MC_CHECK(std::strcmp(
                   access_restriction_name(access_restriction::element_const),
                   "element_const") == 0);
    D_MC_CHECK(std::strcmp(
                   access_restriction_name(access_restriction::key_const),
                   "key_const") == 0);

    // pairwise distinct spellings
    D_MC_CHECK(std::strcmp(
                   access_restriction_name(access_restriction::none),
                   access_restriction_name(access_restriction::element_const))
                   != 0);
    D_MC_CHECK(std::strcmp(
                   access_restriction_name(access_restriction::element_const),
                   access_restriction_name(access_restriction::key_const))
                   != 0);

    return ok;
}

/*
tests_access_restriction_of
  Verifies the coordinate a type's discipline holds const.
  Tests the following:
  - a map or multimap freezes the KEY: its value_type is a pair
  - a set or multiset freezes the ELEMENT: it carries a key_type but its
    value_type is not a pair
  - the unordered forms answer as their ordered counterparts do
  - a plain sequence freezes nothing, and neither does a non-container
  - the keyed mark takes PRECEDENCE over the equivalence mark, which is what
    keeps a map - which carries both - out of the set answer
  - the keyed mark requires BOTH halves of the pair protocol: a value_type with
    only first_type, or only second_type, is not keyed
  - the marks are read structurally, not by intent: a vector of pairs answers
    key_const, since the detector sees the same evidence a map presents
*/
bool tests_access_restriction_of()
{
    bool ok = true;

    // keyed disciplines freeze the key
    D_MC_CHECK(access_restriction_of<DMcMapIi>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<DMcMultimapIi>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<DMcUmapIi>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<DMcMapSi>::value
                   == access_restriction::key_const);

    // equivalence disciplines freeze the element
    D_MC_CHECK(access_restriction_of<DMcSetInt>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<DMcMultisetInt>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<DMcUsetInt>::value
                   == access_restriction::element_const);

    // sequences freeze nothing
    D_MC_CHECK(access_restriction_of<DMcVectorInt>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcDequeInt>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcListInt>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcFlistInt>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcString>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcArray4>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcInitList>::value
                   == access_restriction::none);

    // and neither do non-containers
    D_MC_CHECK(access_restriction_of<int>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<void>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcBare>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcIntPtr>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<DMcTupleI>::value
                   == access_restriction::none);

    // a bare pair carries first_type and second_type but no value_type, so the
    // keyed probe - which looks INSIDE value_type - does not fire
    D_MC_CHECK(access_restriction_of<DMcPairIi>::value
                   == access_restriction::none);

    // the marks in isolation
    D_MC_CHECK(access_restriction_of<DMcKeyTypeOnly>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<mc_pair_value_only>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<mc_plain_value_only>::value
                   == access_restriction::none);

    // precedence: keyed wins when both marks are present
    D_MC_CHECK(access_restriction_of<DMcKeyAndPair>::value
                   == access_restriction::key_const);

    // the keyed probe needs BOTH halves of the pair protocol
    D_MC_CHECK(access_restriction_of<mc_half_pair_first>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_half_pair_second>::value
                   == access_restriction::none);

    // structural, not intentional: a sequence of pairs presents the keyed mark
    D_MC_CHECK(access_restriction_of<DMcVectorPair>::value
                   == access_restriction::key_const);

    // the discipline-shaped probes
    D_MC_CHECK(access_restriction_of<DMcSetShaped>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<DMcMapShaped>::value
                   == access_restriction::key_const);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(access_restriction_of<DMcStringView>::value
                   == access_restriction::none);
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_MC_CHECK(access_restriction_of<DMcSpanInt>::value
                   == access_restriction::none);
#endif

    return ok;
}

/*
tests_access_restriction_of_companions
  Verifies the ::type member and the `_v` companion of access_restriction_of.
  Tests the following:
  - ::type is exactly integral_constant<access_restriction, value>, for each of
    the three restrictions
  - access_restriction_of_t names that same type and carries the same value
  - the `_v` companion agrees with ::value and is usable in a constant
    expression, gated on the language's variable-template support
  - the `_v` companions of constrains_element and constrains_key agree with
    their traits likewise
*/
bool tests_access_restriction_of_companions()
{
    bool ok = true;

    // ::type is the integral_constant carrying the restriction
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of<DMcMapIi>::type,
                     std::integral_constant<
                         access_restriction,
                         access_restriction::key_const>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of<DMcSetInt>::type,
                     std::integral_constant<
                         access_restriction,
                         access_restriction::element_const>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of<DMcVectorInt>::type,
                     std::integral_constant<
                         access_restriction,
                         access_restriction::none>>::value);

    // the _t alias names the same type
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of_t<DMcMapIi>,
                     access_restriction_of<DMcMapIi>::type>::value);
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of_t<DMcSetInt>,
                     access_restriction_of<DMcSetInt>::type>::value);

    // and carries the same value
    D_MC_CHECK(access_restriction_of_t<DMcMapIi>::value
                   == access_restriction_of<DMcMapIi>::value);
    D_MC_CHECK(access_restriction_of_t<DMcSetInt>::value
                   == access_restriction_of<DMcSetInt>::value);
    D_MC_CHECK(access_restriction_of_t<DMcVectorInt>::value
                   == access_restriction_of<DMcVectorInt>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(access_restriction_of_v<DMcMapIi>
                          == access_restriction::key_const);

    D_MC_CHECK(access_restriction_of_v<DMcMapIi>
                   == access_restriction_of<DMcMapIi>::value);
    D_MC_CHECK(access_restriction_of_v<DMcSetInt>
                   == access_restriction_of<DMcSetInt>::value);
    D_MC_CHECK(access_restriction_of_v<DMcVectorInt>
                   == access_restriction_of<DMcVectorInt>::value);
    D_MC_CHECK(access_restriction_of_v<void>
                   == access_restriction_of<void>::value);

    // the companions of the two predicates
    D_MC_STATIC_CHECK(constrains_key_v<DMcMapIi>      == true);
    D_MC_STATIC_CHECK(constrains_element_v<DMcSetInt> == true);

    D_MC_CHECK(constrains_element_v<DMcSetInt>
                   == djinterp::constrains_element<DMcSetInt>::value);
    D_MC_CHECK(constrains_element_v<DMcMapIi>
                   == djinterp::constrains_element<DMcMapIi>::value);
    D_MC_CHECK(constrains_key_v<DMcMapIi>
                   == djinterp::constrains_key<DMcMapIi>::value);
    D_MC_CHECK(constrains_key_v<DMcVectorInt>
                   == djinterp::constrains_key<DMcVectorInt>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}

/*
tests_constrains_element
  Verifies the predicate for "the discipline holds the ELEMENT const".
  Tests the following:
  - the equivalence disciplines qualify: set, multiset, unordered_set
  - the keyed disciplines do NOT, since they freeze the key instead
  - sequences and non-containers do not
  - the predicate equals its definition: the restriction IS element_const
  - it holds for a type carrying the equivalence mark alone, regardless of
    grade, which is what makes the dimension independent
*/
bool tests_constrains_element()
{
    bool ok = true;

    // equivalence disciplines
    D_MC_CHECK(constrains_element<DMcSetInt>::value      == true);
    D_MC_CHECK(constrains_element<DMcMultisetInt>::value == true);
    D_MC_CHECK(constrains_element<DMcUsetInt>::value     == true);
    D_MC_CHECK(constrains_element<DMcSetShaped>::value   == true);

    // keyed disciplines freeze the key, not the element
    D_MC_CHECK(constrains_element<DMcMapIi>::value      == false);
    D_MC_CHECK(constrains_element<DMcMultimapIi>::value == false);
    D_MC_CHECK(constrains_element<DMcUmapIi>::value     == false);
    D_MC_CHECK(constrains_element<DMcMapShaped>::value  == false);

    // sequences and non-containers
    D_MC_CHECK(constrains_element<DMcVectorInt>::value == false);
    D_MC_CHECK(constrains_element<DMcListInt>::value   == false);
    D_MC_CHECK(constrains_element<DMcArray4>::value    == false);
    D_MC_CHECK(constrains_element<int>::value           == false);
    D_MC_CHECK(constrains_element<void>::value          == false);
    D_MC_CHECK(constrains_element<DMcBare>::value       == false);

    // the mark alone is enough, at any grade
    D_MC_CHECK(constrains_element<DMcKeyTypeOnly>::value == true);
    D_MC_CHECK(constrains_element<DMcElementAndElementConst>::value
                   == true);
    D_MC_CHECK(constrains_element<DMcFullAndElementConst>::value == true);

    // agreement with the definition
    D_MC_CHECK(constrains_element<DMcSetInt>::value
                   == ( access_restriction_of<DMcSetInt>::value
                            == access_restriction::element_const ));
    D_MC_CHECK(constrains_element<DMcMapIi>::value
                   == ( access_restriction_of<DMcMapIi>::value
                            == access_restriction::element_const ));

    return ok;
}

/*
tests_constrains_key
  Verifies the predicate for "the discipline holds the KEY const while freeing
  the value".
  Tests the following:
  - the keyed disciplines qualify: map, multimap, unordered_map
  - the equivalence disciplines do NOT
  - sequences and non-containers do not
  - the predicate equals its definition
  - the two predicates are MUTUALLY EXCLUSIVE, and both false exactly when the
    restriction is none - so together they partition the dimension
*/
bool tests_constrains_key()
{
    bool ok = true;

    // keyed disciplines
    D_MC_CHECK(constrains_key<DMcMapIi>::value      == true);
    D_MC_CHECK(constrains_key<DMcMultimapIi>::value == true);
    D_MC_CHECK(constrains_key<DMcUmapIi>::value     == true);
    D_MC_CHECK(constrains_key<DMcMapSi>::value      == true);
    D_MC_CHECK(constrains_key<DMcMapShaped>::value  == true);

    // equivalence disciplines
    D_MC_CHECK(constrains_key<DMcSetInt>::value      == false);
    D_MC_CHECK(constrains_key<DMcMultisetInt>::value == false);
    D_MC_CHECK(constrains_key<DMcUsetInt>::value     == false);

    // sequences and non-containers
    D_MC_CHECK(constrains_key<DMcVectorInt>::value == false);
    D_MC_CHECK(constrains_key<DMcArray4>::value    == false);
    D_MC_CHECK(constrains_key<int>::value           == false);
    D_MC_CHECK(constrains_key<void>::value          == false);
    D_MC_CHECK(constrains_key<DMcBare>::value       == false);

    // the mark alone is enough, at any grade
    D_MC_CHECK(constrains_key<mc_pair_value_only>::value       == true);
    D_MC_CHECK(constrains_key<DMcKeyAndPair>::value          == true);
    D_MC_CHECK(constrains_key<DMcElementAndKeyConst>::value == true);

    // agreement with the definition
    D_MC_CHECK(constrains_key<DMcMapIi>::value
                   == ( access_restriction_of<DMcMapIi>::value
                            == access_restriction::key_const ));
    D_MC_CHECK(constrains_key<DMcSetInt>::value
                   == ( access_restriction_of<DMcSetInt>::value
                            == access_restriction::key_const ));

    // mutually exclusive, and jointly exhaustive of the non-none case
    D_MC_CHECK(!(   constrains_element<DMcMapIi>::value
                 && constrains_key<DMcMapIi>::value ));
    D_MC_CHECK(!(   constrains_element<DMcSetInt>::value
                 && constrains_key<DMcSetInt>::value ));
    D_MC_CHECK(!(   constrains_element<DMcKeyAndPair>::value
                 && constrains_key<DMcKeyAndPair>::value ));
    D_MC_CHECK(   (    !constrains_element<DMcVectorInt>::value
                    && !constrains_key<DMcVectorInt>::value )
               == ( access_restriction_of<DMcVectorInt>::value
                        == access_restriction::none ));
    D_MC_CHECK(   (    !constrains_element<DMcMapIi>::value
                    && !constrains_key<DMcMapIi>::value )
               == ( access_restriction_of<DMcMapIi>::value
                        == access_restriction::none ));

    return ok;
}

/*
tests_access_restriction_cv_ref_invariance
  Verifies that the access dimension inherits the clean_t contract.
  Tests the following:
  - all three restrictions are stable across every cv- and reference-qualified
    spelling of their carrier
  - the two predicates are stable likewise
*/
bool tests_access_restriction_cv_ref_invariance()
{
    bool ok = true;

    D_MC_CV_REF_VALUE(access_restriction_of, DMcMapIi,
                      access_restriction::key_const);
    D_MC_CV_REF_VALUE(access_restriction_of, DMcSetInt,
                      access_restriction::element_const);
    D_MC_CV_REF_VALUE(access_restriction_of, DMcVectorInt,
                      access_restriction::none);
    D_MC_CV_REF_VALUE(access_restriction_of, DMcBare,
                      access_restriction::none);

    D_MC_CV_REF_AGREE(constrains_element, DMcSetInt,    true);
    D_MC_CV_REF_AGREE(constrains_element, DMcMapIi,     false);
    D_MC_CV_REF_AGREE(constrains_element, DMcVectorInt, false);

    D_MC_CV_REF_AGREE(constrains_key, DMcMapIi,     true);
    D_MC_CV_REF_AGREE(constrains_key, DMcSetInt,    false);
    D_MC_CV_REF_AGREE(constrains_key, DMcVectorInt, false);

    return ok;
}

/*
tests_access_grade_orthogonality
  Verifies that the two dimensions of the model are genuinely independent, by
  exhibiting a witness for EVERY cell of the four-by-three grid.  Neither
  dimension may be inferred from the other, so a change that entangled them -
  say, deriving element_const from the absence of element mutation - would
  leave some cell unoccupied and fail here.
  Tests the following:
  - immutable        x {none, element_const, key_const}
  - element_mutable  x {none, element_const, key_const}
  - structural       x {none, element_const, key_const}
  - fully_mutable    x {none, element_const, key_const}
  - and, for the record, that the two std disciplines land where the header
    says they do: set is (structural, element_const), map is (structural,
    key_const)
*/
bool tests_access_grade_orthogonality()
{
    bool ok = true;

    // --- immutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<DMcSizedOnly>::value
                        == mutability::immutable )
               && ( access_restriction_of<DMcSizedOnly>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<DMcKeyTypeOnly>::value
                        == mutability::immutable )
               && ( access_restriction_of<DMcKeyTypeOnly>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<mc_pair_value_only>::value
                        == mutability::immutable )
               && ( access_restriction_of<mc_pair_value_only>::value
                        == access_restriction::key_const ));

    // --- element_mutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<DMcArray4>::value
                        == mutability::element_mutable )
               && ( access_restriction_of<DMcArray4>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<DMcElementAndElementConst>::value
                        == mutability::element_mutable )
               && ( access_restriction_of<DMcElementAndElementConst>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<DMcElementAndKeyConst>::value
                        == mutability::element_mutable )
               && ( access_restriction_of<DMcElementAndKeyConst>::value
                        == access_restriction::key_const ));

    // --- structural_mutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<DMcListInt>::value
                        == mutability::structural_mutable )
               && ( access_restriction_of<DMcListInt>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<DMcSetInt>::value
                        == mutability::structural_mutable )
               && ( access_restriction_of<DMcSetInt>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<DMcMapIi>::value
                        == mutability::structural_mutable )
               && ( access_restriction_of<DMcMapIi>::value
                        == access_restriction::key_const ));

    // --- fully_mutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<DMcVectorInt>::value
                        == mutability::fully_mutable )
               && ( access_restriction_of<DMcVectorInt>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<DMcFullAndElementConst>::value
                        == mutability::fully_mutable )
               && ( access_restriction_of<DMcFullAndElementConst>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<DMcVectorPair>::value
                        == mutability::fully_mutable )
               && ( access_restriction_of<DMcVectorPair>::value
                        == access_restriction::key_const ));

    // the access dimension is not a function of the grade: one grade spans
    // three restrictions, and one restriction spans four grades
    D_MC_CHECK(access_restriction_of<DMcKeyTypeOnly>::value
                   != access_restriction_of<DMcSizedOnly>::value);
    D_MC_CHECK(mutability_of<DMcKeyTypeOnly>::value
                   != mutability_of<DMcSetInt>::value);
    D_MC_CHECK(   ( constrains_element<DMcKeyTypeOnly>::value == true )
               && ( constrains_element<DMcFullAndElementConst>::value
                        == true )
               && ( mutability_of<DMcKeyTypeOnly>::value
                        != mutability_of<DMcFullAndElementConst>::value ));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
