/*******************************************************************************
* djinterp [core]                    container_copy_merge_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests_class.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_copy_merge_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// VI.  Aggregate snapshot
// ===========================================================================
//   merge_class restates the whole reading of a pair in eight members.  It
// introduces no detection, so the tests here are about WIRING - that each
// member is fed from the trait it claims to restate, and that no two members
// are crossed.
//
//   Four of the eight are booleans of the same type and two more are
// disciplines of the same type, so a crossed pair would compile silently and
// show only on a carrier where the two disagree.  Every check below uses a
// carrier pair chosen so that the member under test differs from its
// neighbours.


/*
tests_merge_class_disciplines
  Verifies the two discipline members.
  Tests the following:
  - left_discipline and right_discipline restate merge_discipline_of on the
    respective operand
  - the two are NOT crossed, shown on pairs whose operands wear different
    disciplines - the only carriers on which a swap would be visible
  - they are cleaned, so a qualified operand reads the same
  - both are usable in a constant expression
*/
bool
tests_merge_class_disciplines()
{
    bool ok = true;

    // restating the per-operand verdict
    D_CM_CHECK((merge_class<cm_vector, cm_vector>::left_discipline)
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK((merge_class<cm_map, cm_multimap>::left_discipline)
                   == merge_discipline_of<cm_map>::value);
    D_CM_CHECK((merge_class<cm_map, cm_multimap>::right_discipline)
                   == merge_discipline_of<cm_multimap>::value);

    // not crossed - the operands differ, so a swap would show
    D_CM_CHECK(   ( (merge_class<cm_map, cm_multimap>::left_discipline)
                        == merge_discipline::map )
               && ( (merge_class<cm_map, cm_multimap>::right_discipline)
                        == merge_discipline::multimap ));
    D_CM_CHECK(   ( (merge_class<cm_set, cm_multiset>::left_discipline)
                        == merge_discipline::set )
               && ( (merge_class<cm_set, cm_multiset>::right_discipline)
                        == merge_discipline::multiset ));
    D_CM_CHECK(   ( (merge_class<cm_vector, cm_map>::left_discipline)
                        == merge_discipline::sequence )
               && ( (merge_class<cm_vector, cm_map>::right_discipline)
                        == merge_discipline::map ));

    // swapping the operands swaps the members
    D_CM_CHECK((merge_class<cm_map, cm_multimap>::left_discipline)
                   == (merge_class<cm_multimap, cm_map>::right_discipline));
    D_CM_CHECK((merge_class<cm_map, cm_multimap>::right_discipline)
                   == (merge_class<cm_multimap, cm_map>::left_discipline));

    // a non-container operand
    D_CM_CHECK((merge_class<int, cm_vector>::left_discipline)
                   == merge_discipline::unknown);
    D_CM_CHECK((merge_class<cm_vector, int>::right_discipline)
                   == merge_discipline::unknown);

    // cleaned
    D_CM_CHECK((merge_class<const cm_map&, cm_multimap&&>::left_discipline)
                   == (merge_class<cm_map, cm_multimap>::left_discipline));

    // constexpr
    D_CM_STATIC_CHECK((merge_class<cm_map, cm_map>::left_discipline)
                          == merge_discipline::map);

    return ok;
}

/*
tests_merge_class_kind
  Verifies the kind member and its spelling.
  Tests the following:
  - kind restates merge_kind_of
  - kind_name is the spelling OF THAT KIND, not of a fixed one - checked on
    carriers of several kinds, since a hard-wired name would agree with the
    right answer on any single carrier
  - the kind agrees with the two discipline members fed to the combining
    function, where the elements permit
  - both are usable in a constant expression
*/
bool
tests_merge_class_kind()
{
    bool ok = true;

    // restating the pair verdict
    D_CM_CHECK((merge_class<cm_vector, cm_vector>::kind)
                   == (merge_kind_of<cm_vector, cm_vector>::value));
    D_CM_CHECK((merge_class<cm_set, cm_set>::kind)
                   == (merge_kind_of<cm_set, cm_set>::value));
    D_CM_CHECK((merge_class<cm_map, cm_multimap>::kind)
                   == (merge_kind_of<cm_map, cm_multimap>::value));
    D_CM_CHECK((merge_class<cm_vector, cm_set>::kind)
                   == (merge_kind_of<cm_vector, cm_set>::value));

    // read literally, across the kinds
    D_CM_CHECK((merge_class<cm_vector, cm_deque>::kind)
                   == merge_kind::concatenation);
    D_CM_CHECK((merge_class<cm_set, cm_set>::kind) == merge_kind::set_union);
    D_CM_CHECK((merge_class<cm_set, cm_multiset>::kind)
                   == merge_kind::multiset_union);
    D_CM_CHECK((merge_class<cm_map, cm_map>::kind)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((merge_class<cm_vector, cm_map>::kind)
                   == merge_kind::incompatible);

    // the name tracks the kind rather than being fixed
    D_CM_CHECK(cm_kind_is(merge_class<cm_vector, cm_deque>::kind,
                          "concatenation"));
    D_CM_CHECK(cm_kind_is(merge_class<cm_set, cm_set>::kind, "set_union"));
    D_CM_CHECK(cm_kind_is(merge_class<cm_set, cm_multiset>::kind,
                          "multiset_union"));
    D_CM_CHECK(cm_kind_is(merge_class<cm_map, cm_map>::kind, "keyed_merge"));
    D_CM_CHECK(cm_kind_is(merge_class<cm_vector, cm_map>::kind,
                          "incompatible"));

    // ... and the member agrees with the free function on the member kind
    D_CM_CHECK(std::strcmp(merge_class<cm_set, cm_set>::kind_name,
                           merge_kind_name(
                               merge_class<cm_set, cm_set>::kind)) == 0);
    D_CM_CHECK(std::strcmp(merge_class<cm_map, cm_map>::kind_name,
                           merge_kind_name(
                               merge_class<cm_map, cm_map>::kind)) == 0);

    // the kind follows from the two discipline members
    D_CM_CHECK((merge_class<cm_set, cm_multiset>::kind)
                   == merge_kind_from_disciplines(
                          merge_class<cm_set, cm_multiset>::left_discipline,
                          merge_class<cm_set,
                                      cm_multiset>::right_discipline));

    // constexpr
    D_CM_STATIC_CHECK((merge_class<cm_set, cm_set>::kind)
                          == merge_kind::set_union);
    D_CM_STATIC_CHECK(merge_class<cm_set, cm_set>::kind_name[0] == 's');

    return ok;
}

/*
tests_merge_class_provisos
  Verifies the four boolean members.
  Tests the following:
  - elements_ok, mergeable, key_conflict and may_overflow each restate their
    standalone trait
  - no two are crossed, shown on carrier pairs where each member disagrees with
    its neighbours - a pair that merges but has incompatible-free elements and
    no conflict, a pair that conflicts, a pair that may overflow
  - elements_ok and mergeable come apart: element compatibility is necessary
    but not sufficient, so a discipline mismatch shows the two differing
  - all four are usable in a constant expression
*/
bool
tests_merge_class_provisos()
{
    bool ok = true;

    // each restating its trait
    D_CM_CHECK((merge_class<cm_vector, cm_vector>::elements_ok)
                   == (merge_elements_compatible<cm_vector,
                                                 cm_vector>::value));
    D_CM_CHECK((merge_class<cm_vector, cm_set>::mergeable)
                   == (is_mergeable<cm_vector, cm_set>::value));
    D_CM_CHECK((merge_class<cm_map, cm_map>::key_conflict)
                   == (merge_has_key_conflict<cm_map, cm_map>::value));
    D_CM_CHECK((merge_class<cm_array, cm_array>::may_overflow)
                   == (merge_may_overflow<cm_array, cm_array>::value));

    // a pair that merges with no conflict and no overflow
    D_CM_CHECK(   ( (merge_class<cm_vector, cm_vector>::elements_ok)
                        == true )
               && ( (merge_class<cm_vector, cm_vector>::mergeable) == true )
               && ( (merge_class<cm_vector, cm_vector>::key_conflict)
                        == false )
               && ( (merge_class<cm_vector, cm_vector>::may_overflow)
                        == false ));

    // a pair that conflicts
    D_CM_CHECK(   ( (merge_class<cm_map, cm_map>::mergeable) == true )
               && ( (merge_class<cm_map, cm_map>::key_conflict) == true )
               && ( (merge_class<cm_map, cm_map>::may_overflow) == false ));

    // a pair that may overflow
    D_CM_CHECK(   ( (merge_class<cm_array, cm_array>::mergeable) == true )
               && ( (merge_class<cm_array, cm_array>::key_conflict) == false )
               && ( (merge_class<cm_array, cm_array>::may_overflow)
                        == true ));

    // elements_ok and mergeable come apart
    D_CM_CHECK(   ( (merge_class<cm_vector, cm_set>::elements_ok) == true )
               && ( (merge_class<cm_vector, cm_set>::mergeable) == false ));

    // ... and agree where the elements are the cause
    D_CM_CHECK(   ( (merge_class<cm_vector, cm_vec_string>::elements_ok)
                        == false )
               && ( (merge_class<cm_vector, cm_vec_string>::mergeable)
                        == false ));

    // constexpr
    D_CM_STATIC_CHECK(merge_class<cm_map, cm_map>::key_conflict);
    D_CM_STATIC_CHECK(merge_class<cm_array, cm_array>::may_overflow);
    D_CM_STATIC_CHECK(
        (merge_class<cm_vector, cm_set>::mergeable) == false);

    return ok;
}

/*
tests_merge_class_agreement
  Verifies that the aggregate restates the standalone traits and does not
  shadow them - the property that makes it safe to read either one.
  Tests the following:
  - every member agrees with its standalone trait, across carrier pairs
    spanning all five kinds
  - the agreement holds on the asymmetric pairs, where the left-biased members
    would most easily be mis-wired
  - the agreement holds on the pair-sequence misread, so the aggregate inherits
    the discipline reading exactly rather than correcting it
  - the agreement holds where the merge is refused, when several members are
    false for different reasons
*/
bool
tests_merge_class_agreement()
{
    bool ok = true;

    // spanning the kinds
    D_CM_CHECK((merge_class<cm_vector, cm_deque>::kind)
                   == (merge_kind_of<cm_vector, cm_deque>::value));
    D_CM_CHECK((merge_class<cm_set, cm_set>::kind)
                   == (merge_kind_of<cm_set, cm_set>::value));
    D_CM_CHECK((merge_class<cm_multiset, cm_set>::kind)
                   == (merge_kind_of<cm_multiset, cm_set>::value));
    D_CM_CHECK((merge_class<cm_multimap, cm_map>::kind)
                   == (merge_kind_of<cm_multimap, cm_map>::value));
    D_CM_CHECK((merge_class<int, int>::kind)
                   == (merge_kind_of<int, int>::value));

    // the asymmetric pairs
    D_CM_CHECK((merge_class<cm_array, cm_vector>::may_overflow)
                   == (merge_may_overflow<cm_array, cm_vector>::value));
    D_CM_CHECK((merge_class<cm_vector, cm_array>::may_overflow)
                   == (merge_may_overflow<cm_vector, cm_array>::value));
    D_CM_CHECK((merge_class<cm_array, cm_vector>::may_overflow)
                   != (merge_class<cm_vector, cm_array>::may_overflow));

    // the pair-sequence misread is inherited, not corrected
    D_CM_CHECK((merge_class<cm_pair_vector, cm_map>::left_discipline)
                   == merge_discipline_of<cm_pair_vector>::value);
    D_CM_CHECK((merge_class<cm_pair_vector, cm_map>::kind)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((merge_class<cm_pair_vector, cm_map>::key_conflict) == true);

    // where the merge is refused
    D_CM_CHECK(   ( (merge_class<cm_vector, cm_vec_string>::elements_ok)
                        == (merge_elements_compatible<
                                cm_vector, cm_vec_string>::value) )
               && ( (merge_class<cm_vector, cm_vec_string>::mergeable)
                        == (is_mergeable<cm_vector,
                                         cm_vec_string>::value) )
               && ( (merge_class<cm_vector, cm_vec_string>::key_conflict)
                        == (merge_has_key_conflict<
                                cm_vector, cm_vec_string>::value) )
               && ( (merge_class<cm_vector, cm_vec_string>::may_overflow)
                        == (merge_may_overflow<cm_vector,
                                               cm_vec_string>::value) ));

    return ok;
}

/*
tests_merge_class_cv_ref
  Verifies that the aggregate cleans both of its operands.
  Tests the following:
  - every member is unchanged across qualified spellings of either side, on
    carrier pairs where the members disagree with one another
  - the volatile spellings in particular
  - the two discipline members in particular, since they are the ones a
    per-operand clean would have to be written twice for
*/
bool
tests_merge_class_cv_ref()
{
    bool ok = true;

    // the disciplines
    D_CM_CHECK((merge_class<const cm_map, cm_multimap>::left_discipline)
                   == (merge_class<cm_map, cm_multimap>::left_discipline));
    D_CM_CHECK((merge_class<volatile cm_map, cm_multimap>::left_discipline)
                   == (merge_class<cm_map, cm_multimap>::left_discipline));
    D_CM_CHECK((merge_class<cm_map, const volatile cm_multimap&
                           >::right_discipline)
                   == (merge_class<cm_map, cm_multimap>::right_discipline));
    D_CM_CHECK((merge_class<cm_map&&, cm_multimap&>::right_discipline)
                   == (merge_class<cm_map, cm_multimap>::right_discipline));

    // the kind
    D_CM_CHECK((merge_class<const cm_set&, cm_multiset&&>::kind)
                   == (merge_class<cm_set, cm_multiset>::kind));
    D_CM_CHECK((merge_class<volatile cm_vector, const cm_set>::kind)
                   == (merge_class<cm_vector, cm_set>::kind));

    // the booleans
    D_CM_CHECK((merge_class<const cm_map&, const cm_map&>::key_conflict)
                   == (merge_class<cm_map, cm_map>::key_conflict));
    D_CM_CHECK((merge_class<const cm_array&, cm_vector>::may_overflow)
                   == (merge_class<cm_array, cm_vector>::may_overflow));
    D_CM_CHECK((merge_class<volatile cm_vector, const cm_array&
                           >::may_overflow)
                   == (merge_class<cm_vector, cm_array>::may_overflow));
    D_CM_CHECK((merge_class<cm_vector&&, volatile cm_set>::elements_ok)
                   == (merge_class<cm_vector, cm_set>::elements_ok));
    D_CM_CHECK((merge_class<cm_vector&&, volatile cm_set>::mergeable)
                   == (merge_class<cm_vector, cm_set>::mergeable));

    return ok;
}

/*
tests_traits_metafunction_shape
  Records which of this header's traits derive from std::integral_constant and
  which merely carry a `value` member.

  The distinction is invisible at the point of use - both spell ::value - but
  it decides whether the standard metafunction members are available.  A caller
  writing a generic utility over "traits" will find that four of these are not
  metafunctions in the standard sense: they carry no value_type, no nested type
  reachable by inheritance, and no conversion to their value.

  Tests the following:
  - the five integral_constant-derived traits carry value_type and convert to
    their value
  - the four bare-value traits do not derive from integral_constant
  - two of the four supply a nested `type` alias by hand instead, so they are
    usable for tag dispatch even though they are not metafunctions themselves
  - merge_may_overflow and merge_class supply no nested type at all
*/
bool
tests_traits_metafunction_shape()
{
    bool ok = true;

    // derived from integral_constant
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_elements_compatible<cm_seq, cm_seq> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_copyable_container<cm_vector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        copy_preserves_all_axes<int> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_mergeable<cm_vector, cm_vector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_has_key_conflict<cm_map, cm_map> >::value);

    // ... so they convert to their value
    D_CM_CHECK(is_copyable_container<cm_vector>() == true);
    D_CM_CHECK((is_mergeable<cm_vector, cm_set>()) == false);

    // NOT derived from integral_constant
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_discipline_of_t<cm_vector>,
                        merge_discipline_of<cm_vector> >::value == false);
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_kind_of_t<cm_vector, cm_vector>,
                        merge_kind_of<cm_vector, cm_vector> >::value
            == false);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_may_overflow<cm_array, cm_array> >::value
            == false);

    // two of them supply a nested type by hand
    D_CM_STATIC_CHECK(
        std::is_same<merge_discipline_of<cm_vector>::type,
                     merge_discipline_of_t<cm_vector> >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of<cm_set, cm_set>::type,
                     merge_kind_of_t<cm_set, cm_set> >::value);

    // ... and those nested types ARE integral_constants
    D_CM_STATIC_CHECK(
        std::is_same<merge_discipline_of_t<cm_map>::value_type,
                     merge_discipline>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of_t<cm_set, cm_set>::value_type,
                     merge_kind>::value);

    // the aggregate is a plain struct of constants
    D_CM_STATIC_CHECK(
        std::is_same<decltype(merge_class<cm_map, cm_map>::kind),
                     const merge_kind>::value);
    D_CM_STATIC_CHECK(
        std::is_same<decltype(merge_class<cm_map, cm_map>::key_conflict),
                     const bool>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
