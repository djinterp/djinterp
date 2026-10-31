/*******************************************************************************
* djinterp [cpp]                ordered_container_traits_tests_ordering_kind.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/ordered_container_traits_tests_ordering_kind.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "ordered_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  The ordering kind
// ===========================================================================
//   The two predicates of section I are complementary only over containers, so
// the axis needs a third state for everything that is not one.  ordering_kind
// supplies it, and ordering_kind_of collapses the pair of predicates into a
// single total classification.


/*
tests_ordering_kind_enum
  Verifies the enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration, so a kind cannot be mistaken for an int
  - the three enumerators carry the implicit ordinals 0..2 in declaration order
    and are pairwise distinct
  - a value-initialised kind is non_container, which is the safe default: an
    unclassified type claims nothing
  - a kind is usable as a non-type template argument, which is what lets
    ordering_kind_of derive its `type` member from integral_constant
  - the underlying type is deliberately not asserted: this enumeration does not
    fix one, so it is implementation-defined and no part of the contract.  The
    consequence that IS pinned, in tests_ordering_kind_out_of_range, is that
    the well-defined value range is only 0..3
*/
bool tests_ordering_kind_enum()
{
    bool ok = true;

    // scoped
    D_OR_STATIC_CHECK(std::is_enum<ordering_kind>::value == true);
    D_OR_STATIC_CHECK(std::is_convertible<ordering_kind, int>::value == false);

    // the implicit ordinals
    D_OR_CHECK(static_cast<int>(ordering_kind::non_container) == 0);
    D_OR_CHECK(static_cast<int>(ordering_kind::unordered)     == 1);
    D_OR_CHECK(static_cast<int>(ordering_kind::ordered)       == 2);

    // a value-initialised kind claims nothing
    D_OR_CHECK(ordering_kind{} == ordering_kind::non_container);

    // pairwise distinct, over the whole enum
    for (int i = 0; i < ord_kind_count; ++i)
    {
        for (int j = 0; j < ord_kind_count; ++j)
        {
            const bool same = ( ord_kind_at(i) == ord_kind_at(j) );

            D_OR_CHECK(same == (i == j));
        }
    }

    // usable as a non-type template argument
    D_OR_STATIC_CHECK(
        std::integral_constant<ordering_kind,
                               ordering_kind::ordered>::value
            == ordering_kind::ordered);

    return ok;
}

/*
tests_ordering_kind_name
  Verifies the diagnostic spelling of each state.
  Tests the following:
  - each of the three states spells its own name
  - the spellings are pairwise distinct, so a summary cannot conflate two
    states
  - the function is constexpr and usable in a constant expression
*/
bool tests_ordering_kind_name()
{
    bool ok = true;

    // each state's spelling
    D_OR_CHECK(ord_kind_name_is(ordering_kind::non_container, "non_container"));
    D_OR_CHECK(ord_kind_name_is(ordering_kind::unordered,     "unordered"));
    D_OR_CHECK(ord_kind_name_is(ordering_kind::ordered,       "ordered"));

    // pairwise distinct spellings
    for (int i = 0; i < ord_kind_count; ++i)
    {
        for (int j = 0; j < ord_kind_count; ++j)
        {
            const bool same =
                ( std::strcmp(ordering_kind_name(ord_kind_at(i)),
                              ordering_kind_name(ord_kind_at(j))) == 0 );

            D_OR_CHECK(same == (i == j));
        }
    }

    // constexpr
    D_OR_STATIC_CHECK(ordering_kind_name(ordering_kind::ordered)[0] == 'o');

    return ok;
}

/*
tests_ordering_kind_of
  Verifies the classification itself, on a carrier for each state.
  Tests the following:
  - every standard sequence container classifies ordered
  - every standard associative container classifies unordered, tree-backed and
    hash-backed alike
  - a non-iterable type classifies non_container, whether or not it carries a
    key_type - the iterability test comes first
  - the classification is total: every carrier the suite defines lands on
    exactly one of the three states
*/
bool tests_ordering_kind_of()
{
    bool ok = true;

    // ordered
    D_OR_CHECK(ordering_kind_of<DOrdVector>::value == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdArray>::value  == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdDeque>::value  == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdList>::value   == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdFlist>::value  == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdString>::value == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdNested>::value == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdSequence>::value
                   == ordering_kind::ordered);
    D_OR_CHECK(ordering_kind_of<DOrdInputOnly>::value
                   == ordering_kind::ordered);

    // unordered
    D_OR_CHECK(ordering_kind_of<DOrdSet>::value  == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<DOrdMap>::value  == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<DOrdUset>::value == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<DOrdUmap>::value == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<DOrdMultiset>::value
                   == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<DOrdKeyed>::value
                   == ordering_kind::unordered);

    // non_container - with and without a key_type
    D_OR_CHECK(ordering_kind_of<DOrdBare>::value
                   == ordering_kind::non_container);
    D_OR_CHECK(ordering_kind_of<DOrdPair>::value
                   == ordering_kind::non_container);
    D_OR_CHECK(ordering_kind_of<DOrdBeginOnly>::value
                   == ordering_kind::non_container);
    D_OR_CHECK(ordering_kind_of<int>::value
                   == ordering_kind::non_container);
    D_OR_CHECK(ordering_kind_of<void>::value
                   == ordering_kind::non_container);
    D_OR_CHECK(   ( internal::has_key_type_helper<
                        DOrdKeyedNotIterable>::value == true )
               && ( ordering_kind_of<DOrdKeyedNotIterable>::value
                        == ordering_kind::non_container ));

    return ok;
}

