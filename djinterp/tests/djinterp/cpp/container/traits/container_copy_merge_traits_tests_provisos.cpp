/*******************************************************************************
* djinterp [cpp]                  container_copy_merge_traits_tests_provisos.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/container_copy_merge_traits_tests_provisos.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_copy_merge_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// V.   Merge provisos
// ===========================================================================
//   Three traits attached to a defined merge: the result type, the possibility
// of a key conflict, and the possibility of an overflow.  Two of the three are
// LEFT-BIASED - the result is taken as the left operand, and the overflow
// proviso reads the result - so they are the only asymmetric traits in the
// header, and that asymmetry is what these tests concentrate on.


/*
tests_merge_result_type
  Verifies the result-type alias.
  Tests the following:
  - the result is the left operand, cleaned of cv and reference qualification
  - it is the left operand whichever discipline the two carry, and whichever
    the right operand is
  - it is defined even where the merge is NOT - the alias is unconditional, so
    it names a type for an incompatible pair and for a non-container, and is
    meaningful only alongside is_mergeable.  Reading it without that check is
    the mistake its unconditional shape invites
  - it is asymmetric: swapping the operands changes the result
*/
bool tests_merge_result_type()
{
    bool ok = true;

    // the left operand
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector, DCmVector>,
                     DCmVector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector, DCmDeque>,
                     DCmVector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmSet, DCmMultiset>,
                     DCmSet>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmMap, DCmMultimap>,
                     DCmMap>::value);

    // cleaned
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<const DCmVector&, DCmVector>,
                     DCmVector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<volatile DCmVector, DCmVector>,
                     DCmVector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector&&, DCmVector>,
                     DCmVector>::value);

    // defined even where the merge is not
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector, DCmSet>,
                     DCmVector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<int, DCmVector>, int>::value);
    D_CM_CHECK((is_mergeable<int, DCmVector>::value) == false);

    // asymmetric
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector, DCmDeque>,
                     merge_result_type_t<DCmDeque, DCmVector> >::value
            == false);

    // it agrees with the trait's own nested alias
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type<DCmSet, DCmMultiset>::type,
                     merge_result_type_t<DCmSet, DCmMultiset> >::value);

    return ok;
}

/*
tests_merge_has_key_conflict
  Verifies the key-conflict proviso.

  A conflict is possible when the merge is keyed AND at least one operand is a
  map - a discipline admitting each key once.  Two multimaps repeat keys freely
  and raise none.

  Tests the following:
  - two maps conflict
  - a map with a multimap conflicts, in either order - one unique side is
    enough
  - two multimaps do not
  - the disjunction is over the OPERANDS' disciplines, not the result's, so the
    order of a map / multimap pair does not matter even though the result type
    does depend on order
  - the sequence and bag kinds never conflict, having no keys
*/
bool tests_merge_has_key_conflict()
{
    bool ok = true;

    // two maps
    D_CM_CHECK((merge_has_key_conflict<DCmMap, DCmMap>::value) == true);
    D_CM_CHECK((merge_has_key_conflict<DCmMapLike, DCmMapLike>::value)
                   == true);

    // one map is enough, in either order
    D_CM_CHECK((merge_has_key_conflict<DCmMap, DCmMultimap>::value) == true);
    D_CM_CHECK((merge_has_key_conflict<DCmMultimap, DCmMap>::value) == true);

    // ... so this proviso IS symmetric, though the result type is not
    D_CM_CHECK((merge_has_key_conflict<DCmMap, DCmMultimap>::value)
                   == (merge_has_key_conflict<DCmMultimap, DCmMap>::value));
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmMap, DCmMultimap>,
                     merge_result_type_t<DCmMultimap, DCmMap> >::value
            == false);

    // two multimaps repeat keys freely
    D_CM_CHECK((merge_has_key_conflict<DCmMultimap, DCmMultimap>::value)
                   == false);
    D_CM_CHECK((merge_has_key_conflict<DCmMultimapLike,
                                       DCmMultimapLike>::value) == false);

    // the unkeyed kinds never conflict
    D_CM_CHECK((merge_has_key_conflict<DCmVector, DCmVector>::value)
                   == false);
    D_CM_CHECK((merge_has_key_conflict<DCmSet, DCmSet>::value) == false);
    D_CM_CHECK((merge_has_key_conflict<DCmMultiset, DCmMultiset>::value)
                   == false);

    return ok;
}

