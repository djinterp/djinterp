/*******************************************************************************
* djinterp [core]               sorted_container_traits_tests_classification.cpp
*
*
* path:      /tests/djinterp/core/container/traits/sorted_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "sorted_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Sortedness classification
// ===========================================================================
//   Five states over two arms.  The cascade asks the ORDER axis first and only
// then consults a signal, so which signal is even looked at depends on the
// answer: an unordered container is read for its comparator, an ordered one
// for its interval bounds or its explicit invariant, and anything else is not
// a container at all.


/*
tests_sortedness_enum
  Verifies the enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration, so a state cannot be mistaken for an int
  - the five enumerators carry the implicit ordinals 0..4 in declaration order
    and are pairwise distinct
  - a value-initialised state is non_container, the safe default: an
    unclassified type claims nothing
  - a state is usable as a non-type template argument, which is what lets
    sortedness_of derive its `type` member from integral_constant
  - the underlying type is deliberately not asserted, this enumeration fixing
    none; the consequence pinned instead is the 0..7 value range
*/
bool
tests_sortedness_enum()
{
    bool ok = true;

    // scoped
    D_SR_STATIC_CHECK(std::is_enum<sortedness>::value == true);
    D_SR_STATIC_CHECK(std::is_convertible<sortedness, int>::value == false);

    // the implicit ordinals
    D_SR_CHECK(static_cast<int>(sortedness::non_container)   == 0);
    D_SR_CHECK(static_cast<int>(sortedness::unordered)       == 1);
    D_SR_CHECK(static_cast<int>(sortedness::monotone)        == 2);
    D_SR_CHECK(static_cast<int>(sortedness::order_dependent) == 3);
    D_SR_CHECK(static_cast<int>(sortedness::sorted)          == 4);

    // a value-initialised state claims nothing
    D_SR_CHECK(sortedness{} == sortedness::non_container);

    // pairwise distinct, over the whole enum
    for (int i = 0; i < srt_state_count; ++i)
    {
        for (int j = 0; j < srt_state_count; ++j)
        {
            const bool same = ( srt_state_at(i) == srt_state_at(j) );

            D_SR_CHECK(same == (i == j));
        }
    }

    // usable as a non-type template argument
    D_SR_STATIC_CHECK(
        std::integral_constant<sortedness, sortedness::monotone>::value
            == sortedness::monotone);

    return ok;
}

/*
tests_sortedness_name
  Verifies the diagnostic spelling of each state.
  Tests the following:
  - each of the five states spells its own name
  - the spellings are pairwise distinct, so a summary cannot conflate two
    states
  - the function is constexpr and usable in a constant expression
*/
bool
tests_sortedness_name()
{
    bool ok = true;

    // each state's spelling
    D_SR_CHECK(srt_name_is(sortedness::non_container,   "non_container"));
    D_SR_CHECK(srt_name_is(sortedness::unordered,       "unordered"));
    D_SR_CHECK(srt_name_is(sortedness::monotone,        "monotone"));
    D_SR_CHECK(srt_name_is(sortedness::order_dependent, "order_dependent"));
    D_SR_CHECK(srt_name_is(sortedness::sorted,          "sorted"));

    // pairwise distinct spellings
    for (int i = 0; i < srt_state_count; ++i)
    {
        for (int j = 0; j < srt_state_count; ++j)
        {
            const bool same =
                ( std::strcmp(sortedness_name(srt_state_at(i)),
                              sortedness_name(srt_state_at(j))) == 0 );

            D_SR_CHECK(same == (i == j));
        }
    }

    // constexpr
    D_SR_STATIC_CHECK(sortedness_name(sortedness::sorted)[0] == 's');

    return ok;
}

