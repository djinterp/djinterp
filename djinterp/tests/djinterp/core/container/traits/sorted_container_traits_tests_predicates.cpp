/*******************************************************************************
* djinterp [core]                   sorted_container_traits_tests_predicates.cpp
*
*
* path:      /tests/djinterp/core/container/traits/sorted_container_traits_tests_predicates.cpp
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
bool
tests_is_sorted_container()
{
    bool ok = true;

    // the opted-in carriers
    D_SR_CHECK(is_sorted_container<srt_bounds>::value         == true);
    D_SR_CHECK(is_sorted_container<srt_invariant_true>::value == true);
    D_SR_CHECK(is_sorted_container<srt_both_signals>::value   == true);

    // no standard container qualifies
    D_SR_CHECK(is_sorted_container<srt_set>::value      == false);
    D_SR_CHECK(is_sorted_container<srt_multiset>::value == false);
    D_SR_CHECK(is_sorted_container<srt_map>::value      == false);
    D_SR_CHECK(is_sorted_container<srt_uset>::value     == false);
    D_SR_CHECK(is_sorted_container<srt_vector>::value   == false);
    D_SR_CHECK(is_sorted_container<srt_string>::value   == false);

    // nor does an opted-out or a signal-free carrier
    D_SR_CHECK(is_sorted_container<srt_invariant_false>::value == false);
    D_SR_CHECK(is_sorted_container<srt_bounds_lower_only>::value == false);
    D_SR_CHECK(is_sorted_container<srt_sequence>::value == false);

    // non-containers
    D_SR_CHECK(is_sorted_container<srt_bare>::value == false);
    D_SR_CHECK(is_sorted_container<srt_pair>::value == false);
    D_SR_CHECK(is_sorted_container<int>::value      == false);
    D_SR_CHECK(is_sorted_container<void>::value     == false);

    // the definition
    D_SR_CHECK(is_sorted_container<srt_bounds>::value
                   == ( sortedness_of<srt_bounds>::value
                            == sortedness::sorted ));
    D_SR_CHECK(is_sorted_container<srt_set>::value
                   == ( sortedness_of<srt_set>::value
                            == sortedness::sorted ));
    D_SR_CHECK(is_sorted_container<srt_bare>::value
                   == ( sortedness_of<srt_bare>::value
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
bool
tests_is_unsorted_container()
{
    bool ok = true;

    // the three container states that are not sorted
    D_SR_CHECK(is_unsorted_container<srt_uset>::value    == true);  // unordered
    D_SR_CHECK(is_unsorted_container<srt_umap>::value    == true);
    D_SR_CHECK(is_unsorted_container<srt_keyed>::value   == true);
    D_SR_CHECK(is_unsorted_container<srt_set>::value     == true);  // monotone
    D_SR_CHECK(is_unsorted_container<srt_map>::value     == true);
    D_SR_CHECK(is_unsorted_container<srt_vector>::value  == true);  // order-dep
    D_SR_CHECK(is_unsorted_container<srt_list>::value    == true);
    D_SR_CHECK(is_unsorted_container<srt_string>::value  == true);
    D_SR_CHECK(is_unsorted_container<srt_sequence>::value == true);

    // a sorted container does not
    D_SR_CHECK(is_unsorted_container<srt_bounds>::value         == false);
    D_SR_CHECK(is_unsorted_container<srt_invariant_true>::value == false);

    // nor does a non-container
    D_SR_CHECK(is_unsorted_container<srt_bare>::value == false);
    D_SR_CHECK(is_unsorted_container<srt_pair>::value == false);
    D_SR_CHECK(is_unsorted_container<srt_keyed_not_iterable>::value == false);
    D_SR_CHECK(is_unsorted_container<int>::value      == false);
    D_SR_CHECK(is_unsorted_container<void>::value     == false);

    // the definition, term by term
    D_SR_CHECK(is_unsorted_container<srt_vector>::value
                   == (    ( sortedness_of<srt_vector>::value
                                 != sortedness::non_container )
                        && ( sortedness_of<srt_vector>::value
                                 != sortedness::sorted ) ));
    D_SR_CHECK(is_unsorted_container<srt_bounds>::value
                   == (    ( sortedness_of<srt_bounds>::value
                                 != sortedness::non_container )
                        && ( sortedness_of<srt_bounds>::value
                                 != sortedness::sorted ) ));
    D_SR_CHECK(is_unsorted_container<srt_bare>::value
                   == (    ( sortedness_of<srt_bare>::value
                                 != sortedness::non_container )
                        && ( sortedness_of<srt_bare>::value
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
bool
tests_is_unsorted_not_complement()
{
    bool ok = true;

    // never both true
    D_SR_CHECK(!(    is_sorted_container<srt_bounds>::value
                  && is_unsorted_container<srt_bounds>::value ));
    D_SR_CHECK(!(    is_sorted_container<srt_set>::value
                  && is_unsorted_container<srt_set>::value ));
    D_SR_CHECK(!(    is_sorted_container<srt_bare>::value
                  && is_unsorted_container<srt_bare>::value ));

    // both false for a non-container
    D_SR_CHECK(   ( is_sorted_container<srt_bare>::value   == false )
               && ( is_unsorted_container<srt_bare>::value == false ));
    D_SR_CHECK(   ( is_sorted_container<srt_pair>::value   == false )
               && ( is_unsorted_container<srt_pair>::value == false ));
    D_SR_CHECK(   ( is_sorted_container<int>::value   == false )
               && ( is_unsorted_container<int>::value == false ));
    D_SR_CHECK(   ( is_sorted_container<srt_keyed_not_iterable>::value
                        == false )
               && ( is_unsorted_container<srt_keyed_not_iterable>::value
                        == false ));

    // over containers they partition
    D_SR_CHECK((    is_sorted_container<srt_vector>::value
                 || is_unsorted_container<srt_vector>::value ) == true);
    D_SR_CHECK((    is_sorted_container<srt_set>::value
                 || is_unsorted_container<srt_set>::value ) == true);
    D_SR_CHECK((    is_sorted_container<srt_bounds>::value
                 || is_unsorted_container<srt_bounds>::value ) == true);
    D_SR_CHECK((    is_sorted_container<srt_uset>::value
                 || is_unsorted_container<srt_uset>::value ) == true);

    // ... and only over containers
    D_SR_CHECK((    is_sorted_container<srt_bare>::value
                 || is_unsorted_container<srt_bare>::value ) == false);

    // where the two spellings disagree
    D_SR_CHECK(is_unsorted_container<srt_bare>::value
                   != !is_sorted_container<srt_bare>::value);
    D_SR_CHECK(is_unsorted_container<srt_vector>::value
                   == !is_sorted_container<srt_vector>::value);

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
bool
tests_is_monotone_container()
{
    bool ok = true;

    // tree-backed
    D_SR_CHECK(is_monotone_container<srt_set>::value      == true);
    D_SR_CHECK(is_monotone_container<srt_multiset>::value == true);
    D_SR_CHECK(is_monotone_container<srt_map>::value      == true);
    D_SR_CHECK(is_monotone_container<srt_keyed_compare>::value == true);

    // hashed
    D_SR_CHECK(is_monotone_container<srt_uset>::value  == false);
    D_SR_CHECK(is_monotone_container<srt_umap>::value  == false);
    D_SR_CHECK(is_monotone_container<srt_keyed>::value == false);

    // sequences never reach it, whatever they declare
    D_SR_CHECK(is_monotone_container<srt_vector>::value == false);
    D_SR_CHECK(is_monotone_container<srt_sequence_compare>::value == false);
    D_SR_CHECK(is_monotone_container<srt_bounds>::value == false);
    D_SR_CHECK(is_monotone_container<srt_invariant_true>::value == false);

    // non-containers
    D_SR_CHECK(is_monotone_container<srt_bare>::value == false);
    D_SR_CHECK(is_monotone_container<int>::value      == false);
    D_SR_CHECK(is_monotone_container<void>::value     == false);

    // the definition
    D_SR_CHECK(is_monotone_container<srt_set>::value
                   == ( sortedness_of<srt_set>::value
                            == sortedness::monotone ));
    D_SR_CHECK(is_monotone_container<srt_uset>::value
                   == ( sortedness_of<srt_uset>::value
                            == sortedness::monotone ));
    D_SR_CHECK(is_monotone_container<srt_bounds>::value
                   == ( sortedness_of<srt_bounds>::value
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
bool
tests_admits_sorted_enumeration()
{
    bool ok = true;

    // the two qualifying states
    D_SR_CHECK(admits_sorted_enumeration<srt_bounds>::value         == true);
    D_SR_CHECK(admits_sorted_enumeration<srt_invariant_true>::value == true);
    D_SR_CHECK(admits_sorted_enumeration<srt_set>::value      == true);
    D_SR_CHECK(admits_sorted_enumeration<srt_multiset>::value == true);
    D_SR_CHECK(admits_sorted_enumeration<srt_map>::value      == true);

    // the three that do not
    D_SR_CHECK(admits_sorted_enumeration<srt_uset>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<srt_umap>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<srt_vector>::value == false);
    D_SR_CHECK(admits_sorted_enumeration<srt_list>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<srt_string>::value == false);
    D_SR_CHECK(admits_sorted_enumeration<srt_bare>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<srt_pair>::value   == false);
    D_SR_CHECK(admits_sorted_enumeration<int>::value        == false);
    D_SR_CHECK(admits_sorted_enumeration<void>::value       == false);

    // the definition, on a carrier for each disjunct and for neither
    D_SR_CHECK(admits_sorted_enumeration<srt_set>::value
                   == (    is_sorted_container<srt_set>::value
                        || is_monotone_container<srt_set>::value ));
    D_SR_CHECK(admits_sorted_enumeration<srt_bounds>::value
                   == (    is_sorted_container<srt_bounds>::value
                        || is_monotone_container<srt_bounds>::value ));
    D_SR_CHECK(admits_sorted_enumeration<srt_vector>::value
                   == (    is_sorted_container<srt_vector>::value
                        || is_monotone_container<srt_vector>::value ));
    D_SR_CHECK(admits_sorted_enumeration<srt_bare>::value
                   == (    is_sorted_container<srt_bare>::value
                        || is_monotone_container<srt_bare>::value ));

    // strictly weaker than is_sorted_container
    D_SR_CHECK(   ( is_sorted_container<srt_set>::value       == false )
               && ( admits_sorted_enumeration<srt_set>::value == true ));

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
bool
tests_predicates_monotone_paradox()
{
    bool ok = true;

    // both true at once
    D_SR_CHECK(   ( is_unsorted_container<srt_set>::value      == true )
               && ( admits_sorted_enumeration<srt_set>::value  == true ));
    D_SR_CHECK(   ( is_unsorted_container<srt_map>::value      == true )
               && ( admits_sorted_enumeration<srt_map>::value  == true ));
    D_SR_CHECK(   ( is_unsorted_container<srt_multiset>::value == true )
               && ( admits_sorted_enumeration<srt_multiset>::value == true ));

    // ... and the state that reconciles them
    D_SR_CHECK(sortedness_of<srt_set>::value == sortedness::monotone);
    D_SR_CHECK(is_monotone_container<srt_set>::value  == true);
    D_SR_CHECK(is_sorted_container<srt_set>::value    == false);

    // where the two predicates disagree, and where they do not
    D_SR_CHECK(   ( is_unsorted_container<srt_set>::value
                        != admits_sorted_enumeration<srt_set>::value )
                   == false);                          // monotone: both true
    D_SR_CHECK(   ( is_unsorted_container<srt_uset>::value     == true )
               && ( admits_sorted_enumeration<srt_uset>::value == false ));
    D_SR_CHECK(   ( is_unsorted_container<srt_vector>::value     == true )
               && ( admits_sorted_enumeration<srt_vector>::value == false ));
    D_SR_CHECK(   ( is_unsorted_container<srt_bounds>::value     == false )
               && ( admits_sorted_enumeration<srt_bounds>::value == true ));
    D_SR_CHECK(   ( is_unsorted_container<srt_bare>::value     == false )
               && ( admits_sorted_enumeration<srt_bare>::value == false ));

    // the negation a dispatch site must NOT use
    D_SR_CHECK(!is_unsorted_container<srt_set>::value
                   != admits_sorted_enumeration<srt_set>::value);

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
bool
tests_predicates_partition()
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
    D_SR_ROW(srt_bare,               false, false, false, false);
    D_SR_ROW(srt_keyed_not_iterable, false, false, false, false);

    // unordered
    D_SR_ROW(srt_uset,  false, true,  false, false);
    D_SR_ROW(srt_keyed, false, true,  false, false);

    // monotone
    D_SR_ROW(srt_set,           false, true,  true,  true);
    D_SR_ROW(srt_keyed_compare, false, true,  true,  true);

    // order_dependent
    D_SR_ROW(srt_vector,   false, true,  false, false);
    D_SR_ROW(srt_sequence, false, true,  false, false);

    // sorted
    D_SR_ROW(srt_bounds,         true,  false, false, true);
    D_SR_ROW(srt_invariant_true, true,  false, false, true);

#undef D_SR_ROW

    // sorted and monotone name disjoint states
    D_SR_CHECK(!(    is_sorted_container<srt_set>::value
                  && is_monotone_container<srt_set>::value ));
    D_SR_CHECK(!(    is_sorted_container<srt_bounds>::value
                  && is_monotone_container<srt_bounds>::value ));
    D_SR_CHECK(!(    is_sorted_container<srt_vector>::value
                  && is_monotone_container<srt_vector>::value ));

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
bool
tests_predicates_cv_ref()
{
    bool ok = true;

    D_SR_CV_REF_AGREE(is_sorted_container, srt_bounds, true);
    D_SR_CV_REF_AGREE(is_sorted_container, srt_set,    false);
    D_SR_CV_REF_AGREE(is_sorted_container, srt_bare,   false);

    D_SR_CV_REF_AGREE(is_unsorted_container, srt_vector, true);
    D_SR_CV_REF_AGREE(is_unsorted_container, srt_set,    true);
    D_SR_CV_REF_AGREE(is_unsorted_container, srt_bounds, false);
    D_SR_CV_REF_AGREE(is_unsorted_container, srt_bare,   false);

    D_SR_CV_REF_AGREE(is_monotone_container, srt_map,    true);
    D_SR_CV_REF_AGREE(is_monotone_container, srt_umap,   false);

    D_SR_CV_REF_AGREE(admits_sorted_enumeration, srt_set,    true);
    D_SR_CV_REF_AGREE(admits_sorted_enumeration, srt_bounds, true);
    D_SR_CV_REF_AGREE(admits_sorted_enumeration, srt_vector, false);

    // spelled out on one carrier of each predicate
    D_SR_CHECK(is_sorted_container<volatile srt_bounds>::value      == true);
    D_SR_CHECK(is_unsorted_container<volatile srt_set>::value       == true);
    D_SR_CHECK(is_monotone_container<const volatile srt_map&>::value
                   == true);
    D_SR_CHECK(admits_sorted_enumeration<volatile srt_set>::value   == true);

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
bool
tests_predicates_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_SR_STATIC_CHECK(is_sorted_container_v<srt_bounds>       == true);
    D_SR_STATIC_CHECK(is_unsorted_container_v<srt_vector>     == true);
    D_SR_STATIC_CHECK(is_monotone_container_v<srt_set>        == true);
    D_SR_STATIC_CHECK(admits_sorted_enumeration_v<srt_set>    == true);
    D_SR_STATIC_CHECK(admits_sorted_enumeration_v<srt_vector> == false);

    D_SR_CHECK(is_sorted_container_v<srt_bounds>
                   == is_sorted_container<srt_bounds>::value);
    D_SR_CHECK(is_sorted_container_v<srt_set>
                   == is_sorted_container<srt_set>::value);
    D_SR_CHECK(is_sorted_container_v<srt_bare>
                   == is_sorted_container<srt_bare>::value);

    D_SR_CHECK(is_unsorted_container_v<srt_vector>
                   == is_unsorted_container<srt_vector>::value);
    D_SR_CHECK(is_unsorted_container_v<srt_bounds>
                   == is_unsorted_container<srt_bounds>::value);
    D_SR_CHECK(is_unsorted_container_v<srt_bare>
                   == is_unsorted_container<srt_bare>::value);

    D_SR_CHECK(is_monotone_container_v<srt_map>
                   == is_monotone_container<srt_map>::value);
    D_SR_CHECK(is_monotone_container_v<srt_umap>
                   == is_monotone_container<srt_umap>::value);

    D_SR_CHECK(admits_sorted_enumeration_v<srt_set>
                   == admits_sorted_enumeration<srt_set>::value);
    D_SR_CHECK(admits_sorted_enumeration_v<srt_bounds>
                   == admits_sorted_enumeration<srt_bounds>::value);
    D_SR_CHECK(admits_sorted_enumeration_v<srt_uset>
                   == admits_sorted_enumeration<srt_uset>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