/*
tests_merge_has_key_conflict_needs_keyed_merge
  Verifies the FIRST conjunct - that the proviso is gated on the merge being
  keyed at all, not merely on a map being present.
  Tests the following:
  - a map paired with something outside its family reports no conflict, though
    a map is present - the merge is refused, so there is nothing to conflict in
  - a map paired with a non-container likewise
  - the conjunction re-derived from its two parts on carriers of each kind
  - the two conjuncts are independent: a keyed merge without a map (two
    multimaps), and a map without a keyed merge (a map against a set), both
    occur and both report false
*/
bool tests_merge_has_key_conflict_needs_keyed_merge()
{
    bool ok = true;

    // a map present, but the merge is refused
    D_CM_CHECK((merge_kind_of<DCmMap, DCmSet>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_has_key_conflict<DCmMap, DCmSet>::value) == false);
    D_CM_CHECK((merge_has_key_conflict<DCmMap, DCmVector>::value) == false);
    D_CM_CHECK((merge_has_key_conflict<DCmMap, int>::value) == false);

    // the conjunction, re-derived
    D_CM_CHECK((merge_has_key_conflict<DCmMap, DCmMap>::value)
                   == (    ( (merge_kind_of<DCmMap, DCmMap>::value)
                                 == merge_kind::keyed_merge )
                        && (    ( merge_discipline_of<DCmMap>::value
                                      == merge_discipline::map )
                             || ( merge_discipline_of<DCmMap>::value
                                      == merge_discipline::map ) ) ));
    D_CM_CHECK((merge_has_key_conflict<DCmMultimap, DCmMultimap>::value)
                   == (    ( (merge_kind_of<DCmMultimap,
                                            DCmMultimap>::value)
                                 == merge_kind::keyed_merge )
                        && (    ( merge_discipline_of<DCmMultimap>::value
                                      == merge_discipline::map )
                             || ( merge_discipline_of<DCmMultimap>::value
                                      == merge_discipline::map ) ) ));

    // the conjuncts are independent: keyed without a map
    D_CM_CHECK(   ( (merge_kind_of<DCmMultimap, DCmMultimap>::value)
                        == merge_kind::keyed_merge )
               && ( (merge_has_key_conflict<DCmMultimap,
                                            DCmMultimap>::value) == false ));

    // ... and a map without a keyed merge
    D_CM_CHECK(   ( merge_discipline_of<DCmMap>::value
                        == merge_discipline::map )
               && ( (merge_kind_of<DCmMap, DCmSet>::value)
                        != merge_kind::keyed_merge )
               && ( (merge_has_key_conflict<DCmMap, DCmSet>::value)
                        == false ));

    // a conflict implies a keyed merge
    D_CM_CHECK(   !(merge_has_key_conflict<DCmMap, DCmMap>::value)
               || ( (merge_kind_of<DCmMap, DCmMap>::value)
                        == merge_kind::keyed_merge ));

    return ok;
}

/*
tests_merge_may_overflow
  Verifies the capacity proviso on carriers of each kind.
  Tests the following:
  - a bounded result may overflow, since a merge only grows
  - an unbounded result may not
  - the proviso is gated on the merge being DEFINED: a bounded left operand
    paired with something it cannot merge with reports false
  - the conjunction re-derived from its two parts
  - std::array is the standard bounded carrier and std::vector the unbounded
    one, so the verdict is not confined to synthetic types
*/
bool tests_merge_may_overflow()
{
    bool ok = true;

    // bounded results
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmArray>::value)   == true);
    D_CM_CHECK((DMergeMayOverflow<DCmExtent, DCmSeq>::value)    == true);
    D_CM_CHECK((DMergeMayOverflow<DCmStaticBounds, DCmSeq>::value)
                   == true);
    D_CM_CHECK((DMergeMayOverflow<DCmCapacityOnly, DCmSeq>::value) == true);

    // unbounded results
    D_CM_CHECK((DMergeMayOverflow<DCmVector, DCmVector>::value) == false);
    D_CM_CHECK((DMergeMayOverflow<DCmUnbounded, DCmSeq>::value)  == false);
    D_CM_CHECK((DMergeMayOverflow<DCmCapacityReserve, DCmSeq>::value)
                   == false);

    // gated on the merge being defined
    D_CM_CHECK((is_mergeable<DCmArray, DCmSet>::value) == false);
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmSet>::value) == false);
    D_CM_CHECK((DMergeMayOverflow<DCmArray, int>::value) == false);
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmVecString>::value) == false);

    // the conjunction, re-derived
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmArray>::value)
                   == (    (is_mergeable<DCmArray, DCmArray>::value)
                        && internal::has_tuple_size_helper<DCmArray>::value ));
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmSet>::value)
                   == (    (is_mergeable<DCmArray, DCmSet>::value)
                        && internal::has_tuple_size_helper<DCmArray>::value ));

    return ok;
}

