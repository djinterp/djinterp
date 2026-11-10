/*******************************************************************************
* djinterp [cpp]                       sorted_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/sorted_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
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
bool tests_has_key_compare_helper()
{
    bool ok = true;

    // tree-backed associatives carry it
    D_SR_CHECK(internal::has_key_compare_helper<DSrtSet>::value      == true);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtMultiset>::value == true);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtMap>::value      == true);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtKeyedCompare>::value
                   == true);

    // hashed ones do not - the distinction the order axis could not draw
    D_SR_CHECK(internal::has_key_compare_helper<DSrtUset>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtUmap>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtKeyed>::value == false);

    // the sequence containers
    D_SR_CHECK(internal::has_key_compare_helper<DSrtVector>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtList>::value   == false);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtString>::value == false);

    // access control participates
    D_SR_CHECK(internal::has_key_compare_helper<DSrtPrivateCompare>::value
                   == false);

    // detected independently of the order axis
    D_SR_CHECK(internal::has_key_compare_helper<DSrtSequenceCompare>::value
                   == true);

    // non-containers
    D_SR_CHECK(internal::has_key_compare_helper<DSrtBare>::value == false);
    D_SR_CHECK(internal::has_key_compare_helper<DSrtPair>::value == false);
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
bool tests_has_interval_bounds_helper()
{
    bool ok = true;

    // the intended shape
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtBounds>::value
                   == true);
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtBothSignals>::value
                   == true);

    // half an interval is no interval
    D_SR_CHECK(
        internal::has_interval_bounds_helper<DSrtBoundsLowerOnly>::value
            == false);

    // ordinary containers
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtVector>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtList>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtSequence>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtBare>::value
                   == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<int>::value  == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<void>::value == false);

    // std::set has both names, as member functions, and is not detected
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtSet>::value  == false);
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtMap>::value  == false);

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
bool tests_interval_bounds_shapes()
{
    bool ok = true;

    // accepted, and correct
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtBounds>::value
                   == true);
    D_SR_CHECK(sortedness_of<DSrtBounds>::value == sortedness::sorted);

    // accepted, wrongly - per-object fields are not a domain
    D_SR_CHECK(
        internal::has_interval_bounds_helper<DSrtBoundsNonstatic>::value
            == true);
    D_SR_CHECK(sortedness_of<DSrtBoundsNonstatic>::value
                   == sortedness::sorted);
    D_SR_CHECK(is_sorted_container<DSrtBoundsNonstatic>::value == true);

    // accepted, wrongly - a function is not a bound
    D_SR_CHECK(
        internal::has_interval_bounds_helper<DSrtBoundsFunctions>::value
            == true);
    D_SR_CHECK(sortedness_of<DSrtBoundsFunctions>::value
                   == sortedness::sorted);
    D_SR_CHECK(is_sorted_container<DSrtBoundsFunctions>::value == true);

    // rejected, correctly - the std::set shape
    D_SR_CHECK(
        internal::has_interval_bounds_helper<
            DSrtBoundsMemberFunctions>::value == false);
    D_SR_CHECK(sortedness_of<DSrtBoundsMemberFunctions>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(internal::has_interval_bounds_helper<DSrtSet>::value == false);

    // rejected, correctly - half an interval
    D_SR_CHECK(
        internal::has_interval_bounds_helper<DSrtBoundsLowerOnly>::value
            == false);
    D_SR_CHECK(sortedness_of<DSrtBoundsLowerOnly>::value
                   == sortedness::order_dependent);

    // the two accepted-but-wrong shapes are indistinguishable from the right
    // one at the classification level, which is what makes them worth pinning
    D_SR_CHECK(sortedness_of<DSrtBoundsNonstatic>::value
                   == sortedness_of<DSrtBounds>::value);
    D_SR_CHECK(sortedness_of<DSrtBoundsFunctions>::value
                   == sortedness_of<DSrtBounds>::value);

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
bool tests_sorted_invariant_helper()
{
    bool ok = true;

    // absent - the primary template
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtSequence>::value
                   == false);
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtVector>::value == false);
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtSet>::value    == false);
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtBare>::value   == false);
    D_SR_CHECK(internal::sorted_invariant_helper<int>::value        == false);
    D_SR_CHECK(internal::sorted_invariant_helper<void>::value       == false);

    // asserted
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtInvariantTrue>::value
                   == true);

    // denied, though present - a value read, not a presence test
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtInvariantFalse>::value
                   == false);

    // contextually converted
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtInvariantIntOne>::value
                   == true);
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtInvariantIntZero>::value
                   == false);

    // and it reaches the classification
    D_SR_CHECK(sortedness_of<DSrtInvariantTrue>::value
                   == sortedness::sorted);
    D_SR_CHECK(sortedness_of<DSrtInvariantFalse>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(sortedness_of<DSrtInvariantIntOne>::value
                   == sortedness::sorted);
    D_SR_CHECK(sortedness_of<DSrtInvariantIntZero>::value
                   == sortedness::order_dependent);

    // an opting-out type is indistinguishable from one that never opted in,
    // which is the intended reading
    D_SR_CHECK(sortedness_of<DSrtInvariantFalse>::value
                   == sortedness_of<DSrtSequence>::value);

    return ok;
}

/*
tests_sorted_invariant_shapes
  Verifies which declarations the invariant probe accepts.

  The member must be a constant usable as a bool template argument. That one
  requirement rejects, in substitution's immediate context, the two shapes
  that used to misbehave (until 2026-11-09):
  - a STATIC MEMBER FUNCTION. Named by decltype it was accepted, and it
    decayed to a pointer that is never null, so it asserted sortedness; under
    GCC's sanitizers that comparison was not a constant expression, and the
    suite did not build.
  - a NON-STATIC data member. It was selected the same way and then read
    without an object, a hard error in any unit that classified the type.
  Both are now ignored like a missing member. (The interval helper only
  NAMES its members, so it never evaluated anything; see
  tests_interval_bounds_shapes.)

  A TYPE ALIAS is correctly rejected, decltype of a type name being ill-formed.

  Tests the following:
  - the static member function and the non-static shapes, each ignored, and
    carried through to the classification
  - the type alias shape, correctly denied
  - the boundary itself: a static CONSTANT is read for its value while a static
    FUNCTION is read for its address, so two declarations of the same name and
    the same apparent intent give opposite answers when the constant is false
*/
bool tests_sorted_invariant_shapes()
{
    bool ok = true;

    // a static member function is no constant, so it asserts nothing
    D_SR_CHECK(
        internal::sorted_invariant_helper<DSrtInvariantFunction>::value
            == false);
    D_SR_CHECK(sortedness_of<DSrtInvariantFunction>::value
                   == sortedness::order_dependent);
    D_SR_CHECK(is_sorted_container<DSrtInvariantFunction>::value == false);

    // nor is a non-static data member, read without an object
    D_SR_CHECK(
        internal::sorted_invariant_helper<DSrtInvariantNonStatic>::value
            == false);
    D_SR_CHECK(sortedness_of<DSrtInvariantNonStatic>::value
                   == sortedness::order_dependent);

    // a type alias is correctly rejected
    D_SR_CHECK(internal::sorted_invariant_helper<DSrtInvariantAlias>::value
                   == false);
    D_SR_CHECK(sortedness_of<DSrtInvariantAlias>::value
                   == sortedness::order_dependent);

    // a function and a false constant now read alike: neither asserts
    D_SR_CHECK(
        internal::sorted_invariant_helper<DSrtInvariantFunction>::value
            == internal::sorted_invariant_helper<DSrtInvariantFalse>::value);

    // and a function-shaped invariant is no longer a true constant's twin
    D_SR_CHECK(sortedness_of<DSrtInvariantFunction>::value
                   != sortedness_of<DSrtInvariantTrue>::value);

    // the two signals are joined by a disjunction, so either alone suffices
    // and both together are no different
    D_SR_CHECK(sortedness_of<DSrtBothSignals>::value == sortedness::sorted);
    D_SR_CHECK(   ( internal::has_interval_bounds_helper<
                        DSrtBothSignals>::value == true )
               && ( internal::sorted_invariant_helper<
                        DSrtBothSignals>::value == true ));
    D_SR_CHECK(sortedness_of<DSrtBothSignals>::value
                   == sortedness_of<DSrtBounds>::value);
    D_SR_CHECK(sortedness_of<DSrtBothSignals>::value
                   == sortedness_of<DSrtInvariantTrue>::value);

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
bool tests_signals_cv_ref()
{
    bool ok = true;

    D_SR_CV_REF_AGREE(internal::has_key_compare_helper, DSrtSet,    true);
    D_SR_CV_REF_AGREE(internal::has_key_compare_helper, DSrtUset,   false);
    D_SR_CV_REF_AGREE(internal::has_key_compare_helper, DSrtVector, false);

    D_SR_CV_REF_AGREE(internal::has_interval_bounds_helper, DSrtBounds, true);
    D_SR_CV_REF_AGREE(internal::has_interval_bounds_helper, DSrtVector, false);
    D_SR_CV_REF_AGREE(internal::has_interval_bounds_helper,
                      DSrtBoundsLowerOnly, false);

    D_SR_CV_REF_AGREE(internal::sorted_invariant_helper,
                      DSrtInvariantTrue, true);
    D_SR_CV_REF_AGREE(internal::sorted_invariant_helper,
                      DSrtInvariantFalse, false);
    D_SR_CV_REF_AGREE(internal::sorted_invariant_helper, DSrtVector, false);

    // spelled out on one carrier of each probe
    D_SR_CHECK(internal::has_key_compare_helper<volatile DSrtMap>::value
                   == true);
    D_SR_CHECK(
        internal::has_interval_bounds_helper<const volatile DSrtBounds&>::value
            == true);
    D_SR_CHECK(
        internal::sorted_invariant_helper<volatile DSrtInvariantTrue>::value
            == true);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
