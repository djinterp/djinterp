/*******************************************************************************
* djinterp [cpp]                    sorted_container_traits_tests_predicates.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/sorted_container_traits_tests_predicates.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "sorted_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. Classification predicates
// ===========================================================================
//   Four convenience predicates over the five states.  Three name a single
// state; one - admits_sorted_enumeration - names a union, and it is that union
// which carries the header's real dispatch question.


/*
tests_is_sorted_container
  Verifies the strictest predicate: the type maintains its POSITIONS in
  comparator order.
  Tests the following:
  - only a carrier that opted in, by interval bounds or by explicit invariant,
    qualifies
  - no standard container qualifies, not even std::set - a set has no positions
    to maintain, which is the whole distinction between sorted and monotone
  - a non-container does not qualify
  - it equals `sortedness_of == sorted` on every carrier
*/
bool tests_is_sorted_container()
{
    bool ok = true;

    // the opted-in carriers
    D_SR_CHECK(is_sorted_container<DSrtBounds>::value         == true);
    D_SR_CHECK(is_sorted_container<DSrtInvariantTrue>::value == true);
    D_SR_CHECK(is_sorted_container<DSrtBothSignals>::value   == true);

    // no standard container qualifies
    D_SR_CHECK(is_sorted_container<DSrtSet>::value      == false);
    D_SR_CHECK(is_sorted_container<DSrtMultiset>::value == false);
    D_SR_CHECK(is_sorted_container<DSrtMap>::value      == false);
    D_SR_CHECK(is_sorted_container<DSrtUset>::value     == false);
    D_SR_CHECK(is_sorted_container<DSrtVector>::value   == false);
    D_SR_CHECK(is_sorted_container<DSrtString>::value   == false);

    // nor does an opted-out or a signal-free carrier
    D_SR_CHECK(is_sorted_container<DSrtInvariantFalse>::value == false);
    D_SR_CHECK(is_sorted_container<DSrtBoundsLowerOnly>::value == false);
    D_SR_CHECK(is_sorted_container<DSrtSequence>::value == false);

    // non-containers
    D_SR_CHECK(is_sorted_container<DSrtBare>::value == false);
    D_SR_CHECK(is_sorted_container<DSrtPair>::value == false);
    D_SR_CHECK(is_sorted_container<int>::value      == false);
    D_SR_CHECK(is_sorted_container<void>::value     == false);

    // the definition
    D_SR_CHECK(is_sorted_container<DSrtBounds>::value
                   == ( sortedness_of<DSrtBounds>::value
                            == sortedness::sorted ));
    D_SR_CHECK(is_sorted_container<DSrtSet>::value
                   == ( sortedness_of<DSrtSet>::value
                            == sortedness::sorted ));
    D_SR_CHECK(is_sorted_container<DSrtBare>::value
                   == ( sortedness_of<DSrtBare>::value
                            == sortedness::sorted ));

    return ok;
}