/*
tests_sortedness_of_cascade
  Verifies every branch of the classification, on a carrier for each of the
  five states.
  Tests the following:
  - an unordered container without a comparator is `unordered`
  - an unordered container WITH one is `monotone`
  - an ordered container with no signal is `order_dependent`
  - an ordered container with either signal is `sorted`
  - a non-container is `non_container`, whether or not it carries a key
  - the classification is total, every carrier landing on exactly one state
  - the cascade re-derived term by term, on a carrier for each arm
*/
bool
tests_sortedness_of_cascade()
{
    bool ok = true;

    // the unordered arm
    D_SR_CHECK(sortedness_of<srt_keyed>::value == sortedness::unordered);
    D_SR_CHECK(sortedness_of<srt_keyed_compare>::value
                   == sortedness::monotone);

    // the ordered arm
    D_SR_CHECK(sortedness_of<srt_sequence>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(sortedness_of<srt_bounds>::value == sortedness::sorted);
    D_SR_CHECK(sortedness_of<srt_invariant_true>::value
                   == sortedness::sorted);

    // neither arm
    D_SR_CHECK(sortedness_of<srt_bare>::value == sortedness::non_container);
    D_SR_CHECK(sortedness_of<srt_keyed_not_iterable>::value
                   == sortedness::non_container);
    D_SR_CHECK(sortedness_of<srt_pair>::value == sortedness::non_container);
    D_SR_CHECK(sortedness_of<int>::value      == sortedness::non_container);
    D_SR_CHECK(sortedness_of<void>::value     == sortedness::non_container);

    // the cascade, re-derived term by term
    D_SR_CHECK(sortedness_of<srt_keyed_compare>::value
                   == ( is_unordered_container<srt_keyed_compare>::value
                            ? ( internal::has_key_compare_helper<
                                    srt_keyed_compare>::value
                                    ? sortedness::monotone
                                    : sortedness::unordered )
                      : is_ordered_container<srt_keyed_compare>::value
                            ? ( (    internal::has_interval_bounds_helper<
                                         srt_keyed_compare>::value
                                  || internal::sorted_invariant_helper<
                                         srt_keyed_compare>::value )
                                    ? sortedness::sorted
                                    : sortedness::order_dependent )
                      :       sortedness::non_container ));
    D_SR_CHECK(sortedness_of<srt_bounds>::value
                   == ( is_unordered_container<srt_bounds>::value
                            ? ( internal::has_key_compare_helper<
                                    srt_bounds>::value
                                    ? sortedness::monotone
                                    : sortedness::unordered )
                      : is_ordered_container<srt_bounds>::value
                            ? ( (    internal::has_interval_bounds_helper<
                                         srt_bounds>::value
                                  || internal::sorted_invariant_helper<
                                         srt_bounds>::value )
                                    ? sortedness::sorted
                                    : sortedness::order_dependent )
                      :       sortedness::non_container ));
    D_SR_CHECK(sortedness_of<srt_bare>::value
                   == ( is_unordered_container<srt_bare>::value
                            ? ( internal::has_key_compare_helper<
                                    srt_bare>::value
                                    ? sortedness::monotone
                                    : sortedness::unordered )
                      : is_ordered_container<srt_bare>::value
                            ? ( (    internal::has_interval_bounds_helper<
                                         srt_bare>::value
                                  || internal::sorted_invariant_helper<
                                         srt_bare>::value )
                                    ? sortedness::sorted
                                    : sortedness::order_dependent )
                      :       sortedness::non_container ));

    return ok;
}

/*
tests_sortedness_of_precedence
  Verifies that the ORDER-axis test comes first, which decides which signal is
  even consulted.  Each fixture here carries a signal belonging to the OTHER
  arm, so it pins that the signal is ignored rather than merely outvoted.
  Tests the following:
  - a KEYED container declaring interval bounds reads `unordered`, not
    `sorted`: the unordered arm is entered first and its bounds are never
    looked at.  This is correct - interval bounds describe positions, and a
    keyed container has none - but it means the same declaration means
    different things on either arm
  - an ORDERED container declaring a comparator reads `order_dependent`, not
    `monotone`: the comparator branch lives inside the unordered arm only.
    Also correct - naming a comparator is not promising to have applied it
  - the two arms are therefore not interchangeable, shown by a pair of fixtures
    that differ only in whether a key_type is present
  - within the ordered arm the two signals are joined by a disjunction, so
    either alone reaches `sorted`
*/
bool
tests_sortedness_of_precedence()
{
    bool ok = true;

    // bounds on the unordered arm are never consulted
    D_SR_CHECK(   ( internal::has_interval_bounds_helper<
                        srt_keyed_bounds>::value == true )
               && ( sortedness_of<srt_keyed_bounds>::value
                        == sortedness::unordered ));
    D_SR_CHECK(sortedness_of<srt_keyed_bounds>::value != sortedness::sorted);

    // ... and the same declaration on the ordered arm does reach sorted
    D_SR_CHECK(sortedness_of<srt_bounds>::value == sortedness::sorted);
    D_SR_CHECK(   ( is_unordered_container<srt_keyed_bounds>::value == true )
               && ( is_ordered_container<srt_bounds>::value         == true )
               && ( sortedness_of<srt_keyed_bounds>::value
                        != sortedness_of<srt_bounds>::value ));

    // a comparator on the ordered arm is never consulted
    D_SR_CHECK(   ( internal::has_key_compare_helper<
                        srt_sequence_compare>::value == true )
               && ( sortedness_of<srt_sequence_compare>::value
                        == sortedness::order_dependent ));
    D_SR_CHECK(sortedness_of<srt_sequence_compare>::value
                   != sortedness::monotone);

    // ... and the same declaration on the unordered arm does reach monotone
    D_SR_CHECK(sortedness_of<srt_keyed_compare>::value
                   == sortedness::monotone);
    D_SR_CHECK(   ( is_ordered_container<srt_sequence_compare>::value == true )
               && ( is_unordered_container<srt_keyed_compare>::value  == true )
               && ( sortedness_of<srt_sequence_compare>::value
                        != sortedness_of<srt_keyed_compare>::value ));

    // an ordered container carrying a comparator is indistinguishable from one
    // carrying nothing at all
    D_SR_CHECK(sortedness_of<srt_sequence_compare>::value
                   == sortedness_of<srt_sequence>::value);

    // within the ordered arm, either signal alone suffices
    D_SR_CHECK(   ( internal::sorted_invariant_helper<srt_bounds>::value
                        == false )
               && ( sortedness_of<srt_bounds>::value == sortedness::sorted ));
    D_SR_CHECK(   ( internal::has_interval_bounds_helper<
                        srt_invariant_true>::value == false )
               && ( sortedness_of<srt_invariant_true>::value
                        == sortedness::sorted ));

    return ok;
}

/*
tests_sortedness_of_std_types
  Verifies the state each standard container receives - where the
  classification meets real interfaces.
  Tests the following:
  - the tree-backed associatives are `monotone`: they enumerate in comparator
    order without having positions to maintain
  - the hashed associatives are `unordered`, which is where the sortedness axis
    finally separates two containers the order axis could not tell apart
  - every sequence container is `order_dependent`: sortedness is a property an
    instance may happen to have, and the type promises nothing
  - a std::string is order_dependent too, being a sequence of characters and
    not a sorted structure
  - no standard container reaches `sorted`, that state being reserved for types
    that explicitly opt in - so a suite testing only the standard library would
    never exercise it
*/
bool
tests_sortedness_of_std_types()
{
    bool ok = true;

    // tree-backed: monotone
    D_SR_CHECK(sortedness_of<srt_set>::value      == sortedness::monotone);
    D_SR_CHECK(sortedness_of<srt_multiset>::value == sortedness::monotone);
    D_SR_CHECK(sortedness_of<srt_map>::value      == sortedness::monotone);

    // hashed: unordered - the separation the order axis could not draw
    D_SR_CHECK(sortedness_of<srt_uset>::value == sortedness::unordered);
    D_SR_CHECK(sortedness_of<srt_umap>::value == sortedness::unordered);
    D_SR_CHECK(sortedness_of<srt_map>::value != sortedness_of<srt_umap>::value);
    D_SR_CHECK(ordering_kind_of<srt_map>::value
                   == ordering_kind_of<srt_umap>::value);

    // sequences: order_dependent
    D_SR_CHECK(sortedness_of<srt_vector>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(sortedness_of<srt_list>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(sortedness_of<srt_array>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(sortedness_of<srt_string>::value
                   == sortedness::order_dependent);

    // non-containers
    D_SR_CHECK(sortedness_of<srt_pair>::value == sortedness::non_container);

    // no standard container is `sorted`
    D_SR_CHECK(sortedness_of<srt_set>::value    != sortedness::sorted);
    D_SR_CHECK(sortedness_of<srt_map>::value    != sortedness::sorted);
    D_SR_CHECK(sortedness_of<srt_vector>::value != sortedness::sorted);
    D_SR_CHECK(sortedness_of<srt_uset>::value   != sortedness::sorted);

    // ... and reaching it takes an explicit opt-in
    D_SR_CHECK(sortedness_of<srt_invariant_true>::value
                   == sortedness::sorted);

    return ok;
}

/*
tests_sortedness_out_of_range
  Verifies what the spelling function does with a value that names no
  enumerator.

  sortedness_name is a chained ternary that tests four of the five states and
  FALLS THROUGH to "sorted".  So an unnamed value reads as the state making the
  STRONGEST claim, rather than as non_container, the state that claims nothing.
  This is the same shape both spelling functions in the sibling header have, so
  it is a house pattern rather than a local slip; nothing in the header
  produces such a value, and it is pinned so that it stays deliberate.

  The value range probed is 0..7: the enumeration fixes no underlying type and
  four is its largest enumerator, so a three-bit field bounds it.  5, 6 and 7
  are the valid values that name nothing.

  Tests the following:
  - all three unnamed values in range read as "sorted"
  - they are therefore indistinguishable by name from the sorted state itself,
    though they differ from it as values
  - the named states remain pairwise distinct by name
  - a value-initialised state is non_container, so default construction is safe
    even though the fall-through is not
*/
bool
tests_sortedness_out_of_range()
{
    bool ok = true;

    const sortedness five  = static_cast<sortedness>(5);
    const sortedness six   = static_cast<sortedness>(6);
    const sortedness seven = static_cast<sortedness>(7);

    // the fall-through answers the strongest claim
    D_SR_CHECK(srt_name_is(five,  "sorted"));
    D_SR_CHECK(srt_name_is(six,   "sorted"));
    D_SR_CHECK(srt_name_is(seven, "sorted"));
    D_SR_CHECK(std::strcmp(sortedness_name(five),
                           sortedness_name(sortedness::sorted)) == 0);

    // though the values themselves differ
    D_SR_CHECK(five != sortedness::sorted);
    D_SR_CHECK(five != six);
    D_SR_CHECK(six  != seven);
    D_SR_CHECK(static_cast<int>(seven) == 7);

    // the named states stay distinct
    for (int i = 0; i < srt_state_count; ++i)
    {
        for (int j = 0; j < srt_state_count; ++j)
        {
            const bool same =
                ( std::strcmp(sortedness_name(srt_state_at(i)),
                              sortedness_name(srt_state_at(j))) == 0 );

            D_SR_CHECK(same == ( srt_state_at(i) == srt_state_at(j) ));
        }
    }

    // the default remains the state that claims nothing
    D_SR_CHECK(sortedness{} == sortedness::non_container);
    D_SR_CHECK(srt_name_is(sortedness{}, "non_container"));

    return ok;
}

/*
tests_sortedness_cv_ref
  Verifies that the classification honours the clean_t contract.
  Tests the following:
  - the classification is identical across all seven cv- and reference-
    qualified spellings, on a carrier for each of the five states
  - the volatile spellings in particular
*/
bool
tests_sortedness_cv_ref()
{
    bool ok = true;

    D_SR_CV_REF_AGREE(sortedness_of, srt_set,   sortedness::monotone);
    D_SR_CV_REF_AGREE(sortedness_of, srt_map,   sortedness::monotone);
    D_SR_CV_REF_AGREE(sortedness_of, srt_uset,  sortedness::unordered);
    D_SR_CV_REF_AGREE(sortedness_of, srt_vector,
                      sortedness::order_dependent);
    D_SR_CV_REF_AGREE(sortedness_of, srt_string,
                      sortedness::order_dependent);
    D_SR_CV_REF_AGREE(sortedness_of, srt_bounds, sortedness::sorted);
    D_SR_CV_REF_AGREE(sortedness_of, srt_invariant_true, sortedness::sorted);
    D_SR_CV_REF_AGREE(sortedness_of, srt_bare, sortedness::non_container);

    // spelled out on one carrier
    D_SR_CHECK(sortedness_of<volatile srt_set>::value
                   == sortedness::monotone);
    D_SR_CHECK(sortedness_of<const volatile srt_bounds&>::value
                   == sortedness::sorted);

    return ok;
}

/*
tests_sortedness_companions
  Verifies the `_t` alias and the `_v` companion.
  Tests the following:
  - the `_t` alias is the integral_constant naming the state, so the
    classification is carried in the type system and not merely as a value
  - the `_v` companion agrees with ::value across the five states
  - both are usable in a constant expression
  - the `_v` half is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool
tests_sortedness_companions()
{
    bool ok = true;

    // the _t alias is the integral_constant, exactly
    D_SR_STATIC_CHECK(
        std::is_same<sortedness_of_t<srt_set>,
                     std::integral_constant<sortedness,
                                            sortedness::monotone> >::value);
    D_SR_STATIC_CHECK(
        std::is_same<sortedness_of_t<srt_vector>,
                     std::integral_constant<
                         sortedness,
                         sortedness::order_dependent> >::value);
    D_SR_STATIC_CHECK(
        std::is_same<sortedness_of_t<srt_bare>,
                     std::integral_constant<
                         sortedness,
                         sortedness::non_container> >::value);

    // ... and carries the value the trait reports
    D_SR_CHECK(sortedness_of_t<srt_uset>::value
                   == sortedness_of<srt_uset>::value);
    D_SR_CHECK(sortedness_of_t<srt_bounds>::value
                   == sortedness_of<srt_bounds>::value);

    // usable in a constant expression
    D_SR_STATIC_CHECK(sortedness_of<srt_map>::value == sortedness::monotone);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_SR_STATIC_CHECK(sortedness_of_v<srt_set> == sortedness::monotone);

    D_SR_CHECK(sortedness_of_v<srt_set>    == sortedness_of<srt_set>::value);
    D_SR_CHECK(sortedness_of_v<srt_uset>   == sortedness_of<srt_uset>::value);
    D_SR_CHECK(sortedness_of_v<srt_vector> == sortedness_of<srt_vector>::value);
    D_SR_CHECK(sortedness_of_v<srt_bounds> == sortedness_of<srt_bounds>::value);
    D_SR_CHECK(sortedness_of_v<srt_bare>   == sortedness_of<srt_bare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