/*
tests_merge_may_overflow_routes
  Verifies each of the four boundedness routes separately, and the anti-signal
  that switches the fourth one off.
  Tests the following:
  - each of the four signals reaches the proviso on its own, on a carrier that
    has that signal alone
  - the capacity route is a CONJUNCTION with the absence of reserve(), so a
    growable container carrying both reports false - which is why std::vector,
    which has a capacity(), is not treated as bounded
  - the disjunction is re-derived term by term on a carrier of each route
  - a carrier with none of the four reports false though the merge is defined
*/
bool tests_merge_may_overflow_routes()
{
    bool ok = true;

    // route 1: a static extent
    D_CM_CHECK(   ( internal::has_extent_helper<DCmExtent>::value == true )
               && ( (DMergeMayOverflow<DCmExtent, DCmSeq>::value) == true ));

    // route 2: a tuple_size specialization
    D_CM_CHECK(   ( internal::has_tuple_size_helper<DCmArray>::value == true )
               && ( (DMergeMayOverflow<DCmArray, DCmArray>::value)
                        == true ));

    // route 3: static interval bounds
    D_CM_CHECK(   ( internal::has_static_bounds_helper<
                        DCmStaticBounds>::value == true )
               && ( (DMergeMayOverflow<DCmStaticBounds, DCmSeq>::value)
                        == true ));

    // route 4: a capacity() with no reserve()
    D_CM_CHECK(   ( internal::has_capacity_helper<DCmCapacityOnly>::value
                        == true )
               && ( internal::has_reserve_helper<DCmCapacityOnly>::value
                        == false )
               && ( (DMergeMayOverflow<DCmCapacityOnly, DCmSeq>::value)
                        == true ));

    // ... and the anti-signal switching route 4 off
    D_CM_CHECK(   ( internal::has_capacity_helper<
                        DCmCapacityReserve>::value == true )
               && ( internal::has_reserve_helper<
                        DCmCapacityReserve>::value == true )
               && ( (DMergeMayOverflow<DCmCapacityReserve, DCmSeq>::value)
                        == false ));

    // ... which is why a std::vector is not bounded
    D_CM_CHECK(   ( internal::has_capacity_helper<DCmVector>::value == true )
               && ( (DMergeMayOverflow<DCmVector, DCmVector>::value)
                        == false ));

    // the disjunction, re-derived on a carrier of each route
    D_CM_CHECK((DMergeMayOverflow<DCmExtent, DCmSeq>::value)
                   == (    (is_mergeable<DCmExtent, DCmSeq>::value)
                        && (    internal::has_extent_helper<DCmExtent>::value
                             || internal::has_tuple_size_helper<
                                    DCmExtent>::value
                             || internal::has_static_bounds_helper<
                                    DCmExtent>::value
                             || (    internal::has_capacity_helper<
                                         DCmExtent>::value
                                  && !internal::has_reserve_helper<
                                          DCmExtent>::value ) ) ));

    // a half-open domain is not a bound, so it does not reach the proviso
    D_CM_CHECK(   ( (is_mergeable<DCmLowerBoundOnly, DCmSeq>::value)
                        == true )
               && ( (DMergeMayOverflow<DCmLowerBoundOnly, DCmSeq>::value)
                        == false ));

    // none of the four, merge defined
    D_CM_CHECK(   ( (is_mergeable<DCmUnbounded, DCmSeq>::value) == true )
               && ( (DMergeMayOverflow<DCmUnbounded, DCmSeq>::value)
                        == false ));

    return ok;
}

/*
tests_merge_may_overflow_is_left_biased
  Verifies that the proviso reads the RESULT type - the left operand - and not
  both operands.

  This is the one place the header's left-bias becomes observable in a boolean.
  A bounded container merged into an unbounded one cannot overflow, and an
  unbounded one merged into a bounded one can; which of the two is which
  depends entirely on the argument order.  A caller who writes the operands in
  the other order gets the opposite answer to the same real question.

  Tests the following:
  - a bounded left operand with an unbounded right may overflow
  - the same pair in the other order may not
  - the two verdicts differ, stated directly, while the merge kind is the same
    both ways round
  - the right operand's boundedness is not consulted at all: two carriers
    differing only in the right operand's bounds give the same verdict
*/
bool tests_merge_may_overflow_is_left_biased()
{
    bool ok = true;

    // bounded on the left
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmVector>::value) == true);

    // unbounded on the left
    D_CM_CHECK((DMergeMayOverflow<DCmVector, DCmArray>::value) == false);

    // the two differ, though the kind does not
    D_CM_CHECK((DMergeMayOverflow<DCmArray, DCmVector>::value)
                   != (DMergeMayOverflow<DCmVector, DCmArray>::value));
    D_CM_CHECK((merge_kind_of<DCmArray, DCmVector>::value)
                   == (merge_kind_of<DCmVector, DCmArray>::value));
    D_CM_CHECK((is_mergeable<DCmArray, DCmVector>::value)
                   == (is_mergeable<DCmVector, DCmArray>::value));

    // the right operand's bounds are not consulted
    D_CM_CHECK((DMergeMayOverflow<DCmExtent, DCmUnbounded>::value)
                   == (DMergeMayOverflow<DCmExtent, DCmArray>::value));
    D_CM_CHECK((DMergeMayOverflow<DCmUnbounded, DCmExtent>::value)
                   == (DMergeMayOverflow<DCmUnbounded, DCmSeq>::value));

    // ... and the result type is what it reads
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmArray, DCmVector>,
                     DCmArray>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector, DCmArray>,
                     DCmVector>::value);

    return ok;
}