/*
tests_is_unsorted_container
  Verifies the predicate naming the three container states that are not sorted.
  Tests the following:
  - each of unordered, monotone and order_dependent qualifies
  - a sorted container does not
  - a NON-container does not either, which is what makes this narrower than a
    plain negation - see tests_is_unsorted_not_complement
  - it equals its definition term by term: a container state, and not sorted
*/
bool tests_is_unsorted_container()
{
    bool ok = true;

    // the three container states that are not sorted
    D_SR_CHECK(is_unsorted_container<DSrtUset>::value    == true);  // unordered
    D_SR_CHECK(is_unsorted_container<DSrtUmap>::value    == true);
    D_SR_CHECK(is_unsorted_container<DSrtKeyed>::value   == true);
    D_SR_CHECK(is_unsorted_container<DSrtSet>::value     == true);  // monotone
    D_SR_CHECK(is_unsorted_container<DSrtMap>::value     == true);
    D_SR_CHECK(is_unsorted_container<DSrtVector>::value  == true);  // order-dep
    D_SR_CHECK(is_unsorted_container<DSrtList>::value    == true);
    D_SR_CHECK(is_unsorted_container<DSrtString>::value  == true);
    D_SR_CHECK(is_unsorted_container<DSrtSequence>::value == true);

    // a sorted container does not
    D_SR_CHECK(is_unsorted_container<DSrtBounds>::value         == false);
    D_SR_CHECK(is_unsorted_container<DSrtInvariantTrue>::value == false);

    // nor does a non-container
    D_SR_CHECK(is_unsorted_container<DSrtBare>::value == false);
    D_SR_CHECK(is_unsorted_container<DSrtPair>::value == false);
    D_SR_CHECK(is_unsorted_container<DSrtKeyedNotIterable>::value == false);
    D_SR_CHECK(is_unsorted_container<int>::value      == false);
    D_SR_CHECK(is_unsorted_container<void>::value     == false);

    // the definition, term by term
    D_SR_CHECK(is_unsorted_container<DSrtVector>::value
                   == (    ( sortedness_of<DSrtVector>::value
                                 != sortedness::non_container )
                        && ( sortedness_of<DSrtVector>::value
                                 != sortedness::sorted ) ));
    D_SR_CHECK(is_unsorted_container<DSrtBounds>::value
                   == (    ( sortedness_of<DSrtBounds>::value
                                 != sortedness::non_container )
                        && ( sortedness_of<DSrtBounds>::value
                                 != sortedness::sorted ) ));
    D_SR_CHECK(is_unsorted_container<DSrtBare>::value
                   == (    ( sortedness_of<DSrtBare>::value
                                 != sortedness::non_container )
                        && ( sortedness_of<DSrtBare>::value
                                 != sortedness::sorted ) ));

    return ok;
}

/*
tests_is_unsorted_not_complement
  Verifies that is_unsorted_container is NOT the negation of
  is_sorted_container, and that the difference is exactly the non_container
  state.

  Both predicates are false for a type that is not a container at all, so the
  two together do not cover the world.  This is deliberate and useful: `is
  unsorted` should mean "is a container, and its type makes no ordering
  promise", not "is anything whatsoever other than a sorted container".  Code
  written as `if (!is_sorted_container<T>)` therefore admits ints and structs,
  where `if (is_unsorted_container<T>)` does not.

  Tests the following:
  - the two predicates are never both true
  - they are both FALSE for a non-container, on several carriers
  - over containers, and only over containers, they do partition
  - the difference between `is_unsorted` and `!is_sorted` is precisely the
    non_container state, shown by exhibiting a type where the two disagree
*/
bool tests_is_unsorted_not_complement()
{
    bool ok = true;

    // never both true
    D_SR_CHECK(!(    is_sorted_container<DSrtBounds>::value
                  && is_unsorted_container<DSrtBounds>::value ));
    D_SR_CHECK(!(    is_sorted_container<DSrtSet>::value
                  && is_unsorted_container<DSrtSet>::value ));
    D_SR_CHECK(!(    is_sorted_container<DSrtBare>::value
                  && is_unsorted_container<DSrtBare>::value ));

    // both false for a non-container
    D_SR_CHECK(   ( is_sorted_container<DSrtBare>::value   == false )
               && ( is_unsorted_container<DSrtBare>::value == false ));
    D_SR_CHECK(   ( is_sorted_container<DSrtPair>::value   == false )
               && ( is_unsorted_container<DSrtPair>::value == false ));
    D_SR_CHECK(   ( is_sorted_container<int>::value   == false )
               && ( is_unsorted_container<int>::value == false ));
    D_SR_CHECK(   ( is_sorted_container<DSrtKeyedNotIterable>::value
                        == false )
               && ( is_unsorted_container<DSrtKeyedNotIterable>::value
                        == false ));

    // over containers they partition
    D_SR_CHECK((    is_sorted_container<DSrtVector>::value
                 || is_unsorted_container<DSrtVector>::value ) == true);
    D_SR_CHECK((    is_sorted_container<DSrtSet>::value
                 || is_unsorted_container<DSrtSet>::value ) == true);
    D_SR_CHECK((    is_sorted_container<DSrtBounds>::value
                 || is_unsorted_container<DSrtBounds>::value ) == true);
    D_SR_CHECK((    is_sorted_container<DSrtUset>::value
                 || is_unsorted_container<DSrtUset>::value ) == true);

    // ... and only over containers
    D_SR_CHECK((    is_sorted_container<DSrtBare>::value
                 || is_unsorted_container<DSrtBare>::value ) == false);

    // where the two spellings disagree
    D_SR_CHECK(is_unsorted_container<DSrtBare>::value
                   != !is_sorted_container<DSrtBare>::value);
    D_SR_CHECK(is_unsorted_container<DSrtVector>::value
                   == !is_sorted_container<DSrtVector>::value);

    return ok;
}

