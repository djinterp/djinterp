/*******************************************************************************
* djinterp [core]            container_copy_merge_traits_tests_prerequisites.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests_prerequisites.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_copy_merge_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   Operand prerequisites
// ===========================================================================
//   One trait, three conjuncts: both operands must be containers, and their
// elements must combine.  The element clause is itself a three-way
// disjunction - same type, or convertible in either direction - which makes
// the trait symmetric overall even though two of its three terms are not.


/*
tests_merge_elements_compatible
  Verifies the element prerequisite on carriers of each kind.
  Tests the following:
  - identical element types are compatible
  - element types related by a standard conversion are compatible, in either
    argument order
  - unrelated element types are not
  - the trait is symmetric, though its middle and last terms are each
    one-directional - it is the DISJUNCTION of the two directions that makes it
    so, not either term alone
*/
bool
tests_merge_elements_compatible()
{
    bool ok = true;

    // same element type
    D_CM_CHECK((merge_elements_compatible<cm_vector, cm_vector>::value)
                   == true);
    D_CM_CHECK((merge_elements_compatible<cm_seq, cm_seq>::value) == true);

    // a standard conversion, either way round
    D_CM_CHECK((merge_elements_compatible<cm_vector,
                                          cm_vec_double>::value) == true);
    D_CM_CHECK((merge_elements_compatible<cm_vec_double,
                                          cm_vector>::value) == true);

    // a user-defined one-way conversion, either way round
    D_CM_CHECK((merge_elements_compatible<cm_from, cm_to>::value) == true);
    D_CM_CHECK((merge_elements_compatible<cm_to, cm_from>::value) == true);

    // unrelated elements
    D_CM_CHECK((merge_elements_compatible<cm_vector,
                                          cm_vec_string>::value) == false);
    D_CM_CHECK((merge_elements_compatible<cm_from,
                                          cm_unrelated_elem>::value)
                   == false);

    // symmetric overall
    D_CM_CHECK((merge_elements_compatible<cm_from, cm_to>::value)
                   == (merge_elements_compatible<cm_to, cm_from>::value));
    D_CM_CHECK((merge_elements_compatible<cm_vector, cm_vec_string>::value)
                   == (merge_elements_compatible<cm_vec_string,
                                                 cm_vector>::value));

    // ... while the underlying conversion is NOT
    D_CM_CHECK((elements_convertible<cm_from, cm_to>::value) == true);
    D_CM_CHECK((elements_convertible<cm_to, cm_from>::value) == false);

    return ok;
}

/*
tests_merge_elements_needs_containers
  Verifies the two container conjuncts, which precede the element clause.
  Tests the following:
  - a non-container on either side defeats the trait, even where the element
    clause would otherwise hold
  - a non-container on BOTH sides likewise
  - the guard is is_iterable_container specifically, so a type carrying
    value_type and every associative mark but no begin()/end() still fails
  - each conjunct is separately necessary, shown by holding one side fixed
*/
bool
tests_merge_elements_needs_containers()
{
    bool ok = true;

    // a non-container on the right
    D_CM_CHECK((merge_elements_compatible<cm_vector, int>::value) == false);
    D_CM_CHECK((merge_elements_compatible<cm_seq,
                                          cm_not_container>::value) == false);

    // ... on the left
    D_CM_CHECK((merge_elements_compatible<int, cm_vector>::value) == false);
    D_CM_CHECK((merge_elements_compatible<cm_not_container,
                                          cm_seq>::value) == false);

    // ... on both
    D_CM_CHECK((merge_elements_compatible<int, int>::value)         == false);
    D_CM_CHECK((merge_elements_compatible<cm_pair, cm_pair>::value) == false);

    // the guard is iterability, not the presence of container-ish members
    D_CM_CHECK(is_iterable_container<cm_key_no_iter>::value == false);
    D_CM_CHECK((merge_elements_compatible<cm_key_no_iter,
                                          cm_map_like>::value) == false);

    // each conjunct separately necessary
    D_CM_CHECK(   ( (merge_elements_compatible<cm_seq, cm_seq>::value)
                        == true )
               && ( (merge_elements_compatible<cm_seq,
                                               cm_not_container>::value)
                        == false )
               && ( (merge_elements_compatible<cm_not_container,
                                               cm_seq>::value) == false ));

    return ok;
}

