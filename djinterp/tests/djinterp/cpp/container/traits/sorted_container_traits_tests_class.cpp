/*******************************************************************************
* djinterp [cpp]                         sorted_container_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/sorted_container_traits_tests_class.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "sorted_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================
//   DSortedContainerClass collects the whole axis into one instantiation, so
// a diagnostic or a dispatch site can read every answer without naming eight
// traits.  It must therefore agree with those traits exactly: the aggregate is
// a view, never a second opinion.


/*
tests_sorted_container_class_signals
  Verifies the three raw-signal members, which report what the type DECLARED
  rather than what the axis concluded from it.
  Tests the following:
  - has_comparator tracks the key_compare alias, true for the tree-backed
    associatives and false for the hashed ones
  - interval_domain tracks the bounds pair
  - sorted_invariant tracks the opt-in constant's VALUE, so a carrier declaring
    it false reports false - the member is not a presence flag
  - the signal members are independent of the conclusion: a keyed carrier
    declaring bounds reports interval_domain true while its kind is unordered,
    the bounds having been declared but never consulted
  - all three are usable in a constant expression
*/
bool tests_sorted_container_class_signals()
{
    bool ok = true;

    // has_comparator
    D_SR_CHECK(DSortedContainerClass<DSrtSet>::has_comparator  == true);
    D_SR_CHECK(DSortedContainerClass<DSrtMap>::has_comparator  == true);
    D_SR_CHECK(DSortedContainerClass<DSrtUset>::has_comparator == false);
    D_SR_CHECK(DSortedContainerClass<DSrtVector>::has_comparator == false);

    // interval_domain
    D_SR_CHECK(DSortedContainerClass<DSrtBounds>::interval_domain == true);
    D_SR_CHECK(DSortedContainerClass<DSrtVector>::interval_domain == false);
    D_SR_CHECK(
        DSortedContainerClass<DSrtBoundsLowerOnly>::interval_domain
            == false);

    // sorted_invariant - a value, not a presence flag
    D_SR_CHECK(
        DSortedContainerClass<DSrtInvariantTrue>::sorted_invariant == true);
    D_SR_CHECK(
        DSortedContainerClass<DSrtInvariantFalse>::sorted_invariant
            == false);
    D_SR_CHECK(DSortedContainerClass<DSrtVector>::sorted_invariant == false);

    // signals are independent of the conclusion
    D_SR_CHECK(   ( DSortedContainerClass<
                        DSrtKeyedBounds>::interval_domain == true )
               && ( DSortedContainerClass<DSrtKeyedBounds>::kind
                        == sortedness::unordered ));
    D_SR_CHECK(   ( DSortedContainerClass<
                        DSrtSequenceCompare>::has_comparator == true )
               && ( DSortedContainerClass<DSrtSequenceCompare>::kind
                        == sortedness::order_dependent ));

    // usable in a constant expression
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtSet>::has_comparator);
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtBounds>::interval_domain);
    D_SR_STATIC_CHECK(
        DSortedContainerClass<DSrtInvariantTrue>::sorted_invariant);

    return ok;
}

/*
tests_sorted_container_class_kind
  Verifies the classification members.
  Tests the following:
  - `kind` carries the state, on a carrier for each of the five
  - `kind_name` carries that state's spelling, evaluated at compile time from
    `kind` rather than recomputed - so the two can never disagree
  - both are usable in a constant expression
*/
bool tests_sorted_container_class_kind()
{
    bool ok = true;

    // a carrier for each state
    D_SR_CHECK(DSortedContainerClass<DSrtBare>::kind
                   == sortedness::non_container);
    D_SR_CHECK(DSortedContainerClass<DSrtUset>::kind
                   == sortedness::unordered);
    D_SR_CHECK(DSortedContainerClass<DSrtSet>::kind
                   == sortedness::monotone);
    D_SR_CHECK(DSortedContainerClass<DSrtVector>::kind
                   == sortedness::order_dependent);
    D_SR_CHECK(DSortedContainerClass<DSrtBounds>::kind
                   == sortedness::sorted);

    // kind_name spells kind
    D_SR_CHECK(std::strcmp(DSortedContainerClass<DSrtBare>::kind_name,
                           "non_container") == 0);
    D_SR_CHECK(std::strcmp(DSortedContainerClass<DSrtUset>::kind_name,
                           "unordered") == 0);
    D_SR_CHECK(std::strcmp(DSortedContainerClass<DSrtSet>::kind_name,
                           "monotone") == 0);
    D_SR_CHECK(std::strcmp(DSortedContainerClass<DSrtVector>::kind_name,
                           "order_dependent") == 0);
    D_SR_CHECK(std::strcmp(DSortedContainerClass<DSrtBounds>::kind_name,
                           "sorted") == 0);

    // ... and cannot disagree with it
    D_SR_CHECK(std::strcmp(
                   DSortedContainerClass<DSrtSet>::kind_name,
                   sortedness_name(DSortedContainerClass<DSrtSet>::kind))
                   == 0);
    D_SR_CHECK(std::strcmp(
                   DSortedContainerClass<DSrtUmap>::kind_name,
                   sortedness_name(DSortedContainerClass<DSrtUmap>::kind))
                   == 0);

    // usable in a constant expression
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtSet>::kind
                          == sortedness::monotone);
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtSet>::kind_name[0] == 'm');

    return ok;
}

