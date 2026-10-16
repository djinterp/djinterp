/*******************************************************************************
* djinterp [core]                      sorted_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/core/container/traits/sorted_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "sorted_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   Sortedness signals
// ===========================================================================
//   Three detectors, one per way a type can declare an ordering guarantee: a
// comparator alias, a pair of interval bounds, or an explicit opt-in constant.
// The first two only NAME what they look for; the third READS it, which is the
// difference that decides how each behaves at its edges.


/*
tests_has_key_compare_helper
  Verifies the comparator tell - a `key_compare` alias - which is what
  separates a tree-backed associative container from a hashed one.
  Tests the following:
  - std::set, std::multiset and std::map carry it; std::unordered_set and
    std::unordered_map do not, which is precisely the distinction the order
    axis alone could not draw
  - the sequence containers do not carry it
  - a PRIVATE key_compare does not count, access control participating in the
    probe as it does throughout
  - the detection is independent of the order axis: it is a plain alias probe,
    and the cascade decides where its answer is consulted
*/
bool
tests_has_key_compare_helper()
{
    bool ok = true;

    // tree-backed associatives carry it
    D_SR_CHECK(internal::has_key_compare_helper<srt_set>::value      == true);
    D_SR_CHECK(internal::has_key_compare_helper<srt_multiset>::value == true);
    D_SR_CHECK(internal::has_key_compare_helper<srt_map>::value      == true);
    D_SR_CHECK(internal::has_key_compare_helper<srt_keyed_compare>::value
                   == true);

    // hashed ones do not - the distinction the order axis could not draw
    D_SR_CHECK(internal::has_key_compare_helper<srt_uset>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<srt_umap>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<srt_keyed>::value == false);

    // the sequence containers
    D_SR_CHECK(internal::has_key_compare_helper<srt_vector>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<srt_list>::value   == false);
    D_SR_CHECK(internal::has_key_compare_helper<srt_string>::value == false);

    // access control participates
    D_SR_CHECK(internal::has_key_compare_helper<srt_private_compare>::value
                   == false);

    // detected independently of the order axis
    D_SR_CHECK(internal::has_key_compare_helper<srt_sequence_compare>::value
                   == true);

    // non-containers
    D_SR_CHECK(internal::has_key_compare_helper<srt_bare>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<srt_pair>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<int>::value      == false);
    D_SR_CHECK(internal::has_key_compare_helper<void>::value     == false);

    return ok;
}

/*
tests_has_interval_bounds_helper
  Verifies the interval tell - a lower_bound AND an upper_bound member - on the
  shapes it is meant for.
  Tests the following:
  - a type declaring both static constant bounds is detected
  - a type declaring only one is not: the two are joined by a conjunction, and
    half an interval is no interval
  - ordinary containers declare neither and are not detected
  - std::set is NOT detected, though it has members of both names, because
    they are non-static member functions - see tests_interval_bounds_shapes
*/
bool
tests_has_interval_bounds_helper()
{
    bool ok = true;

    // the intended shape
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_bounds>::value
                   == true);
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_both_signals>::value
                   == true);

    // half an interval is no interval
    D_SR_CHECK(
        internal::has_interval_bounds_helper<srt_bounds_lower_only>::value
            == false);

    // ordinary containers
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_vector>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_list>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_sequence>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_bare>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<int>::value  == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<void>::value == false);

    // std::set has both names, as member functions, and is not detected
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_set>::value  == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_map>::value  == false);

    return ok;
}