/*
tests_is_monotone_container
  Verifies the predicate naming the state a keyed container reaches when it
  carries a comparator.
  Tests the following:
  - the tree-backed associatives qualify; the hashed ones do not
  - a sequence container never qualifies, whatever it declares - the monotone
    state is reachable only from the unordered arm
  - it equals `sortedness_of == monotone` on every carrier
*/
bool tests_is_monotone_container()
{
    bool ok = true;

    // tree-backed
    D_SR_CHECK(is_monotone_container<DSrtSet>::value      == true);
    D_SR_CHECK(is_monotone_container<DSrtMultiset>::value == true);
    D_SR_CHECK(is_monotone_container<DSrtMap>::value      == true);
    D_SR_CHECK(is_monotone_container<DSrtKeyedCompare>::value == true);

    // hashed
    D_SR_CHECK(is_monotone_container<DSrtUset>::value  == false);
    D_SR_CHECK(is_monotone_container<DSrtUmap>::value  == false);
    D_SR_CHECK(is_monotone_container<DSrtKeyed>::value == false);

    // sequences never reach it, whatever they declare
    D_SR_CHECK(is_monotone_container<DSrtVector>::value == false);
    D_SR_CHECK(is_monotone_container<DSrtSequenceCompare>::value == false);
    D_SR_CHECK(is_monotone_container<DSrtBounds>::value == false);
    D_SR_CHECK(is_monotone_container<DSrtInvariantTrue>::value == false);

    // non-containers
    D_SR_CHECK(is_monotone_container<DSrtBare>::value == false);
    D_SR_CHECK(is_monotone_container<int>::value      == false);
    D_SR_CHECK(is_monotone_container<void>::value     == false);

    // the definition
    D_SR_CHECK(is_monotone_container<DSrtSet>::value
                   == ( sortedness_of<DSrtSet>::value
                            == sortedness::monotone ));
    D_SR_CHECK(is_monotone_container<DSrtUset>::value
                   == ( sortedness_of<DSrtUset>::value
                            == sortedness::monotone ));
    D_SR_CHECK(is_monotone_container<DSrtBounds>::value
                   == ( sortedness_of<DSrtBounds>::value
                            == sortedness::monotone ));

    return ok;
}

