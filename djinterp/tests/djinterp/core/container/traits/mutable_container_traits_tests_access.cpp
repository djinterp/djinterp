/*******************************************************************************
* djinterp [core]                      mutable_container_traits_tests_access.cpp
*
*
* path:      /tests/djinterp/core/container/traits/mutable_container_traits_tests_access.cpp
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
bool
tests_access_internal_marks()
{
    bool ok = true;

    // --- the equivalence mark: a nested key_type ---
    D_MC_CHECK(internal::has_equivalence_helper<mc_set_int>::value == true);
    D_MC_CHECK(
        internal::has_equivalence_helper<mc_multiset_int>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<mc_uset_int>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<mc_map_ii>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<mc_multimap_ii>::value == true);
    D_MC_CHECK(internal::has_equivalence_helper<mc_umap_ii>::value == true);

    D_MC_CHECK(internal::has_equivalence_helper<mc_vector_int>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<mc_list_int>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<mc_string>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<mc_array_4>::value == false);
    D_MC_CHECK(
        internal::has_equivalence_helper<mc_vector_pair>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<int>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<void>::value == false);
    D_MC_CHECK(internal::has_equivalence_helper<mc_bare>::value == false);

    // --- the keyed mark: value_type::first_type AND ::second_type ---
    D_MC_CHECK(internal::has_keyed_value_helper<mc_map_ii>::value      == true);
    D_MC_CHECK(
        internal::has_keyed_value_helper<mc_multimap_ii>::value == true);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_umap_ii>::value     == true);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_map_si>::value      == true);

    // any pair-valued type presents it, map or not
    D_MC_CHECK(internal::has_keyed_value_helper<mc_vector_pair>::value == true);

    D_MC_CHECK(internal::has_keyed_value_helper<mc_set_int>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_uset_int>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_vector_int>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_string>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<int>::value == false);
    D_MC_CHECK(internal::has_keyed_value_helper<void>::value == false);

    // a bare pair has first_type and second_type but no value_type
    D_MC_CHECK(internal::has_keyed_value_helper<mc_pair_ii>::value  == false);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_tuple_i>::value  == false);

    // the AND-shape: one half of the pair protocol is not enough
    D_MC_CHECK(internal::has_keyed_value_helper<mc_half_pair_first>::value
                   == false);
    D_MC_CHECK(internal::has_keyed_value_helper<mc_half_pair_second>::value
                   == false);

    // --- the two marks are independent ---
    D_MC_CHECK(   ( internal::has_equivalence_helper<mc_set_int>::value
                        == true )
               && ( internal::has_keyed_value_helper<mc_set_int>::value
                        == false ));
    D_MC_CHECK(   ( internal::has_equivalence_helper<mc_pair_value_only>::value
                        == false )
               && ( internal::has_keyed_value_helper<mc_pair_value_only>::value
                        == true ));
    D_MC_CHECK(   ( internal::has_equivalence_helper<mc_key_and_pair>::value
                        == true )
               && ( internal::has_keyed_value_helper<mc_key_and_pair>::value
                        == true ));
    D_MC_CHECK(   ( internal::has_equivalence_helper<mc_vector_int>::value
                        == false )
               && ( internal::has_keyed_value_helper<mc_vector_int>::value
                        == false ));

    // --- the clean_t contract ---
    D_MC_CV_REF_AGREE(internal::has_equivalence_helper, mc_set_int,    true);
    D_MC_CV_REF_AGREE(internal::has_equivalence_helper, mc_vector_int, false);
    D_MC_CV_REF_AGREE(internal::has_keyed_value_helper, mc_map_ii,     true);
    D_MC_CV_REF_AGREE(internal::has_keyed_value_helper, mc_set_int,    false);

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
bool
tests_access_restriction_enum()
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
    legal as the initialiser of mutable_container_class::access_name
  - the trailing conditional is the catch-all, reached here by key_const
*/
bool
tests_access_restriction_name()
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
bool
tests_access_restriction_of()
{
    bool ok = true;

    // keyed disciplines freeze the key
    D_MC_CHECK(access_restriction_of<mc_map_ii>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<mc_multimap_ii>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<mc_umap_ii>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<mc_map_si>::value
                   == access_restriction::key_const);

    // equivalence disciplines freeze the element
    D_MC_CHECK(access_restriction_of<mc_set_int>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<mc_multiset_int>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<mc_uset_int>::value
                   == access_restriction::element_const);

    // sequences freeze nothing
    D_MC_CHECK(access_restriction_of<mc_vector_int>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_deque_int>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_list_int>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_flist_int>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_string>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_array_4>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_init_list>::value
                   == access_restriction::none);

    // and neither do non-containers
    D_MC_CHECK(access_restriction_of<int>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<void>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_bare>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_int_ptr>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_tuple_i>::value
                   == access_restriction::none);

    // a bare pair carries first_type and second_type but no value_type, so the
    // keyed probe - which looks INSIDE value_type - does not fire
    D_MC_CHECK(access_restriction_of<mc_pair_ii>::value
                   == access_restriction::none);

    // the marks in isolation
    D_MC_CHECK(access_restriction_of<mc_key_type_only>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<mc_pair_value_only>::value
                   == access_restriction::key_const);
    D_MC_CHECK(access_restriction_of<mc_plain_value_only>::value
                   == access_restriction::none);

    // precedence: keyed wins when both marks are present
    D_MC_CHECK(access_restriction_of<mc_key_and_pair>::value
                   == access_restriction::key_const);

    // the keyed probe needs BOTH halves of the pair protocol
    D_MC_CHECK(access_restriction_of<mc_half_pair_first>::value
                   == access_restriction::none);
    D_MC_CHECK(access_restriction_of<mc_half_pair_second>::value
                   == access_restriction::none);

    // structural, not intentional: a sequence of pairs presents the keyed mark
    D_MC_CHECK(access_restriction_of<mc_vector_pair>::value
                   == access_restriction::key_const);

    // the discipline-shaped probes
    D_MC_CHECK(access_restriction_of<mc_set_shaped>::value
                   == access_restriction::element_const);
    D_MC_CHECK(access_restriction_of<mc_map_shaped>::value
                   == access_restriction::key_const);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(access_restriction_of<mc_string_view>::value
                   == access_restriction::none);
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_MC_CHECK(access_restriction_of<mc_span_int>::value
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
bool
tests_access_restriction_of_companions()
{
    bool ok = true;

    // ::type is the integral_constant carrying the restriction
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of<mc_map_ii>::type,
                     std::integral_constant<
                         access_restriction,
                         access_restriction::key_const>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of<mc_set_int>::type,
                     std::integral_constant<
                         access_restriction,
                         access_restriction::element_const>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of<mc_vector_int>::type,
                     std::integral_constant<
                         access_restriction,
                         access_restriction::none>>::value);

    // the _t alias names the same type
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of_t<mc_map_ii>,
                     access_restriction_of<mc_map_ii>::type>::value);
    D_MC_STATIC_CHECK(
        std::is_same<access_restriction_of_t<mc_set_int>,
                     access_restriction_of<mc_set_int>::type>::value);

    // and carries the same value
    D_MC_CHECK(access_restriction_of_t<mc_map_ii>::value
                   == access_restriction_of<mc_map_ii>::value);
    D_MC_CHECK(access_restriction_of_t<mc_set_int>::value
                   == access_restriction_of<mc_set_int>::value);
    D_MC_CHECK(access_restriction_of_t<mc_vector_int>::value
                   == access_restriction_of<mc_vector_int>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(access_restriction_of_v<mc_map_ii>
                          == access_restriction::key_const);

    D_MC_CHECK(access_restriction_of_v<mc_map_ii>
                   == access_restriction_of<mc_map_ii>::value);
    D_MC_CHECK(access_restriction_of_v<mc_set_int>
                   == access_restriction_of<mc_set_int>::value);
    D_MC_CHECK(access_restriction_of_v<mc_vector_int>
                   == access_restriction_of<mc_vector_int>::value);
    D_MC_CHECK(access_restriction_of_v<void>
                   == access_restriction_of<void>::value);

    // the companions of the two predicates
    D_MC_STATIC_CHECK(constrains_key_v<mc_map_ii>      == true);
    D_MC_STATIC_CHECK(constrains_element_v<mc_set_int> == true);

    D_MC_CHECK(constrains_element_v<mc_set_int>
                   == djinterp::constrains_element<mc_set_int>::value);
    D_MC_CHECK(constrains_element_v<mc_map_ii>
                   == djinterp::constrains_element<mc_map_ii>::value);
    D_MC_CHECK(constrains_key_v<mc_map_ii>
                   == djinterp::constrains_key<mc_map_ii>::value);
    D_MC_CHECK(constrains_key_v<mc_vector_int>
                   == djinterp::constrains_key<mc_vector_int>::value);

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
bool
tests_constrains_element()
{
    bool ok = true;

    // equivalence disciplines
    D_MC_CHECK(constrains_element<mc_set_int>::value      == true);
    D_MC_CHECK(constrains_element<mc_multiset_int>::value == true);
    D_MC_CHECK(constrains_element<mc_uset_int>::value     == true);
    D_MC_CHECK(constrains_element<mc_set_shaped>::value   == true);

    // keyed disciplines freeze the key, not the element
    D_MC_CHECK(constrains_element<mc_map_ii>::value      == false);
    D_MC_CHECK(constrains_element<mc_multimap_ii>::value == false);
    D_MC_CHECK(constrains_element<mc_umap_ii>::value     == false);
    D_MC_CHECK(constrains_element<mc_map_shaped>::value  == false);

    // sequences and non-containers
    D_MC_CHECK(constrains_element<mc_vector_int>::value == false);
    D_MC_CHECK(constrains_element<mc_list_int>::value   == false);
    D_MC_CHECK(constrains_element<mc_array_4>::value    == false);
    D_MC_CHECK(constrains_element<int>::value           == false);
    D_MC_CHECK(constrains_element<void>::value          == false);
    D_MC_CHECK(constrains_element<mc_bare>::value       == false);

    // the mark alone is enough, at any grade
    D_MC_CHECK(constrains_element<mc_key_type_only>::value == true);
    D_MC_CHECK(constrains_element<mc_element_and_element_const>::value
                   == true);
    D_MC_CHECK(constrains_element<mc_full_and_element_const>::value == true);

    // agreement with the definition
    D_MC_CHECK(constrains_element<mc_set_int>::value
                   == ( access_restriction_of<mc_set_int>::value
                            == access_restriction::element_const ));
    D_MC_CHECK(constrains_element<mc_map_ii>::value
                   == ( access_restriction_of<mc_map_ii>::value
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
bool
tests_constrains_key()
{
    bool ok = true;

    // keyed disciplines
    D_MC_CHECK(constrains_key<mc_map_ii>::value      == true);
    D_MC_CHECK(constrains_key<mc_multimap_ii>::value == true);
    D_MC_CHECK(constrains_key<mc_umap_ii>::value     == true);
    D_MC_CHECK(constrains_key<mc_map_si>::value      == true);
    D_MC_CHECK(constrains_key<mc_map_shaped>::value  == true);

    // equivalence disciplines
    D_MC_CHECK(constrains_key<mc_set_int>::value      == false);
    D_MC_CHECK(constrains_key<mc_multiset_int>::value == false);
    D_MC_CHECK(constrains_key<mc_uset_int>::value     == false);

    // sequences and non-containers
    D_MC_CHECK(constrains_key<mc_vector_int>::value == false);
    D_MC_CHECK(constrains_key<mc_array_4>::value    == false);
    D_MC_CHECK(constrains_key<int>::value           == false);
    D_MC_CHECK(constrains_key<void>::value          == false);
    D_MC_CHECK(constrains_key<mc_bare>::value       == false);

    // the mark alone is enough, at any grade
    D_MC_CHECK(constrains_key<mc_pair_value_only>::value       == true);
    D_MC_CHECK(constrains_key<mc_key_and_pair>::value          == true);
    D_MC_CHECK(constrains_key<mc_element_and_key_const>::value == true);

    // agreement with the definition
    D_MC_CHECK(constrains_key<mc_map_ii>::value
                   == ( access_restriction_of<mc_map_ii>::value
                            == access_restriction::key_const ));
    D_MC_CHECK(constrains_key<mc_set_int>::value
                   == ( access_restriction_of<mc_set_int>::value
                            == access_restriction::key_const ));

    // mutually exclusive, and jointly exhaustive of the non-none case
    D_MC_CHECK(!(   constrains_element<mc_map_ii>::value
                 && constrains_key<mc_map_ii>::value ));
    D_MC_CHECK(!(   constrains_element<mc_set_int>::value
                 && constrains_key<mc_set_int>::value ));
    D_MC_CHECK(!(   constrains_element<mc_key_and_pair>::value
                 && constrains_key<mc_key_and_pair>::value ));
    D_MC_CHECK(   (    !constrains_element<mc_vector_int>::value
                    && !constrains_key<mc_vector_int>::value )
               == ( access_restriction_of<mc_vector_int>::value
                        == access_restriction::none ));
    D_MC_CHECK(   (    !constrains_element<mc_map_ii>::value
                    && !constrains_key<mc_map_ii>::value )
               == ( access_restriction_of<mc_map_ii>::value
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
bool
tests_access_restriction_cv_ref_invariance()
{
    bool ok = true;

    D_MC_CV_REF_VALUE(access_restriction_of, mc_map_ii,
                      access_restriction::key_const);
    D_MC_CV_REF_VALUE(access_restriction_of, mc_set_int,
                      access_restriction::element_const);
    D_MC_CV_REF_VALUE(access_restriction_of, mc_vector_int,
                      access_restriction::none);
    D_MC_CV_REF_VALUE(access_restriction_of, mc_bare,
                      access_restriction::none);

    D_MC_CV_REF_AGREE(constrains_element, mc_set_int,    true);
    D_MC_CV_REF_AGREE(constrains_element, mc_map_ii,     false);
    D_MC_CV_REF_AGREE(constrains_element, mc_vector_int, false);

    D_MC_CV_REF_AGREE(constrains_key, mc_map_ii,     true);
    D_MC_CV_REF_AGREE(constrains_key, mc_set_int,    false);
    D_MC_CV_REF_AGREE(constrains_key, mc_vector_int, false);

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
bool
tests_access_grade_orthogonality()
{
    bool ok = true;

    // --- immutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<mc_sized_only>::value
                        == mutability::immutable )
               && ( access_restriction_of<mc_sized_only>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<mc_key_type_only>::value
                        == mutability::immutable )
               && ( access_restriction_of<mc_key_type_only>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<mc_pair_value_only>::value
                        == mutability::immutable )
               && ( access_restriction_of<mc_pair_value_only>::value
                        == access_restriction::key_const ));

    // --- element_mutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<mc_array_4>::value
                        == mutability::element_mutable )
               && ( access_restriction_of<mc_array_4>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<mc_element_and_element_const>::value
                        == mutability::element_mutable )
               && ( access_restriction_of<mc_element_and_element_const>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<mc_element_and_key_const>::value
                        == mutability::element_mutable )
               && ( access_restriction_of<mc_element_and_key_const>::value
                        == access_restriction::key_const ));

    // --- structural_mutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<mc_list_int>::value
                        == mutability::structural_mutable )
               && ( access_restriction_of<mc_list_int>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<mc_set_int>::value
                        == mutability::structural_mutable )
               && ( access_restriction_of<mc_set_int>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<mc_map_ii>::value
                        == mutability::structural_mutable )
               && ( access_restriction_of<mc_map_ii>::value
                        == access_restriction::key_const ));

    // --- fully_mutable grade, all three restrictions ---
    D_MC_CHECK(   ( mutability_of<mc_vector_int>::value
                        == mutability::fully_mutable )
               && ( access_restriction_of<mc_vector_int>::value
                        == access_restriction::none ));
    D_MC_CHECK(   ( mutability_of<mc_full_and_element_const>::value
                        == mutability::fully_mutable )
               && ( access_restriction_of<mc_full_and_element_const>::value
                        == access_restriction::element_const ));
    D_MC_CHECK(   ( mutability_of<mc_vector_pair>::value
                        == mutability::fully_mutable )
               && ( access_restriction_of<mc_vector_pair>::value
                        == access_restriction::key_const ));

    // the access dimension is not a function of the grade: one grade spans
    // three restrictions, and one restriction spans four grades
    D_MC_CHECK(access_restriction_of<mc_key_type_only>::value
                   != access_restriction_of<mc_sized_only>::value);
    D_MC_CHECK(mutability_of<mc_key_type_only>::value
                   != mutability_of<mc_set_int>::value);
    D_MC_CHECK(   ( constrains_element<mc_key_type_only>::value == true )
               && ( constrains_element<mc_full_and_element_const>::value
                        == true )
               && ( mutability_of<mc_key_type_only>::value
                        != mutability_of<mc_full_and_element_const>::value ));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
