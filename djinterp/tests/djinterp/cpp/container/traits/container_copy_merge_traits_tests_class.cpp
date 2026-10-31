/*******************************************************************************
* djinterp [cpp]                     container_copy_merge_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/container_copy_merge_traits_tests_class.cpp
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
//   DMergeClass restates the whole reading of a pair in eight members.  It
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
bool tests_merge_class_disciplines()
{
    bool ok = true;

    // restating the per-operand verdict
    D_CM_CHECK((DMergeClass<DCmVector, DCmVector>::left_discipline)
                   == merge_discipline_of<DCmVector>::value);
    D_CM_CHECK((DMergeClass<DCmMap, DCmMultimap>::left_discipline)
                   == merge_discipline_of<DCmMap>::value);
    D_CM_CHECK((DMergeClass<DCmMap, DCmMultimap>::right_discipline)
                   == merge_discipline_of<DCmMultimap>::value);

    // not crossed - the operands differ, so a swap would show
    D_CM_CHECK(   ( (DMergeClass<DCmMap, DCmMultimap>::left_discipline)
                        == merge_discipline::map )
               && ( (DMergeClass<DCmMap, DCmMultimap>::right_discipline)
                        == merge_discipline::multimap ));
    D_CM_CHECK(   ( (DMergeClass<DCmSet, DCmMultiset>::left_discipline)
                        == merge_discipline::set )
               && ( (DMergeClass<DCmSet, DCmMultiset>::right_discipline)
                        == merge_discipline::multiset ));
    D_CM_CHECK(   ( (DMergeClass<DCmVector, DCmMap>::left_discipline)
                        == merge_discipline::sequence )
               && ( (DMergeClass<DCmVector, DCmMap>::right_discipline)
                        == merge_discipline::map ));

    // swapping the operands swaps the members
    D_CM_CHECK((DMergeClass<DCmMap, DCmMultimap>::left_discipline)
                   == (DMergeClass<DCmMultimap, DCmMap>::right_discipline));
    D_CM_CHECK((DMergeClass<DCmMap, DCmMultimap>::right_discipline)
                   == (DMergeClass<DCmMultimap, DCmMap>::left_discipline));

    // a non-container operand
    D_CM_CHECK((DMergeClass<int, DCmVector>::left_discipline)
                   == merge_discipline::unknown);
    D_CM_CHECK((DMergeClass<DCmVector, int>::right_discipline)
                   == merge_discipline::unknown);

    // cleaned
    D_CM_CHECK((DMergeClass<const DCmMap&, DCmMultimap&&>::left_discipline)
                   == (DMergeClass<DCmMap, DCmMultimap>::left_discipline));

    // constexpr
    D_CM_STATIC_CHECK((DMergeClass<DCmMap, DCmMap>::left_discipline)
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
bool tests_merge_class_kind()
{
    bool ok = true;

    // restating the pair verdict
    D_CM_CHECK((DMergeClass<DCmVector, DCmVector>::kind)
                   == (merge_kind_of<DCmVector, DCmVector>::value));
    D_CM_CHECK((DMergeClass<DCmSet, DCmSet>::kind)
                   == (merge_kind_of<DCmSet, DCmSet>::value));
    D_CM_CHECK((DMergeClass<DCmMap, DCmMultimap>::kind)
                   == (merge_kind_of<DCmMap, DCmMultimap>::value));
    D_CM_CHECK((DMergeClass<DCmVector, DCmSet>::kind)
                   == (merge_kind_of<DCmVector, DCmSet>::value));

    // read literally, across the kinds
    D_CM_CHECK((DMergeClass<DCmVector, DCmDeque>::kind)
                   == merge_kind::concatenation);
    D_CM_CHECK((DMergeClass<DCmSet, DCmSet>::kind) == merge_kind::set_union);
    D_CM_CHECK((DMergeClass<DCmSet, DCmMultiset>::kind)
                   == merge_kind::multiset_union);
    D_CM_CHECK((DMergeClass<DCmMap, DCmMap>::kind)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((DMergeClass<DCmVector, DCmMap>::kind)
                   == merge_kind::incompatible);

    // the name tracks the kind rather than being fixed
    D_CM_CHECK(cm_kind_is(DMergeClass<DCmVector, DCmDeque>::kind,
                          "concatenation"));
    D_CM_CHECK(cm_kind_is(DMergeClass<DCmSet, DCmSet>::kind, "set_union"));
    D_CM_CHECK(cm_kind_is(DMergeClass<DCmSet, DCmMultiset>::kind,
                          "multiset_union"));
    D_CM_CHECK(cm_kind_is(DMergeClass<DCmMap, DCmMap>::kind, "keyed_merge"));
    D_CM_CHECK(cm_kind_is(DMergeClass<DCmVector, DCmMap>::kind,
                          "incompatible"));

    // ... and the member agrees with the free function on the member kind
    D_CM_CHECK(std::strcmp(DMergeClass<DCmSet, DCmSet>::kind_name,
                           merge_kind_name(
                               DMergeClass<DCmSet, DCmSet>::kind)) == 0);
    D_CM_CHECK(std::strcmp(DMergeClass<DCmMap, DCmMap>::kind_name,
                           merge_kind_name(
                               DMergeClass<DCmMap, DCmMap>::kind)) == 0);

    // the kind follows from the two discipline members
    D_CM_CHECK((DMergeClass<DCmSet, DCmMultiset>::kind)
                   == merge_kind_from_disciplines(
                          DMergeClass<DCmSet, DCmMultiset>::left_discipline,
                          DMergeClass<DCmSet,
                                      DCmMultiset>::right_discipline));

    // constexpr
    D_CM_STATIC_CHECK((DMergeClass<DCmSet, DCmSet>::kind)
                          == merge_kind::set_union);
    D_CM_STATIC_CHECK(DMergeClass<DCmSet, DCmSet>::kind_name[0] == 's');

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
bool tests_merge_class_provisos()
{
    bool ok = true;

    // each restating its trait
    D_CM_CHECK((DMergeClass<DCmVector, DCmVector>::elements_ok)
                   == (merge_elements_compatible<DCmVector,
                                                 DCmVector>::value));
    D_CM_CHECK((DMergeClass<DCmVector, DCmSet>::mergeable)
                   == (is_mergeable<DCmVector, DCmSet>::value));
    D_CM_CHECK((DMergeClass<DCmMap, DCmMap>::key_conflict)
                   == (merge_has_key_conflict<DCmMap, DCmMap>::value));
    D_CM_CHECK((DMergeClass<DCmArray, DCmArray>::may_overflow)
                   == (DMergeMayOverflow<DCmArray, DCmArray>::value));

    // a pair that merges with no conflict and no overflow
    D_CM_CHECK(   ( (DMergeClass<DCmVector, DCmVector>::elements_ok)
                        == true )
               && ( (DMergeClass<DCmVector, DCmVector>::mergeable) == true )
               && ( (DMergeClass<DCmVector, DCmVector>::key_conflict)
                        == false )
               && ( (DMergeClass<DCmVector, DCmVector>::may_overflow)
                        == false ));

    // a pair that conflicts
    D_CM_CHECK(   ( (DMergeClass<DCmMap, DCmMap>::mergeable) == true )
               && ( (DMergeClass<DCmMap, DCmMap>::key_conflict) == true )
               && ( (DMergeClass<DCmMap, DCmMap>::may_overflow) == false ));

    // a pair that may overflow
    D_CM_CHECK(   ( (DMergeClass<DCmArray, DCmArray>::mergeable) == true )
               && ( (DMergeClass<DCmArray, DCmArray>::key_conflict) == false )
               && ( (DMergeClass<DCmArray, DCmArray>::may_overflow)
                        == true ));

    // elements_ok and mergeable come apart
    D_CM_CHECK(   ( (DMergeClass<DCmVector, DCmSet>::elements_ok) == true )
               && ( (DMergeClass<DCmVector, DCmSet>::mergeable) == false ));

    // ... and agree where the elements are the cause
    D_CM_CHECK(   ( (DMergeClass<DCmVector, DCmVecString>::elements_ok)
                        == false )
               && ( (DMergeClass<DCmVector, DCmVecString>::mergeable)
                        == false ));

    // constexpr
    D_CM_STATIC_CHECK(DMergeClass<DCmMap, DCmMap>::key_conflict);
    D_CM_STATIC_CHECK(DMergeClass<DCmArray, DCmArray>::may_overflow);
    D_CM_STATIC_CHECK(
        (DMergeClass<DCmVector, DCmSet>::mergeable) == false);

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
bool tests_merge_class_agreement()
{
    bool ok = true;

    // spanning the kinds
    D_CM_CHECK((DMergeClass<DCmVector, DCmDeque>::kind)
                   == (merge_kind_of<DCmVector, DCmDeque>::value));
    D_CM_CHECK((DMergeClass<DCmSet, DCmSet>::kind)
                   == (merge_kind_of<DCmSet, DCmSet>::value));
    D_CM_CHECK((DMergeClass<DCmMultiset, DCmSet>::kind)
                   == (merge_kind_of<DCmMultiset, DCmSet>::value));
    D_CM_CHECK((DMergeClass<DCmMultimap, DCmMap>::kind)
                   == (merge_kind_of<DCmMultimap, DCmMap>::value));
    D_CM_CHECK((DMergeClass<int, int>::kind)
                   == (merge_kind_of<int, int>::value));

    // the asymmetric pairs
    D_CM_CHECK((DMergeClass<DCmArray, DCmVector>::may_overflow)
                   == (DMergeMayOverflow<DCmArray, DCmVector>::value));
    D_CM_CHECK((DMergeClass<DCmVector, DCmArray>::may_overflow)
                   == (DMergeMayOverflow<DCmVector, DCmArray>::value));
    D_CM_CHECK((DMergeClass<DCmArray, DCmVector>::may_overflow)
                   != (DMergeClass<DCmVector, DCmArray>::may_overflow));

    // the pair-sequence misread is inherited, not corrected
    D_CM_CHECK((DMergeClass<DCmPairVector, DCmMap>::left_discipline)
                   == merge_discipline_of<DCmPairVector>::value);
    D_CM_CHECK((DMergeClass<DCmPairVector, DCmMap>::kind)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((DMergeClass<DCmPairVector, DCmMap>::key_conflict) == true);

    // where the merge is refused
    D_CM_CHECK(   ( (DMergeClass<DCmVector, DCmVecString>::elements_ok)
                        == (merge_elements_compatible<
                                DCmVector, DCmVecString>::value) )
               && ( (DMergeClass<DCmVector, DCmVecString>::mergeable)
                        == (is_mergeable<DCmVector,
                                         DCmVecString>::value) )
               && ( (DMergeClass<DCmVector, DCmVecString>::key_conflict)
                        == (merge_has_key_conflict<
                                DCmVector, DCmVecString>::value) )
               && ( (DMergeClass<DCmVector, DCmVecString>::may_overflow)
                        == (DMergeMayOverflow<DCmVector,
                                               DCmVecString>::value) ));

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
bool tests_merge_class_cv_ref()
{
    bool ok = true;

    // the disciplines
    D_CM_CHECK((DMergeClass<const DCmMap, DCmMultimap>::left_discipline)
                   == (DMergeClass<DCmMap, DCmMultimap>::left_discipline));
    D_CM_CHECK((DMergeClass<volatile DCmMap, DCmMultimap>::left_discipline)
                   == (DMergeClass<DCmMap, DCmMultimap>::left_discipline));
    D_CM_CHECK((DMergeClass<DCmMap, const volatile DCmMultimap&
                           >::right_discipline)
                   == (DMergeClass<DCmMap, DCmMultimap>::right_discipline));
    D_CM_CHECK((DMergeClass<DCmMap&&, DCmMultimap&>::right_discipline)
                   == (DMergeClass<DCmMap, DCmMultimap>::right_discipline));

    // the kind
    D_CM_CHECK((DMergeClass<const DCmSet&, DCmMultiset&&>::kind)
                   == (DMergeClass<DCmSet, DCmMultiset>::kind));
    D_CM_CHECK((DMergeClass<volatile DCmVector, const DCmSet>::kind)
                   == (DMergeClass<DCmVector, DCmSet>::kind));

    // the booleans
    D_CM_CHECK((DMergeClass<const DCmMap&, const DCmMap&>::key_conflict)
                   == (DMergeClass<DCmMap, DCmMap>::key_conflict));
    D_CM_CHECK((DMergeClass<const DCmArray&, DCmVector>::may_overflow)
                   == (DMergeClass<DCmArray, DCmVector>::may_overflow));
    D_CM_CHECK((DMergeClass<volatile DCmVector, const DCmArray&
                           >::may_overflow)
                   == (DMergeClass<DCmVector, DCmArray>::may_overflow));
    D_CM_CHECK((DMergeClass<DCmVector&&, volatile DCmSet>::elements_ok)
                   == (DMergeClass<DCmVector, DCmSet>::elements_ok));
    D_CM_CHECK((DMergeClass<DCmVector&&, volatile DCmSet>::mergeable)
                   == (DMergeClass<DCmVector, DCmSet>::mergeable));

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
  - DMergeMayOverflow and DMergeClass supply no nested type at all
*/
bool tests_traits_metafunction_shape()
{
    bool ok = true;

    // derived from integral_constant
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_elements_compatible<DCmSeq, DCmSeq> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_copyable_container<DCmVector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        copy_preserves_all_axes<int> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_mergeable<DCmVector, DCmVector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_has_key_conflict<DCmMap, DCmMap> >::value);

    // ... so they convert to their value
    D_CM_CHECK(is_copyable_container<DCmVector>() == true);
    D_CM_CHECK((is_mergeable<DCmVector, DCmSet>()) == false);

    // NOT derived from integral_constant
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_discipline_of_t<DCmVector>,
                        merge_discipline_of<DCmVector> >::value == false);
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_kind_of_t<DCmVector, DCmVector>,
                        merge_kind_of<DCmVector, DCmVector> >::value
            == false);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        DMergeMayOverflow<DCmArray, DCmArray> >::value
            == false);

    // two of them supply a nested type by hand
    D_CM_STATIC_CHECK(
        std::is_same<merge_discipline_of<DCmVector>::type,
                     merge_discipline_of_t<DCmVector> >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of<DCmSet, DCmSet>::type,
                     merge_kind_of_t<DCmSet, DCmSet> >::value);

    // ... and those nested types ARE integral_constants
    D_CM_STATIC_CHECK(
        std::is_same<merge_discipline_of_t<DCmMap>::value_type,
                     merge_discipline>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of_t<DCmSet, DCmSet>::value_type,
                     merge_kind>::value);

    // the aggregate is a plain struct of constants
    D_CM_STATIC_CHECK(
        std::is_same<decltype(DMergeClass<DCmMap, DCmMap>::kind),
                     const merge_kind>::value);
    D_CM_STATIC_CHECK(
        std::is_same<decltype(DMergeClass<DCmMap, DCmMap>::key_conflict),
                     const bool>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