/*
tests_admits_sorted_enumeration
  Verifies the union predicate, which is the one most dispatch sites actually
  want: may this container be walked front to back and relied upon to come out
  in comparator order?
  Tests the following:
  - both `sorted` and `monotone` qualify, being the two ways that promise can
    be made
  - `unordered`, `order_dependent` and `non_container` do not
  - it equals the disjunction of its two component predicates on every carrier
  - it is strictly weaker than is_sorted_container, shown by a carrier
    satisfying it without satisfying that
*/
bool tests_admits_sorted_enumeration()
{
    bool ok = true;

    // the two qualifying states
    D_SR_CHECK(admits_sorted_enumeration<DSrtBounds>::value         == true);
    D_SR_CHECK(admits_sorted_enumeration<DSrtInvariantTrue>::value == true);
    D_SR_CHECK(admits_sorted_enumeration<DSrtSet>::value      == true);
    D_SR_CHECK(admits_sorted_enumeration<DSrtMultiset>::value == true);
    D_SR_CHECK(admits_sorted_enumeration<DSrtMap>::value      == true);

    // the three that do not
    D_SR_CHECK(admits_sorted_enumeration<DSrtUset>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<DSrtUmap>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<DSrtVector>::value == false);
    D_SR_CHECK(admits_sorted_enumeration<DSrtList>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<DSrtString>::value == false);
    D_SR_CHECK(admits_sorted_enumeration<DSrtBare>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<DSrtPair>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<int>::value        == false);
    D_SR_CHECK(admits_sorted_enumeration<void>::value       == false);

    // the definition, on a carrier for each disjunct and for neither
    D_SR_CHECK(admits_sorted_enumeration<DSrtSet>::value
                   == (    is_sorted_container<DSrtSet>::value
                        || is_monotone_container<DSrtSet>::value ));
    D_SR_CHECK(admits_sorted_enumeration<DSrtBounds>::value
                   == (    is_sorted_container<DSrtBounds>::value
                        || is_monotone_container<DSrtBounds>::value ));
    D_SR_CHECK(admits_sorted_enumeration<DSrtVector>::value
                   == (    is_sorted_container<DSrtVector>::value
                        || is_monotone_container<DSrtVector>::value ));
    D_SR_CHECK(admits_sorted_enumeration<DSrtBare>::value
                   == (    is_sorted_container<DSrtBare>::value
                        || is_monotone_container<DSrtBare>::value ));

    // strictly weaker than is_sorted_container
    D_SR_CHECK(   ( is_sorted_container<DSrtSet>::value       == false )
               && ( admits_sorted_enumeration<DSrtSet>::value == true ));

    return ok;
}

/*
tests_predicates_monotone_paradox
  Verifies the reading of this axis most likely to be misused.

  A std::set is simultaneously is_unsorted_container AND
  admits_sorted_enumeration.  Read as English that is a contradiction; read as
  the header defines them it is not, because the two answer different
  questions.  is_sorted_container asks whether the type maintains its POSITIONS
  in comparator order - and a set has no positions, so it cannot.
  admits_sorted_enumeration asks whether walking the container yields a sorted
  sequence - and a set does, by construction.  monotone is the state that
  separates them: not sorted, yet sorted-enumerable.

  The practical consequence: code that wants to skip a sorting pass must ask
  admits_sorted_enumeration, NOT `!is_unsorted_container`.  The latter
  needlessly re-sorts every std::set and std::map it is handed.

  Tests the following:
  - a std::set is unsorted and sorted-enumerable at once, and so is a std::map
  - the state that reconciles them is monotone
  - the two predicates disagree on exactly the monotone carriers, and agree
    everywhere else - shown across a carrier for each of the five states
  - a genuinely sorted carrier is the mirror image: not unsorted, and
    enumerable
*/
bool tests_predicates_monotone_paradox()
{
    bool ok = true;

    // both true at once
    D_SR_CHECK(   ( is_unsorted_container<DSrtSet>::value      == true )
               && ( admits_sorted_enumeration<DSrtSet>::value  == true ));
    D_SR_CHECK(   ( is_unsorted_container<DSrtMap>::value      == true )
               && ( admits_sorted_enumeration<DSrtMap>::value  == true ));
    D_SR_CHECK(   ( is_unsorted_container<DSrtMultiset>::value == true )
               && ( admits_sorted_enumeration<DSrtMultiset>::value == true ));

    // ... and the state that reconciles them
    D_SR_CHECK(sortedness_of<DSrtSet>::value == sortedness::monotone);
    D_SR_CHECK(is_monotone_container<DSrtSet>::value  == true);
    D_SR_CHECK(is_sorted_container<DSrtSet>::value    == false);

    // where the two predicates disagree, and where they do not
    D_SR_CHECK(   ( is_unsorted_container<DSrtSet>::value
                        != admits_sorted_enumeration<DSrtSet>::value )
                   == false);                          // monotone: both true
    D_SR_CHECK(   ( is_unsorted_container<DSrtUset>::value     == true )
               && ( admits_sorted_enumeration<DSrtUset>::value == false ));
    D_SR_CHECK(   ( is_unsorted_container<DSrtVector>::value     == true )
               && ( admits_sorted_enumeration<DSrtVector>::value == false ));
    D_SR_CHECK(   ( is_unsorted_container<DSrtBounds>::value     == false )
               && ( admits_sorted_enumeration<DSrtBounds>::value == true ));
    D_SR_CHECK(   ( is_unsorted_container<DSrtBare>::value     == false )
               && ( admits_sorted_enumeration<DSrtBare>::value == false ));

    // the negation a dispatch site must NOT use
    D_SR_CHECK(!is_unsorted_container<DSrtSet>::value
                   != admits_sorted_enumeration<DSrtSet>::value);

    return ok;
}