/*
tests_interval_bounds_shapes
  Verifies exactly WHICH declarations the interval probe accepts, since the
  answer is broader than the name suggests.

  The helper names both members through decltype and never evaluates either.
  decltype of an id-expression is well-formed for a static constant, for a
  NON-STATIC data member, and for a static member function - and ill-formed
  only for a non-static member function.  So of the five shapes a pair of
  members called lower_bound/upper_bound could take, the probe accepts four and
  rejects one, and two of the four accepted are not intervals at all:

    static constexpr data members ...... accepted, and correct
    non-static data members ............ accepted, WRONGLY - these are
                                         per-object fields, not a domain
    static member functions ............ accepted, WRONGLY - a function is not
                                         a bound
    non-static member functions ........ rejected, correctly - the std::set
                                         shape, and the reason std::set is not
                                         mistaken for an interval carrier
    only one of the two ................ rejected, correctly

  Both false positives propagate all the way to sortedness::sorted, so a type
  that merely happens to carry two members of these names claims an ordering
  guarantee it never made.  Tightening the probe - requiring the members be
  static, or constant-evaluable - would close it.  Until then the behaviour is
  pinned so that any change to it is a deliberate one.

  Tests the following:
  - each of the five shapes above, on its own fixture
  - both false positives carried through to the final classification
  - the one correct rejection likewise carried through
*/
bool
tests_interval_bounds_shapes()
{
    bool ok = true;

    // accepted, and correct
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_bounds>::value
                   == true);
    D_SR_CHECK(sortedness_of<srt_bounds>::value == sortedness::sorted);

    // accepted, wrongly - per-object fields are not a domain
    D_SR_CHECK(
        internal::has_interval_bounds_helper<srt_bounds_nonstatic>::value
            == true);
    D_SR_CHECK(sortedness_of<srt_bounds_nonstatic>::value
                   == sortedness::sorted);
    D_SR_CHECK(is_sorted_container<srt_bounds_nonstatic>::value == true);

    // accepted, wrongly - a function is not a bound
    D_SR_CHECK(
        internal::has_interval_bounds_helper<srt_bounds_functions>::value
            == true);
    D_SR_CHECK(sortedness_of<srt_bounds_functions>::value
                   == sortedness::sorted);
    D_SR_CHECK(is_sorted_container<srt_bounds_functions>::value == true);

    // rejected, correctly - the std::set shape
    D_SR_CHECK(
        internal::has_interval_bounds_helper<
            srt_bounds_member_functions>::value == false);
    D_SR_CHECK(sortedness_of<srt_bounds_member_functions>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(internal::has_interval_bounds_helper<srt_set>::value == false);

    // rejected, correctly - half an interval
    D_SR_CHECK(
        internal::has_interval_bounds_helper<srt_bounds_lower_only>::value
            == false);
    D_SR_CHECK(sortedness_of<srt_bounds_lower_only>::value
                   == sortedness::order_dependent);

    // the two accepted-but-wrong shapes are indistinguishable from the right
    // one at the classification level, which is what makes them worth pinning
    D_SR_CHECK(sortedness_of<srt_bounds_nonstatic>::value
                   == sortedness_of<srt_bounds>::value);
    D_SR_CHECK(sortedness_of<srt_bounds_functions>::value
                   == sortedness_of<srt_bounds>::value);

    return ok;
}