/*
tests_ordering_kind_of_agreement
  Verifies that the classification and the two predicates of section I are one
  statement in two forms, so neither can drift from the other.
  Tests the following:
  - ordered exactly when is_ordered_container, on every carrier
  - unordered exactly when is_unordered_container, on every carrier
  - non_container exactly when neither holds
  - the classification re-derived term by term from its own cascade, on a
    carrier for each branch
*/
bool tests_ordering_kind_of_agreement()
{
    bool ok = true;

    // ordered <-> is_ordered_container
    D_OR_CHECK(( ordering_kind_of<DOrdVector>::value == ordering_kind::ordered )
                   == is_ordered_container<DOrdVector>::value);
    D_OR_CHECK(( ordering_kind_of<DOrdSet>::value == ordering_kind::ordered )
                   == is_ordered_container<DOrdSet>::value);
    D_OR_CHECK(( ordering_kind_of<DOrdBare>::value == ordering_kind::ordered )
                   == is_ordered_container<DOrdBare>::value);
    D_OR_CHECK(( ordering_kind_of<DOrdFlist>::value == ordering_kind::ordered )
                   == is_ordered_container<DOrdFlist>::value);

    // unordered <-> is_unordered_container
    D_OR_CHECK(( ordering_kind_of<DOrdMap>::value
                     == ordering_kind::unordered )
                   == is_unordered_container<DOrdMap>::value);
    D_OR_CHECK(( ordering_kind_of<DOrdVector>::value
                     == ordering_kind::unordered )
                   == is_unordered_container<DOrdVector>::value);
    D_OR_CHECK(( ordering_kind_of<DOrdBare>::value
                     == ordering_kind::unordered )
                   == is_unordered_container<DOrdBare>::value);

    // non_container <-> neither
    D_OR_CHECK(( ordering_kind_of<DOrdBare>::value
                     == ordering_kind::non_container )
                   == !(    is_ordered_container<DOrdBare>::value
                         || is_unordered_container<DOrdBare>::value ));
    D_OR_CHECK(( ordering_kind_of<DOrdVector>::value
                     == ordering_kind::non_container )
                   == !(    is_ordered_container<DOrdVector>::value
                         || is_unordered_container<DOrdVector>::value ));
    D_OR_CHECK(( ordering_kind_of<DOrdKeyedNotIterable>::value
                     == ordering_kind::non_container )
                   == !(    is_ordered_container<
                                DOrdKeyedNotIterable>::value
                         || is_unordered_container<
                                DOrdKeyedNotIterable>::value ));

    // the cascade, re-derived term by term
    D_OR_CHECK(ordering_kind_of<DOrdMap>::value
                   == ( !is_iterable_container<DOrdMap>::value
                            ? ordering_kind::non_container
                      : internal::has_key_type_helper<DOrdMap>::value
                            ? ordering_kind::unordered
                      :       ordering_kind::ordered ));
    D_OR_CHECK(ordering_kind_of<DOrdVector>::value
                   == ( !is_iterable_container<DOrdVector>::value
                            ? ordering_kind::non_container
                      : internal::has_key_type_helper<DOrdVector>::value
                            ? ordering_kind::unordered
                      :       ordering_kind::ordered ));
    D_OR_CHECK(ordering_kind_of<DOrdKeyedNotIterable>::value
                   == ( !is_iterable_container<
                             DOrdKeyedNotIterable>::value
                            ? ordering_kind::non_container
                      : internal::has_key_type_helper<
                            DOrdKeyedNotIterable>::value
                            ? ordering_kind::unordered
                      :       ordering_kind::ordered ));

    return ok;
}