/*
tests_predicates_partition
  Verifies the whole predicate table at once - for each of the five states, the
  exact tuple of four answers it produces.  Any change to a predicate's
  definition alters at least one row here, so this is the test that would not
  survive an accidental edit.

    state             sorted  unsorted  monotone  enumerable
    ----------------  ------  --------  --------  ----------
    non_container       F        F         F          F
    unordered           F        T         F          F
    monotone            F        T         T          T
    order_dependent     F        T         F          F
    sorted              T        F         F          T

  Tests the following:
  - each of the five rows, on a carrier known to occupy that state
  - a second, independent carrier for each row, so a row cannot be satisfied by
    one fixture's accident
  - at most one of is_sorted_container and is_monotone_container is ever true,
    the two naming disjoint states
*/
bool tests_predicates_partition()
{
    bool ok = true;

#define D_SR_ROW(T, S, U, M, E)                                               \
    do                                                                        \
    {                                                                         \
        D_SR_CHECK(is_sorted_container<T>::value       == (S));               \
        D_SR_CHECK(is_unsorted_container<T>::value     == (U));               \
        D_SR_CHECK(is_monotone_container<T>::value     == (M));               \
        D_SR_CHECK(admits_sorted_enumeration<T>::value == (E));               \
    } while (0)

    // non_container
    D_SR_ROW(DSrtBare,               false, false, false, false);
    D_SR_ROW(DSrtKeyedNotIterable, false, false, false, false);

    // unordered
    D_SR_ROW(DSrtUset,  false, true,  false, false);
    D_SR_ROW(DSrtKeyed, false, true,  false, false);

    // monotone
    D_SR_ROW(DSrtSet,           false, true,  true,  true);
    D_SR_ROW(DSrtKeyedCompare, false, true,  true,  true);

    // order_dependent
    D_SR_ROW(DSrtVector,   false, true,  false, false);
    D_SR_ROW(DSrtSequence, false, true,  false, false);

    // sorted
    D_SR_ROW(DSrtBounds,         true,  false, false, true);
    D_SR_ROW(DSrtInvariantTrue, true,  false, false, true);

#undef D_SR_ROW

    // sorted and monotone name disjoint states
    D_SR_CHECK(!(    is_sorted_container<DSrtSet>::value
                  && is_monotone_container<DSrtSet>::value ));
    D_SR_CHECK(!(    is_sorted_container<DSrtBounds>::value
                  && is_monotone_container<DSrtBounds>::value ));
    D_SR_CHECK(!(    is_sorted_container<DSrtVector>::value
                  && is_monotone_container<DSrtVector>::value ));

    return ok;
}