/*
tests_provisos_cv_ref
  Verifies that all three provisos clean both of their operands.
  Tests the following:
  - the key-conflict proviso is unchanged across qualified spellings of either
    side
  - the overflow proviso likewise - which matters more here, since it reads the
    left operand as a TYPE rather than only for a boolean, so a missing clean
    would change the result type rather than merely a verdict
  - the result-type alias strips qualification from the left operand
  - the volatile spellings in particular
*/
bool tests_provisos_cv_ref()
{
    bool ok = true;

    D_CM_CV_REF_AGREE2(merge_has_key_conflict, DCmMap, DCmMap, true);
    D_CM_CV_REF_AGREE2(merge_has_key_conflict, DCmMultimap, DCmMultimap,
                       false);
    D_CM_CV_REF_AGREE2(merge_has_key_conflict, DCmVector, DCmVector, false);

    D_CM_CV_REF_AGREE2(DMergeMayOverflow, DCmArray, DCmArray, true);
    D_CM_CV_REF_AGREE2(DMergeMayOverflow, DCmVector, DCmVector, false);
    D_CM_CV_REF_AGREE2(DMergeMayOverflow, DCmArray, DCmVector, true);

    // the alias strips qualification
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<const volatile DCmArray&, DCmSeq>,
                     DCmArray>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<DCmVector&&, DCmSeq>,
                     DCmVector>::value);

    return ok;
}

/*
tests_provisos_companions
  Verifies the hand-written `_v` companions of the two boolean provisos.
  Tests the following:
  - each companion agrees with its ::value on carriers of each verdict
  - each is usable in a constant expression
  - merge_has_key_conflict derives from integral_constant while
    DMergeMayOverflow does NOT - it carries a bare `value` member - so the
    standard members are present on one and absent on the other
  - the block is gated on variable-template support, as the header's own
    definitions are
*/
bool tests_provisos_companions()
{
    bool ok = true;

    // the differing shapes
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_has_key_conflict<DCmMap, DCmMap> >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_has_key_conflict<DCmMap, DCmMap>::value_type,
                     bool>::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        DMergeMayOverflow<DCmArray, DCmArray> >::value
            == false);

    // constexpr
    D_CM_STATIC_CHECK((merge_has_key_conflict<DCmMap, DCmMap>::value));
    D_CM_STATIC_CHECK((DMergeMayOverflow<DCmArray, DCmArray>::value));
    D_CM_STATIC_CHECK((DMergeMayOverflow<DCmVector, DCmVector>::value)
                          == false);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK((merge_has_key_conflict_v<DCmMap, DCmMap>));
    D_CM_STATIC_CHECK((merge_may_overflow_v<DCmArray, DCmArray>));

    D_CM_CHECK((merge_has_key_conflict_v<DCmMap, DCmMultimap>)
                   == (merge_has_key_conflict<DCmMap, DCmMultimap>::value));
    D_CM_CHECK((merge_has_key_conflict_v<DCmMultimap, DCmMultimap>)
                   == (merge_has_key_conflict<DCmMultimap,
                                              DCmMultimap>::value));
    D_CM_CHECK((merge_has_key_conflict_v<DCmVector, DCmVector>)
                   == (merge_has_key_conflict<DCmVector, DCmVector>::value));
    D_CM_CHECK((merge_may_overflow_v<DCmArray, DCmVector>)
                   == (DMergeMayOverflow<DCmArray, DCmVector>::value));
    D_CM_CHECK((merge_may_overflow_v<DCmVector, DCmArray>)
                   == (DMergeMayOverflow<DCmVector, DCmArray>::value));
    D_CM_CHECK((merge_may_overflow_v<const DCmArray&, DCmArray>)
                   == (DMergeMayOverflow<DCmArray, DCmArray>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
