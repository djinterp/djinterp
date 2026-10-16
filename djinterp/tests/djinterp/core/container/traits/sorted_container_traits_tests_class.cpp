/*******************************************************************************
* djinterp [core]                        sorted_container_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/core/container/traits/sorted_container_traits_tests_class.cpp
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
//   sorted_container_class collects the whole axis into one instantiation, so
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
bool
tests_sorted_container_class_signals()
{
    bool ok = true;

    // has_comparator
    D_SR_CHECK(sorted_container_class<srt_set>::has_comparator  == true);
    D_SR_CHECK(sorted_container_class<srt_map>::has_comparator  == true);
    D_SR_CHECK(sorted_container_class<srt_uset>::has_comparator == false);
    D_SR_CHECK(sorted_container_class<srt_vector>::has_comparator == false);

    // interval_domain
    D_SR_CHECK(sorted_container_class<srt_bounds>::interval_domain == true);
    D_SR_CHECK(sorted_container_class<srt_vector>::interval_domain == false);
    D_SR_CHECK(
        sorted_container_class<srt_bounds_lower_only>::interval_domain
            == false);

    // sorted_invariant - a value, not a presence flag
    D_SR_CHECK(
        sorted_container_class<srt_invariant_true>::sorted_invariant == true);
    D_SR_CHECK(
        sorted_container_class<srt_invariant_false>::sorted_invariant
            == false);
    D_SR_CHECK(sorted_container_class<srt_vector>::sorted_invariant == false);

    // signals are independent of the conclusion
    D_SR_CHECK(   ( sorted_container_class<
                        srt_keyed_bounds>::interval_domain == true )
               && ( sorted_container_class<srt_keyed_bounds>::kind
                        == sortedness::unordered ));
    D_SR_CHECK(   ( sorted_container_class<
                        srt_sequence_compare>::has_comparator == true )
               && ( sorted_container_class<srt_sequence_compare>::kind
                        == sortedness::order_dependent ));

    // usable in a constant expression
    D_SR_STATIC_CHECK(sorted_container_class<srt_set>::has_comparator);
    D_SR_STATIC_CHECK(sorted_container_class<srt_bounds>::interval_domain);
    D_SR_STATIC_CHECK(
        sorted_container_class<srt_invariant_true>::sorted_invariant);

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
bool
tests_sorted_container_class_kind()
{
    bool ok = true;

    // a carrier for each state
    D_SR_CHECK(sorted_container_class<srt_bare>::kind
                   == sortedness::non_container);
    D_SR_CHECK(sorted_container_class<srt_uset>::kind
                   == sortedness::unordered);
    D_SR_CHECK(sorted_container_class<srt_set>::kind
                   == sortedness::monotone);
    D_SR_CHECK(sorted_container_class<srt_vector>::kind
                   == sortedness::order_dependent);
    D_SR_CHECK(sorted_container_class<srt_bounds>::kind
                   == sortedness::sorted);

    // kind_name spells kind
    D_SR_CHECK(std::strcmp(sorted_container_class<srt_bare>::kind_name,
                           "non_container") == 0);
    D_SR_CHECK(std::strcmp(sorted_container_class<srt_uset>::kind_name,
                           "unordered") == 0);
    D_SR_CHECK(std::strcmp(sorted_container_class<srt_set>::kind_name,
                           "monotone") == 0);
    D_SR_CHECK(std::strcmp(sorted_container_class<srt_vector>::kind_name,
                           "order_dependent") == 0);
    D_SR_CHECK(std::strcmp(sorted_container_class<srt_bounds>::kind_name,
                           "sorted") == 0);

    // ... and cannot disagree with it
    D_SR_CHECK(std::strcmp(
                   sorted_container_class<srt_set>::kind_name,
                   sortedness_name(sorted_container_class<srt_set>::kind))
                   == 0);
    D_SR_CHECK(std::strcmp(
                   sorted_container_class<srt_umap>::kind_name,
                   sortedness_name(sorted_container_class<srt_umap>::kind))
                   == 0);

    // usable in a constant expression
    D_SR_STATIC_CHECK(sorted_container_class<srt_set>::kind
                          == sortedness::monotone);
    D_SR_STATIC_CHECK(sorted_container_class<srt_set>::kind_name[0] == 'm');

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
bool
tests_sorted_container_class_predicates()
{
    bool ok = true;

    // is_sorted_type
    D_SR_CHECK(sorted_container_class<srt_bounds>::is_sorted_type == true);
    D_SR_CHECK(sorted_container_class<srt_set>::is_sorted_type    == false);
    D_SR_CHECK(sorted_container_class<srt_vector>::is_sorted_type == false);
    D_SR_CHECK(sorted_container_class<srt_bare>::is_sorted_type   == false);

    // is_monotone
    D_SR_CHECK(sorted_container_class<srt_set>::is_monotone    == true);
    D_SR_CHECK(sorted_container_class<srt_map>::is_monotone    == true);
    D_SR_CHECK(sorted_container_class<srt_uset>::is_monotone   == false);
    D_SR_CHECK(sorted_container_class<srt_bounds>::is_monotone == false);

    // sorted_enumeration
    D_SR_CHECK(sorted_container_class<srt_set>::sorted_enumeration
                   == true);
    D_SR_CHECK(sorted_container_class<srt_bounds>::sorted_enumeration
                   == true);
    D_SR_CHECK(sorted_container_class<srt_uset>::sorted_enumeration
                   == false);
    D_SR_CHECK(sorted_container_class<srt_vector>::sorted_enumeration
                   == false);
    D_SR_CHECK(sorted_container_class<srt_bare>::sorted_enumeration
                   == false);

    // the monotone reading survives aggregation
    D_SR_CHECK(   ( sorted_container_class<srt_set>::is_sorted_type
                        == false )
               && ( sorted_container_class<srt_set>::sorted_enumeration
                        == true ));

    // usable in a constant expression
    D_SR_STATIC_CHECK(sorted_container_class<srt_bounds>::is_sorted_type);
    D_SR_STATIC_CHECK(sorted_container_class<srt_set>::is_monotone);
    D_SR_STATIC_CHECK(sorted_container_class<srt_set>::sorted_enumeration);

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
bool
tests_sorted_container_class_agreement()
{
    bool ok = true;

#define D_SR_AGREES(T)                                                        \
    do                                                                        \
    {                                                                         \
        D_SR_CHECK(sorted_container_class<T>::has_comparator                  \
                       == internal::has_key_compare_helper<T>::value);        \
        D_SR_CHECK(sorted_container_class<T>::interval_domain                 \
                       == internal::has_interval_bounds_helper<T>::value);    \
        D_SR_CHECK(sorted_container_class<T>::sorted_invariant                \
                       == internal::sorted_invariant_helper<T>::value);       \
        D_SR_CHECK(sorted_container_class<T>::kind                            \
                       == sortedness_of<T>::value);                           \
        D_SR_CHECK(std::strcmp(sorted_container_class<T>::kind_name,          \
                               sortedness_name(sortedness_of<T>::value))      \
                       == 0);                                                 \
        D_SR_CHECK(sorted_container_class<T>::is_sorted_type                  \
                       == is_sorted_container<T>::value);                     \
        D_SR_CHECK(sorted_container_class<T>::is_monotone                     \
                       == is_monotone_container<T>::value);                   \
        D_SR_CHECK(sorted_container_class<T>::sorted_enumeration              \
                       == admits_sorted_enumeration<T>::value);               \
    } while (0)

    // one carrier per state
    D_SR_AGREES(srt_bare);
    D_SR_AGREES(srt_uset);
    D_SR_AGREES(srt_set);
    D_SR_AGREES(srt_vector);
    D_SR_AGREES(srt_bounds);

    // carriers chosen to separate members that would otherwise agree
    D_SR_AGREES(srt_keyed_bounds);         // interval_domain vs is_sorted_type
    D_SR_AGREES(srt_sequence_compare);     // has_comparator  vs is_monotone
    D_SR_AGREES(srt_invariant_false);      // sorted_invariant vs its presence
    D_SR_AGREES(srt_invariant_true);
    D_SR_AGREES(srt_both_signals);
    D_SR_AGREES(srt_map);
    D_SR_AGREES(srt_string);
    D_SR_AGREES(srt_keyed_not_iterable);

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
bool
tests_sorted_container_class_cv_ref()
{
    bool ok = true;

#define D_SR_CLASS_AGREE(TYPE)                                                \
    do                                                                        \
    {                                                                         \
        D_SR_CHECK(sorted_container_class<const TYPE>::kind                   \
                       == sorted_container_class<TYPE>::kind);                \
        D_SR_CHECK(sorted_container_class<volatile TYPE>::kind                \
                       == sorted_container_class<TYPE>::kind);                \
        D_SR_CHECK(sorted_container_class<const volatile TYPE>::kind          \
                       == sorted_container_class<TYPE>::kind);                \
        D_SR_CHECK(sorted_container_class<TYPE&>::kind                        \
                       == sorted_container_class<TYPE>::kind);                \
        D_SR_CHECK(sorted_container_class<const TYPE&>::kind                  \
                       == sorted_container_class<TYPE>::kind);                \
        D_SR_CHECK(sorted_container_class<TYPE&&>::kind                       \
                       == sorted_container_class<TYPE>::kind);                \
        D_SR_CHECK(sorted_container_class<volatile TYPE>::has_comparator      \
                       == sorted_container_class<TYPE>::has_comparator);      \
        D_SR_CHECK(sorted_container_class<volatile TYPE>::interval_domain     \
                       == sorted_container_class<TYPE>::interval_domain);     \
        D_SR_CHECK(sorted_container_class<volatile TYPE>::sorted_invariant    \
                       == sorted_container_class<TYPE>::sorted_invariant);    \
        D_SR_CHECK(sorted_container_class<const TYPE&>::is_sorted_type        \
                       == sorted_container_class<TYPE>::is_sorted_type);      \
        D_SR_CHECK(sorted_container_class<const TYPE&>::is_monotone           \
                       == sorted_container_class<TYPE>::is_monotone);         \
        D_SR_CHECK(sorted_container_class<const TYPE&>::sorted_enumeration    \
                       == sorted_container_class<TYPE>::sorted_enumeration);  \
        D_SR_CHECK(std::strcmp(                                               \
                       sorted_container_class<volatile TYPE>::kind_name,      \
                       sorted_container_class<TYPE>::kind_name) == 0);        \
    } while (0)

    D_SR_CLASS_AGREE(srt_set);
    D_SR_CLASS_AGREE(srt_uset);
    D_SR_CLASS_AGREE(srt_vector);
    D_SR_CLASS_AGREE(srt_bounds);
    D_SR_CLASS_AGREE(srt_invariant_true);
    D_SR_CLASS_AGREE(srt_bare);

#undef D_SR_CLASS_AGREE

    // spelled out on one carrier
    D_SR_CHECK(sorted_container_class<volatile srt_set>::kind
                   == sortedness::monotone);
    D_SR_CHECK(sorted_container_class<const volatile srt_bounds&>::
                   is_sorted_type == true);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