/*
tests_sorted_invariant_helper
  Verifies the explicit opt-in - a `sorted_invariant` constant - which is the
  only signal in the header that is a VALUE read rather than a presence test.
  Tests the following:
  - a type with no such member reads false, from the primary template
  - a member equal to true asserts the invariant
  - a member equal to FALSE denies it, though the member is present and the
    specialisation is therefore the one selected.  This is the distinction that
    matters: the helper reports the member's value, so `= false` is a working
    opt-out and not merely a no-op
  - the value is contextually converted, so a non-bool constant works: non-zero
    asserts, zero denies
  - the answer propagates to the classification unchanged
*/
bool
tests_sorted_invariant_helper()
{
    bool ok = true;

    // absent - the primary template
    D_SR_CHECK(internal::sorted_invariant_helper<srt_sequence>::value
                   == false);
    D_SR_CHECK(internal::sorted_invariant_helper<srt_vector>::value == false);
    D_SR_CHECK(internal::sorted_invariant_helper<srt_set>::value    == false);
    D_SR_CHECK(internal::sorted_invariant_helper<srt_bare>::value   == false);
    D_SR_CHECK(internal::sorted_invariant_helper<int>::value        == false);
    D_SR_CHECK(internal::sorted_invariant_helper<void>::value       == false);

    // asserted
    D_SR_CHECK(internal::sorted_invariant_helper<srt_invariant_true>::value
                   == true);

    // denied, though present - a value read, not a presence test
    D_SR_CHECK(internal::sorted_invariant_helper<srt_invariant_false>::value
                   == false);

    // contextually converted
    D_SR_CHECK(internal::sorted_invariant_helper<srt_invariant_int_one>::value
                   == true);
    D_SR_CHECK(internal::sorted_invariant_helper<srt_invariant_int_zero>::value
                   == false);

    // and it reaches the classification
    D_SR_CHECK(sortedness_of<srt_invariant_true>::value
                   == sortedness::sorted);
    D_SR_CHECK(sortedness_of<srt_invariant_false>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(sortedness_of<srt_invariant_int_one>::value
                   == sortedness::sorted);
    D_SR_CHECK(sortedness_of<srt_invariant_int_zero>::value
                   == sortedness::order_dependent);

    // an opting-out type is indistinguishable from one that never opted in,
    // which is the intended reading
    D_SR_CHECK(sortedness_of<srt_invariant_false>::value
                   == sortedness_of<srt_sequence>::value);

    return ok;
}

/*
tests_sorted_invariant_shapes
  Verifies which declarations the invariant probe accepts, and documents the
  one that cannot be tested at all.

  THE UNTESTABLE SHAPE.  A NON-STATIC data member named sorted_invariant makes
  the header fail to compile.  The detection - decltype of the id-expression -
  is unevaluated and so succeeds for a non-static data member, selecting the
  specialisation; the specialisation's initialiser is then EVALUATED, and
  reading a non-static data member without an object is ill-formed.  Because
  that error is in the specialisation's body rather than its immediate context,
  substitution failure cannot rescue it, and the result is a hard error:

      error: invalid use of non-static data member 'X::sorted_invariant'

  It is raised by any use of sortedness_of, and so transitively by every
  predicate and by sorted_container_class.  This is the one shape whose fixture
  would break the build rather than fail an assertion, so the suite carries no
  such fixture; the bound is described here instead.  The asymmetry is worth
  noting: the interval helper only NAMES its members and therefore never
  evaluates anything, so the same declaration shape merely misclassifies there
  (see tests_interval_bounds_shapes) rather than failing to compile.

  A STATIC MEMBER FUNCTION is accepted, and asserts.  decltype names it, so the
  specialisation is selected; the function then decays to a pointer, and a
  function pointer is never null, so the cast yields true.  A member given this
  name by accident silently claims sortedness.

  A TYPE ALIAS is correctly rejected, decltype of a type name being ill-formed.

  Tests the following:
  - the static member function shape, asserted, and carried through to the
    classification
  - the type alias shape, correctly denied
  - the boundary itself: a static CONSTANT is read for its value while a static
    FUNCTION is read for its address, so two declarations of the same name and
    the same apparent intent give opposite answers when the constant is false
*/
bool
tests_sorted_invariant_shapes()
{
    bool ok = true;

    // a static member function asserts, by pointer decay
    D_SR_CHECK(
        internal::sorted_invariant_helper<srt_invariant_function>::value
            == true);
    D_SR_CHECK(sortedness_of<srt_invariant_function>::value
                   == sortedness::sorted);
    D_SR_CHECK(is_sorted_container<srt_invariant_function>::value == true);

    // a type alias is correctly rejected
    D_SR_CHECK(internal::sorted_invariant_helper<srt_invariant_alias>::value
                   == false);
    D_SR_CHECK(sortedness_of<srt_invariant_alias>::value
                   == sortedness::order_dependent);

    // the boundary: same name, opposite readings
    D_SR_CHECK(
        internal::sorted_invariant_helper<srt_invariant_function>::value
            != internal::sorted_invariant_helper<srt_invariant_false>::value);

    // a function-shaped invariant is indistinguishable from a true constant
    D_SR_CHECK(sortedness_of<srt_invariant_function>::value
                   == sortedness_of<srt_invariant_true>::value);

    // the two signals are joined by a disjunction, so either alone suffices
    // and both together are no different
    D_SR_CHECK(sortedness_of<srt_both_signals>::value == sortedness::sorted);
    D_SR_CHECK(   ( internal::has_interval_bounds_helper<
                        srt_both_signals>::value == true )
               && ( internal::sorted_invariant_helper<
                        srt_both_signals>::value == true ));
    D_SR_CHECK(sortedness_of<srt_both_signals>::value
                   == sortedness_of<srt_bounds>::value);
    D_SR_CHECK(sortedness_of<srt_both_signals>::value
                   == sortedness_of<srt_invariant_true>::value);

    return ok;
}

/*
tests_signals_cv_ref
  Verifies that all three signal probes honour the clean_t contract.
  Tests the following:
  - each probe is identical across all seven cv- and reference-qualified
    spellings, on a positive and a negative carrier
  - the volatile spellings in particular, which a probe phrased on the
    unstripped type would lose
*/
bool
tests_signals_cv_ref()
{
    bool ok = true;

    D_SR_CV_REF_AGREE(internal::has_key_compare_helper, srt_set,    true);
    D_SR_CV_REF_AGREE(internal::has_key_compare_helper, srt_uset,   false);
    D_SR_CV_REF_AGREE(internal::has_key_compare_helper, srt_vector, false);

    D_SR_CV_REF_AGREE(internal::has_interval_bounds_helper, srt_bounds, true);
    D_SR_CV_REF_AGREE(internal::has_interval_bounds_helper, srt_vector, false);
    D_SR_CV_REF_AGREE(internal::has_interval_bounds_helper,
                      srt_bounds_lower_only, false);

    D_SR_CV_REF_AGREE(internal::sorted_invariant_helper,
                      srt_invariant_true, true);
    D_SR_CV_REF_AGREE(internal::sorted_invariant_helper,
                      srt_invariant_false, false);
    D_SR_CV_REF_AGREE(internal::sorted_invariant_helper, srt_vector, false);

    // spelled out on one carrier of each probe
    D_SR_CHECK(internal::has_key_compare_helper<volatile srt_map>::value
                   == true);
    D_SR_CHECK(
        internal::has_interval_bounds_helper<const volatile srt_bounds&>::value
            == true);
    D_SR_CHECK(
        internal::sorted_invariant_helper<volatile srt_invariant_true>::value
            == true);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