/*
tests_predicates_cv_ref
  Verifies that all four predicates honour the clean_t contract.
  Tests the following:
  - each predicate is identical across all seven cv- and reference-qualified
    spellings, on a positive and a negative carrier
  - the volatile spellings in particular
*/
bool tests_predicates_cv_ref()
{
    bool ok = true;

    D_SR_CV_REF_AGREE(is_sorted_container, DSrtBounds, true);
    D_SR_CV_REF_AGREE(is_sorted_container, DSrtSet,    false);
    D_SR_CV_REF_AGREE(is_sorted_container, DSrtBare,   false);

    D_SR_CV_REF_AGREE(is_unsorted_container, DSrtVector, true);
    D_SR_CV_REF_AGREE(is_unsorted_container, DSrtSet,    true);
    D_SR_CV_REF_AGREE(is_unsorted_container, DSrtBounds, false);
    D_SR_CV_REF_AGREE(is_unsorted_container, DSrtBare,   false);

    D_SR_CV_REF_AGREE(is_monotone_container, DSrtMap,    true);
    D_SR_CV_REF_AGREE(is_monotone_container, DSrtUmap,   false);

    D_SR_CV_REF_AGREE(admits_sorted_enumeration, DSrtSet,    true);
    D_SR_CV_REF_AGREE(admits_sorted_enumeration, DSrtBounds, true);
    D_SR_CV_REF_AGREE(admits_sorted_enumeration, DSrtVector, false);

    // spelled out on one carrier of each predicate
    D_SR_CHECK(is_sorted_container<volatile DSrtBounds>::value      == true);
    D_SR_CHECK(is_unsorted_container<volatile DSrtSet>::value       == true);
    D_SR_CHECK(is_monotone_container<const volatile DSrtMap&>::value
                   == true);
    D_SR_CHECK(admits_sorted_enumeration<volatile DSrtSet>::value   == true);

    return ok;
}

/*
tests_predicates_companions
  Verifies the `_v` companions of all four predicates.
  Tests the following:
  - each companion agrees with its trait's ::value on a positive and a negative
    carrier
  - each is usable in a constant expression
  - gated on variable-template support, since below C++14 the engine emits
    nothing and ::value remains the portable spelling
*/
bool tests_predicates_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_SR_STATIC_CHECK(is_sorted_container_v<DSrtBounds>       == true);
    D_SR_STATIC_CHECK(is_unsorted_container_v<DSrtVector>     == true);
    D_SR_STATIC_CHECK(is_monotone_container_v<DSrtSet>        == true);
    D_SR_STATIC_CHECK(admits_sorted_enumeration_v<DSrtSet>    == true);
    D_SR_STATIC_CHECK(admits_sorted_enumeration_v<DSrtVector> == false);

    D_SR_CHECK(is_sorted_container_v<DSrtBounds>
                   == is_sorted_container<DSrtBounds>::value);
    D_SR_CHECK(is_sorted_container_v<DSrtSet>
                   == is_sorted_container<DSrtSet>::value);
    D_SR_CHECK(is_sorted_container_v<DSrtBare>
                   == is_sorted_container<DSrtBare>::value);

    D_SR_CHECK(is_unsorted_container_v<DSrtVector>
                   == is_unsorted_container<DSrtVector>::value);
    D_SR_CHECK(is_unsorted_container_v<DSrtBounds>
                   == is_unsorted_container<DSrtBounds>::value);
    D_SR_CHECK(is_unsorted_container_v<DSrtBare>
                   == is_unsorted_container<DSrtBare>::value);

    D_SR_CHECK(is_monotone_container_v<DSrtMap>
                   == is_monotone_container<DSrtMap>::value);
    D_SR_CHECK(is_monotone_container_v<DSrtUmap>
                   == is_monotone_container<DSrtUmap>::value);

    D_SR_CHECK(admits_sorted_enumeration_v<DSrtSet>
                   == admits_sorted_enumeration<DSrtSet>::value);
    D_SR_CHECK(admits_sorted_enumeration_v<DSrtBounds>
                   == admits_sorted_enumeration<DSrtBounds>::value);
    D_SR_CHECK(admits_sorted_enumeration_v<DSrtUset>
                   == admits_sorted_enumeration<DSrtUset>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