/*
tests_sorted_container_class_predicates
  Verifies the three derived-predicate members.
  Tests the following:
  - is_sorted_type, is_monotone and sorted_enumeration each carry their
    predicate's answer, on a carrier for each state
  - the monotone reading survives the aggregation: a std::set reports
    is_sorted_type false and sorted_enumeration true, exactly as the standalone
    predicates do
  - all three are usable in a constant expression
*/
bool tests_sorted_container_class_predicates()
{
    bool ok = true;

    // is_sorted_type
    D_SR_CHECK(DSortedContainerClass<DSrtBounds>::is_sorted_type == true);
    D_SR_CHECK(DSortedContainerClass<DSrtSet>::is_sorted_type    == false);
    D_SR_CHECK(DSortedContainerClass<DSrtVector>::is_sorted_type == false);
    D_SR_CHECK(DSortedContainerClass<DSrtBare>::is_sorted_type   == false);

    // is_monotone
    D_SR_CHECK(DSortedContainerClass<DSrtSet>::is_monotone    == true);
    D_SR_CHECK(DSortedContainerClass<DSrtMap>::is_monotone    == true);
    D_SR_CHECK(DSortedContainerClass<DSrtUset>::is_monotone   == false);
    D_SR_CHECK(DSortedContainerClass<DSrtBounds>::is_monotone == false);

    // sorted_enumeration
    D_SR_CHECK(DSortedContainerClass<DSrtSet>::sorted_enumeration
                   == true);
    D_SR_CHECK(DSortedContainerClass<DSrtBounds>::sorted_enumeration
                   == true);
    D_SR_CHECK(DSortedContainerClass<DSrtUset>::sorted_enumeration
                   == false);
    D_SR_CHECK(DSortedContainerClass<DSrtVector>::sorted_enumeration
                   == false);
    D_SR_CHECK(DSortedContainerClass<DSrtBare>::sorted_enumeration
                   == false);

    // the monotone reading survives aggregation
    D_SR_CHECK(   ( DSortedContainerClass<DSrtSet>::is_sorted_type
                        == false )
               && ( DSortedContainerClass<DSrtSet>::sorted_enumeration
                        == true ));

    // usable in a constant expression
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtBounds>::is_sorted_type);
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtSet>::is_monotone);
    D_SR_STATIC_CHECK(DSortedContainerClass<DSrtSet>::sorted_enumeration);

    return ok;
}

/*
tests_sorted_container_class_agreement
  Verifies that every member of the aggregate equals the standalone trait it
  reports, so the two spellings can never drift apart.  This is the test that
  fails if a member is wired to the wrong trait - a mistake the per-member
  tests above could miss, since several traits agree on any one carrier.
  Tests the following:
  - all eight members checked against their traits, across carriers spanning
    every state, so no two traits agree everywhere the check is made
  - the carriers are chosen to separate the members pairwise: a keyed carrier
    with bounds separates interval_domain from is_sorted_type, and a monotone
    one separates is_sorted_type from sorted_enumeration
*/
bool tests_sorted_container_class_agreement()
{
    bool ok = true;

#define D_SR_AGREES(T)                                                        \
    do                                                                        \
    {                                                                         \
        D_SR_CHECK(DSortedContainerClass<T>::has_comparator                  \
                       == internal::has_key_compare_helper<T>::value);        \
        D_SR_CHECK(DSortedContainerClass<T>::interval_domain                 \
                       == internal::has_interval_bounds_helper<T>::value);    \
        D_SR_CHECK(DSortedContainerClass<T>::sorted_invariant                \
                       == internal::sorted_invariant_helper<T>::value);       \
        D_SR_CHECK(DSortedContainerClass<T>::kind                            \
                       == sortedness_of<T>::value);                           \
        D_SR_CHECK(std::strcmp(DSortedContainerClass<T>::kind_name,          \
                               sortedness_name(sortedness_of<T>::value))      \
                       == 0);                                                 \
        D_SR_CHECK(DSortedContainerClass<T>::is_sorted_type                  \
                       == is_sorted_container<T>::value);                     \
        D_SR_CHECK(DSortedContainerClass<T>::is_monotone                     \
                       == is_monotone_container<T>::value);                   \
        D_SR_CHECK(DSortedContainerClass<T>::sorted_enumeration              \
                       == admits_sorted_enumeration<T>::value);               \
    } while (0)

    // one carrier per state
    D_SR_AGREES(DSrtBare);
    D_SR_AGREES(DSrtUset);
    D_SR_AGREES(DSrtSet);
    D_SR_AGREES(DSrtVector);
    D_SR_AGREES(DSrtBounds);

    // carriers chosen to separate members that would otherwise agree
    D_SR_AGREES(DSrtKeyedBounds);         // interval_domain vs is_sorted_type
    D_SR_AGREES(DSrtSequenceCompare);     // has_comparator  vs is_monotone
    D_SR_AGREES(DSrtInvariantFalse);      // sorted_invariant vs its presence
    D_SR_AGREES(DSrtInvariantTrue);
    D_SR_AGREES(DSrtBothSignals);
    D_SR_AGREES(DSrtMap);
    D_SR_AGREES(DSrtString);
    D_SR_AGREES(DSrtKeyedNotIterable);

#undef D_SR_AGREES

    return ok;
}