/*
tests_ordering_kind_out_of_range
  Verifies what the spelling function does with a value that names no
  enumerator.

  ordering_kind_name is a chained ternary that tests two of the three states
  and FALLS THROUGH to "ordered".  So an unnamed value does not read as
  non_container, the state that claims nothing - it reads as the state that
  claims the most.  Nothing in the header produces such a value, so this is a
  latent property rather than a live defect; it is pinned so that it stays a
  deliberate choice, and because the same shape recurs in
  sequential_layout_name and in the sibling header's sortedness_name.

  Note the range being probed.  This enumeration does not fix an underlying
  type, so its well-defined values are only those a two-bit field can hold -
  0 through 3 - because two is the largest enumerator.  3 is therefore the one
  valid value that names nothing, and a larger cast would not be well-defined
  at all.

  Tests the following:
  - the single unnamed value in range reads as "ordered"
  - it is therefore indistinguishable by name from the ordered state itself,
    though the two differ as values
  - by contrast a value-initialised kind is non_container, so default
    construction is safe even though the fall-through is not
*/
bool tests_ordering_kind_out_of_range()
{
    bool ok = true;

    const ordering_kind three = static_cast<ordering_kind>(3);

    // the fall-through answers the most-claiming state
    D_OR_CHECK(ord_kind_name_is(three, "ordered"));
    D_OR_CHECK(std::strcmp(ordering_kind_name(three),
                           ordering_kind_name(ordering_kind::ordered)) == 0);

    // ... though the values themselves differ
    D_OR_CHECK(three != ordering_kind::ordered);
    D_OR_CHECK(static_cast<int>(three) == 3);

    // the default remains the state that claims nothing
    D_OR_CHECK(ordering_kind{} == ordering_kind::non_container);
    D_OR_CHECK(ord_kind_name_is(ordering_kind{}, "non_container"));

    // and no NAMED state can be confused with another
    for (int i = 0; i < ord_kind_count; ++i)
    {
        for (int j = 0; j < ord_kind_count; ++j)
        {
            const bool same =
                ( std::strcmp(ordering_kind_name(ord_kind_at(i)),
                              ordering_kind_name(ord_kind_at(j))) == 0 );

            D_OR_CHECK(same == ( ord_kind_at(i) == ord_kind_at(j) ));
        }
    }

    return ok;
}

/*
tests_ordering_kind_cv_ref
  Verifies that the classification honours the clean_t contract.
  Tests the following:
  - the classification is identical across all seven cv- and reference-
    qualified spellings, on a carrier for each of the three states
  - the volatile spellings in particular
*/
bool tests_ordering_kind_cv_ref()
{
    bool ok = true;

    D_OR_CV_REF_AGREE(ordering_kind_of, DOrdVector, ordering_kind::ordered);
    D_OR_CV_REF_AGREE(ordering_kind_of, DOrdString, ordering_kind::ordered);
    D_OR_CV_REF_AGREE(ordering_kind_of, DOrdMap,    ordering_kind::unordered);
    D_OR_CV_REF_AGREE(ordering_kind_of, DOrdUset,   ordering_kind::unordered);
    D_OR_CV_REF_AGREE(ordering_kind_of, DOrdBare,
                      ordering_kind::non_container);
    D_OR_CV_REF_AGREE(ordering_kind_of, DOrdPair,
                      ordering_kind::non_container);

    // spelled out on one carrier
    D_OR_CHECK(ordering_kind_of<volatile DOrdVector>::value
                   == ordering_kind_of<DOrdVector>::value);
    D_OR_CHECK(ordering_kind_of<const volatile DOrdMap&>::value
                   == ordering_kind::unordered);

    return ok;
}

/*
tests_ordering_kind_companions
  Verifies the `_t` alias and the `_v` companion.
  Tests the following:
  - the `_t` alias is the integral_constant naming the classification, so the
    state is carried in the type system and not merely as a value
  - the `_v` companion agrees with ::value across the three states
  - both are usable in a constant expression
  - the `_v` half is gated on variable-template support
*/
bool tests_ordering_kind_companions()
{
    bool ok = true;

    // the _t alias is the integral_constant, exactly
    D_OR_STATIC_CHECK(
        std::is_same<ordering_kind_of_t<DOrdVector>,
                     std::integral_constant<ordering_kind,
                                            ordering_kind::ordered> >::value);
    D_OR_STATIC_CHECK(
        std::is_same<ordering_kind_of_t<DOrdMap>,
                     std::integral_constant<
                         ordering_kind,
                         ordering_kind::unordered> >::value);
    D_OR_STATIC_CHECK(
        std::is_same<ordering_kind_of_t<DOrdBare>,
                     std::integral_constant<
                         ordering_kind,
                         ordering_kind::non_container> >::value);

    // ... and carries the value the trait reports
    D_OR_CHECK(ordering_kind_of_t<DOrdFlist>::value
                   == ordering_kind_of<DOrdFlist>::value);
    D_OR_CHECK(ordering_kind_of_t<DOrdUset>::value
                   == ordering_kind_of<DOrdUset>::value);

    // usable in a constant expression
    D_OR_STATIC_CHECK(ordering_kind_of<DOrdVector>::value
                          == ordering_kind::ordered);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OR_STATIC_CHECK(ordering_kind_of_v<DOrdMap> == ordering_kind::unordered);

    D_OR_CHECK(ordering_kind_of_v<DOrdVector>
                   == ordering_kind_of<DOrdVector>::value);
    D_OR_CHECK(ordering_kind_of_v<DOrdMap>
                   == ordering_kind_of<DOrdMap>::value);
    D_OR_CHECK(ordering_kind_of_v<DOrdBare>
                   == ordering_kind_of<DOrdBare>::value);
    D_OR_CHECK(ordering_kind_of_v<DOrdString>
                   == ordering_kind_of<DOrdString>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
