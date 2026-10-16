/*******************************************************************************
* djinterp [core]                 container_copy_merge_traits_tests_provisos.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests_provisos.cpp
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
bool
tests_merge_result_type()
{
    bool ok = true;

    // the left operand
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector, cm_vector>,
                     cm_vector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector, cm_deque>,
                     cm_vector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_set, cm_multiset>,
                     cm_set>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_map, cm_multimap>,
                     cm_map>::value);

    // cleaned
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<const cm_vector&, cm_vector>,
                     cm_vector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<volatile cm_vector, cm_vector>,
                     cm_vector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector&&, cm_vector>,
                     cm_vector>::value);

    // defined even where the merge is not
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector, cm_set>,
                     cm_vector>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<int, cm_vector>, int>::value);
    D_CM_CHECK((is_mergeable<int, cm_vector>::value) == false);

    // asymmetric
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector, cm_deque>,
                     merge_result_type_t<cm_deque, cm_vector> >::value
            == false);

    // it agrees with the trait's own nested alias
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type<cm_set, cm_multiset>::type,
                     merge_result_type_t<cm_set, cm_multiset> >::value);

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
bool
tests_merge_has_key_conflict()
{
    bool ok = true;

    // two maps
    D_CM_CHECK((merge_has_key_conflict<cm_map, cm_map>::value) == true);
    D_CM_CHECK((merge_has_key_conflict<cm_map_like, cm_map_like>::value)
                   == true);

    // one map is enough, in either order
    D_CM_CHECK((merge_has_key_conflict<cm_map, cm_multimap>::value) == true);
    D_CM_CHECK((merge_has_key_conflict<cm_multimap, cm_map>::value) == true);

    // ... so this proviso IS symmetric, though the result type is not
    D_CM_CHECK((merge_has_key_conflict<cm_map, cm_multimap>::value)
                   == (merge_has_key_conflict<cm_multimap, cm_map>::value));
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_map, cm_multimap>,
                     merge_result_type_t<cm_multimap, cm_map> >::value
            == false);

    // two multimaps repeat keys freely
    D_CM_CHECK((merge_has_key_conflict<cm_multimap, cm_multimap>::value)
                   == false);
    D_CM_CHECK((merge_has_key_conflict<cm_multimap_like,
                                       cm_multimap_like>::value) == false);

    // the unkeyed kinds never conflict
    D_CM_CHECK((merge_has_key_conflict<cm_vector, cm_vector>::value)
                   == false);
    D_CM_CHECK((merge_has_key_conflict<cm_set, cm_set>::value) == false);
    D_CM_CHECK((merge_has_key_conflict<cm_multiset, cm_multiset>::value)
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
bool
tests_merge_has_key_conflict_needs_keyed_merge()
{
    bool ok = true;

    // a map present, but the merge is refused
    D_CM_CHECK((merge_kind_of<cm_map, cm_set>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_has_key_conflict<cm_map, cm_set>::value) == false);
    D_CM_CHECK((merge_has_key_conflict<cm_map, cm_vector>::value) == false);
    D_CM_CHECK((merge_has_key_conflict<cm_map, int>::value) == false);

    // the conjunction, re-derived
    D_CM_CHECK((merge_has_key_conflict<cm_map, cm_map>::value)
                   == (    ( (merge_kind_of<cm_map, cm_map>::value)
                                 == merge_kind::keyed_merge )
                        && (    ( merge_discipline_of<cm_map>::value
                                      == merge_discipline::map )
                             || ( merge_discipline_of<cm_map>::value
                                      == merge_discipline::map ) ) ));
    D_CM_CHECK((merge_has_key_conflict<cm_multimap, cm_multimap>::value)
                   == (    ( (merge_kind_of<cm_multimap,
                                            cm_multimap>::value)
                                 == merge_kind::keyed_merge )
                        && (    ( merge_discipline_of<cm_multimap>::value
                                      == merge_discipline::map )
                             || ( merge_discipline_of<cm_multimap>::value
                                      == merge_discipline::map ) ) ));

    // the conjuncts are independent: keyed without a map
    D_CM_CHECK(   ( (merge_kind_of<cm_multimap, cm_multimap>::value)
                        == merge_kind::keyed_merge )
               && ( (merge_has_key_conflict<cm_multimap,
                                            cm_multimap>::value) == false ));

    // ... and a map without a keyed merge
    D_CM_CHECK(   ( merge_discipline_of<cm_map>::value
                        == merge_discipline::map )
               && ( (merge_kind_of<cm_map, cm_set>::value)
                        != merge_kind::keyed_merge )
               && ( (merge_has_key_conflict<cm_map, cm_set>::value)
                        == false ));

    // a conflict implies a keyed merge
    D_CM_CHECK(   !(merge_has_key_conflict<cm_map, cm_map>::value)
               || ( (merge_kind_of<cm_map, cm_map>::value)
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
bool
tests_merge_may_overflow()
{
    bool ok = true;

    // bounded results
    D_CM_CHECK((merge_may_overflow<cm_array, cm_array>::value)   == true);
    D_CM_CHECK((merge_may_overflow<cm_extent, cm_seq>::value)    == true);
    D_CM_CHECK((merge_may_overflow<cm_static_bounds, cm_seq>::value)
                   == true);
    D_CM_CHECK((merge_may_overflow<cm_capacity_only, cm_seq>::value) == true);

    // unbounded results
    D_CM_CHECK((merge_may_overflow<cm_vector, cm_vector>::value) == false);
    D_CM_CHECK((merge_may_overflow<cm_unbounded, cm_seq>::value)  == false);
    D_CM_CHECK((merge_may_overflow<cm_capacity_reserve, cm_seq>::value)
                   == false);

    // gated on the merge being defined
    D_CM_CHECK((is_mergeable<cm_array, cm_set>::value) == false);
    D_CM_CHECK((merge_may_overflow<cm_array, cm_set>::value) == false);
    D_CM_CHECK((merge_may_overflow<cm_array, int>::value) == false);
    D_CM_CHECK((merge_may_overflow<cm_array, cm_vec_string>::value) == false);

    // the conjunction, re-derived
    D_CM_CHECK((merge_may_overflow<cm_array, cm_array>::value)
                   == (    (is_mergeable<cm_array, cm_array>::value)
                        && internal::has_tuple_size_helper<cm_array>::value ));
    D_CM_CHECK((merge_may_overflow<cm_array, cm_set>::value)
                   == (    (is_mergeable<cm_array, cm_set>::value)
                        && internal::has_tuple_size_helper<cm_array>::value ));

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
bool
tests_merge_may_overflow_routes()
{
    bool ok = true;

    // route 1: a static extent
    D_CM_CHECK(   ( internal::has_extent_helper<cm_extent>::value == true )
               && ( (merge_may_overflow<cm_extent, cm_seq>::value) == true ));

    // route 2: a tuple_size specialization
    D_CM_CHECK(   ( internal::has_tuple_size_helper<cm_array>::value == true )
               && ( (merge_may_overflow<cm_array, cm_array>::value)
                        == true ));

    // route 3: static interval bounds
    D_CM_CHECK(   ( internal::has_static_bounds_helper<
                        cm_static_bounds>::value == true )
               && ( (merge_may_overflow<cm_static_bounds, cm_seq>::value)
                        == true ));

    // route 4: a capacity() with no reserve()
    D_CM_CHECK(   ( internal::has_capacity_helper<cm_capacity_only>::value
                        == true )
               && ( internal::has_reserve_helper<cm_capacity_only>::value
                        == false )
               && ( (merge_may_overflow<cm_capacity_only, cm_seq>::value)
                        == true ));

    // ... and the anti-signal switching route 4 off
    D_CM_CHECK(   ( internal::has_capacity_helper<
                        cm_capacity_reserve>::value == true )
               && ( internal::has_reserve_helper<
                        cm_capacity_reserve>::value == true )
               && ( (merge_may_overflow<cm_capacity_reserve, cm_seq>::value)
                        == false ));

    // ... which is why a std::vector is not bounded
    D_CM_CHECK(   ( internal::has_capacity_helper<cm_vector>::value == true )
               && ( (merge_may_overflow<cm_vector, cm_vector>::value)
                        == false ));

    // the disjunction, re-derived on a carrier of each route
    D_CM_CHECK((merge_may_overflow<cm_extent, cm_seq>::value)
                   == (    (is_mergeable<cm_extent, cm_seq>::value)
                        && (    internal::has_extent_helper<cm_extent>::value
                             || internal::has_tuple_size_helper<
                                    cm_extent>::value
                             || internal::has_static_bounds_helper<
                                    cm_extent>::value
                             || (    internal::has_capacity_helper<
                                         cm_extent>::value
                                  && !internal::has_reserve_helper<
                                          cm_extent>::value ) ) ));

    // a half-open domain is not a bound, so it does not reach the proviso
    D_CM_CHECK(   ( (is_mergeable<cm_lower_bound_only, cm_seq>::value)
                        == true )
               && ( (merge_may_overflow<cm_lower_bound_only, cm_seq>::value)
                        == false ));

    // none of the four, merge defined
    D_CM_CHECK(   ( (is_mergeable<cm_unbounded, cm_seq>::value) == true )
               && ( (merge_may_overflow<cm_unbounded, cm_seq>::value)
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
bool
tests_merge_may_overflow_is_left_biased()
{
    bool ok = true;

    // bounded on the left
    D_CM_CHECK((merge_may_overflow<cm_array, cm_vector>::value) == true);

    // unbounded on the left
    D_CM_CHECK((merge_may_overflow<cm_vector, cm_array>::value) == false);

    // the two differ, though the kind does not
    D_CM_CHECK((merge_may_overflow<cm_array, cm_vector>::value)
                   != (merge_may_overflow<cm_vector, cm_array>::value));
    D_CM_CHECK((merge_kind_of<cm_array, cm_vector>::value)
                   == (merge_kind_of<cm_vector, cm_array>::value));
    D_CM_CHECK((is_mergeable<cm_array, cm_vector>::value)
                   == (is_mergeable<cm_vector, cm_array>::value));

    // the right operand's bounds are not consulted
    D_CM_CHECK((merge_may_overflow<cm_extent, cm_unbounded>::value)
                   == (merge_may_overflow<cm_extent, cm_array>::value));
    D_CM_CHECK((merge_may_overflow<cm_unbounded, cm_extent>::value)
                   == (merge_may_overflow<cm_unbounded, cm_seq>::value));

    // ... and the result type is what it reads
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_array, cm_vector>,
                     cm_array>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector, cm_array>,
                     cm_vector>::value);

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
bool
tests_provisos_cv_ref()
{
    bool ok = true;

    D_CM_CV_REF_AGREE2(merge_has_key_conflict, cm_map, cm_map, true);
    D_CM_CV_REF_AGREE2(merge_has_key_conflict, cm_multimap, cm_multimap,
                       false);
    D_CM_CV_REF_AGREE2(merge_has_key_conflict, cm_vector, cm_vector, false);

    D_CM_CV_REF_AGREE2(merge_may_overflow, cm_array, cm_array, true);
    D_CM_CV_REF_AGREE2(merge_may_overflow, cm_vector, cm_vector, false);
    D_CM_CV_REF_AGREE2(merge_may_overflow, cm_array, cm_vector, true);

    // the alias strips qualification
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<const volatile cm_array&, cm_seq>,
                     cm_array>::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_result_type_t<cm_vector&&, cm_seq>,
                     cm_vector>::value);

    return ok;
}

/*
tests_provisos_companions
  Verifies the hand-written `_v` companions of the two boolean provisos.
  Tests the following:
  - each companion agrees with its ::value on carriers of each verdict
  - each is usable in a constant expression
  - merge_has_key_conflict derives from integral_constant while
    merge_may_overflow does NOT - it carries a bare `value` member - so the
    standard members are present on one and absent on the other
  - the block is gated on variable-template support, as the header's own
    definitions are
*/
bool
tests_provisos_companions()
{
    bool ok = true;

    // the differing shapes
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_has_key_conflict<cm_map, cm_map> >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_has_key_conflict<cm_map, cm_map>::value_type,
                     bool>::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        merge_may_overflow<cm_array, cm_array> >::value
            == false);

    // constexpr
    D_CM_STATIC_CHECK((merge_has_key_conflict<cm_map, cm_map>::value));
    D_CM_STATIC_CHECK((merge_may_overflow<cm_array, cm_array>::value));
    D_CM_STATIC_CHECK((merge_may_overflow<cm_vector, cm_vector>::value)
                          == false);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK((merge_has_key_conflict_v<cm_map, cm_map>));
    D_CM_STATIC_CHECK((merge_may_overflow_v<cm_array, cm_array>));

    D_CM_CHECK((merge_has_key_conflict_v<cm_map, cm_multimap>)
                   == (merge_has_key_conflict<cm_map, cm_multimap>::value));
    D_CM_CHECK((merge_has_key_conflict_v<cm_multimap, cm_multimap>)
                   == (merge_has_key_conflict<cm_multimap,
                                              cm_multimap>::value));
    D_CM_CHECK((merge_has_key_conflict_v<cm_vector, cm_vector>)
                   == (merge_has_key_conflict<cm_vector, cm_vector>::value));
    D_CM_CHECK((merge_may_overflow_v<cm_array, cm_vector>)
                   == (merge_may_overflow<cm_array, cm_vector>::value));
    D_CM_CHECK((merge_may_overflow_v<cm_vector, cm_array>)
                   == (merge_may_overflow<cm_vector, cm_array>::value));
    D_CM_CHECK((merge_may_overflow_v<const cm_array&, cm_array>)
                   == (merge_may_overflow<cm_array, cm_array>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