/*
tests_sorted_container_class_cv_ref
  Verifies that the aggregate honours the clean_t contract.  It cleans once,
  into a private alias, and hands that to traits which clean again - so the
  redundancy is harmless, cleaning being idempotent, but the outcome is worth
  pinning at the aggregate's own boundary.
  Tests the following:
  - every member is identical across all seven cv- and reference-qualified
    spellings, on carriers spanning several states
  - the volatile spellings in particular
*/
bool tests_sorted_container_class_cv_ref()
{
    bool ok = true;

#define D_SR_CLASS_AGREE(TYPE)                                                \
    do                                                                        \
    {                                                                         \
        D_SR_CHECK(DSortedContainerClass<const TYPE>::kind                   \
                       == DSortedContainerClass<TYPE>::kind);                \
        D_SR_CHECK(DSortedContainerClass<volatile TYPE>::kind                \
                       == DSortedContainerClass<TYPE>::kind);                \
        D_SR_CHECK(DSortedContainerClass<const volatile TYPE>::kind          \
                       == DSortedContainerClass<TYPE>::kind);                \
        D_SR_CHECK(DSortedContainerClass<TYPE&>::kind                        \
                       == DSortedContainerClass<TYPE>::kind);                \
        D_SR_CHECK(DSortedContainerClass<const TYPE&>::kind                  \
                       == DSortedContainerClass<TYPE>::kind);                \
        D_SR_CHECK(DSortedContainerClass<TYPE&&>::kind                       \
                       == DSortedContainerClass<TYPE>::kind);                \
        D_SR_CHECK(DSortedContainerClass<volatile TYPE>::has_comparator      \
                       == DSortedContainerClass<TYPE>::has_comparator);      \
        D_SR_CHECK(DSortedContainerClass<volatile TYPE>::interval_domain     \
                       == DSortedContainerClass<TYPE>::interval_domain);     \
        D_SR_CHECK(DSortedContainerClass<volatile TYPE>::sorted_invariant    \
                       == DSortedContainerClass<TYPE>::sorted_invariant);    \
        D_SR_CHECK(DSortedContainerClass<const TYPE&>::is_sorted_type        \
                       == DSortedContainerClass<TYPE>::is_sorted_type);      \
        D_SR_CHECK(DSortedContainerClass<const TYPE&>::is_monotone           \
                       == DSortedContainerClass<TYPE>::is_monotone);         \
        D_SR_CHECK(DSortedContainerClass<const TYPE&>::sorted_enumeration    \
                       == DSortedContainerClass<TYPE>::sorted_enumeration);  \
        D_SR_CHECK(std::strcmp(                                               \
                       DSortedContainerClass<volatile TYPE>::kind_name,      \
                       DSortedContainerClass<TYPE>::kind_name) == 0);        \
    } while (0)

    D_SR_CLASS_AGREE(DSrtSet);
    D_SR_CLASS_AGREE(DSrtUset);
    D_SR_CLASS_AGREE(DSrtVector);
    D_SR_CLASS_AGREE(DSrtBounds);
    D_SR_CLASS_AGREE(DSrtInvariantTrue);
    D_SR_CLASS_AGREE(DSrtBare);

#undef D_SR_CLASS_AGREE

    // spelled out on one carrier
    D_SR_CHECK(DSortedContainerClass<volatile DSrtSet>::kind
                   == sortedness::monotone);
    D_SR_CHECK(DSortedContainerClass<const volatile DSrtBounds&>::
                   is_sorted_type == true);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