/*
tests_merge_elements_convertibility_is_either_way
  Verifies that the element clause admits a conversion in EITHER direction,
  not only left-to-right.

  A merge has to place both operands' survivors in one result, so it is enough
  that one element type reaches the other - which direction does not matter at
  this level.  Were the clause written with only the left-to-right term, a
  merge would be admitted or refused depending on which operand was named
  first, and the trait would stop being symmetric.

  Tests the following:
  - a pair convertible only left-to-right is compatible
  - the same pair named in the other order is also compatible, which a
    single-direction clause would refuse
  - the disjunction re-derived term by term on that pair, so it is visible that
    exactly one of the two conversion terms is carrying it
  - the same-type term carries pairs that neither conversion term does
*/
bool
tests_merge_elements_convertibility_is_either_way()
{
    bool ok = true;

    // convertible one way only
    D_CM_CHECK((elements_convertible<cm_from, cm_to>::value) == true);
    D_CM_CHECK((elements_convertible<cm_to, cm_from>::value) == false);
    D_CM_CHECK((elements_same_type<cm_from, cm_to>::value)   == false);

    // ... yet compatible in both orders
    D_CM_CHECK((merge_elements_compatible<cm_from, cm_to>::value) == true);
    D_CM_CHECK((merge_elements_compatible<cm_to, cm_from>::value) == true);

    // the disjunction, term by term, in the order that needs the LAST term
    D_CM_CHECK((merge_elements_compatible<cm_to, cm_from>::value)
                   == (    (elements_same_type<cm_to, cm_from>::value)
                        || (elements_convertible<cm_to, cm_from>::value)
                        || (elements_convertible<cm_from, cm_to>::value) ));

    // ... and in the order that needs the middle term
    D_CM_CHECK((merge_elements_compatible<cm_from, cm_to>::value)
                   == (    (elements_same_type<cm_from, cm_to>::value)
                        || (elements_convertible<cm_from, cm_to>::value)
                        || (elements_convertible<cm_to, cm_from>::value) ));

    // the same-type term carries what neither conversion term does
    D_CM_CHECK((elements_same_type<cm_seq, cm_seq>::value) == true);
    D_CM_CHECK((merge_elements_compatible<cm_seq, cm_seq>::value) == true);

    return ok;
}

/*
tests_merge_elements_cv_ref
  Verifies that the trait cleans BOTH of its operands.
  Tests the following:
  - the verdict is unchanged when either side is cv- or reference-qualified,
    true and false alike
  - the volatile spellings in particular
  - a trait cleaning only one of its two arguments would show here and not in a
    one-operand sweep
*/
bool
tests_merge_elements_cv_ref()
{
    bool ok = true;

    D_CM_CV_REF_AGREE2(merge_elements_compatible, cm_vector, cm_vector, true);
    D_CM_CV_REF_AGREE2(merge_elements_compatible, cm_seq, cm_seq, true);
    D_CM_CV_REF_AGREE2(merge_elements_compatible, cm_from, cm_to, true);
    D_CM_CV_REF_AGREE2(merge_elements_compatible, cm_vector, cm_vec_string,
                       false);
    D_CM_CV_REF_AGREE2(merge_elements_compatible, cm_seq, cm_not_container,
                       false);

    return ok;
}

/*
tests_merge_elements_companions
  Verifies the hand-written `_v` companion.

  This trait takes two parameters, so the trait_detect macro cannot emit its
  companion and the header writes one by hand under the same feature gates.
  That hand-written form is worth checking against ::value directly, since a
  macro-emitted companion cannot drift and a hand-written one can.

  Tests the following:
  - the companion agrees with ::value on carriers of each verdict
  - it is usable in a constant expression
  - the trait derives from integral_constant, so it carries the standard
    value_type and type members
  - the block is gated on variable-template support, as the header's own
    definition is
*/
bool
tests_merge_elements_companions()
{
    bool ok = true;

    // the standard metafunction shape
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_elements_compatible<cm_seq, cm_seq> >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_elements_compatible<cm_seq,
                                               cm_seq>::value_type,
                     bool>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK((merge_elements_compatible_v<cm_seq, cm_seq>));
    D_CM_STATIC_CHECK((merge_elements_compatible_v<int, int>) == false);

    D_CM_CHECK((merge_elements_compatible_v<cm_vector, cm_vector>)
                   == (merge_elements_compatible<cm_vector,
                                                 cm_vector>::value));
    D_CM_CHECK((merge_elements_compatible_v<cm_from, cm_to>)
                   == (merge_elements_compatible<cm_from, cm_to>::value));
    D_CM_CHECK((merge_elements_compatible_v<cm_vector, cm_vec_string>)
                   == (merge_elements_compatible<cm_vector,
                                                 cm_vec_string>::value));
    D_CM_CHECK((merge_elements_compatible_v<int, cm_vector>)
                   == (merge_elements_compatible<int, cm_vector>::value));
    D_CM_CHECK((merge_elements_compatible_v<const cm_vector&, cm_vector&&>)
                   == (merge_elements_compatible<cm_vector,
                                                 cm_vector>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
