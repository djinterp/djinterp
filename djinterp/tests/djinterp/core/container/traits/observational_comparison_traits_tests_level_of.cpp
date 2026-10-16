/*******************************************************************************
* djinterp [core]             observational_comparison_traits_tests_level_of.cpp
*
*
* path:      /tests/djinterp/core/container/traits/observational_comparison_traits_tests_level_of.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "observational_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. The level a single type's interface resolves
// ===========================================================================
//   A cascade from finest to coarsest, which is the maximum over the
// completeness levels of the operations present - a finer operation subsumes
// the coarser, so reading the sequence also reads the bag, the set and the
// size.  Each rung has a fixture carrying the minimum interface that reaches
// it, and each STEP of the cascade has a fixture that satisfies two rungs so
// the order is pinned rather than assumed.


/*
tests_observational_level_of_cascade
  Verifies that each rung is reachable, on a fixture carrying the minimum
  interface for it.
  Tests the following:
  - all six outcomes: str, seq, bag, set, size and none
  - seq is reachable by EITHER route - a const iteration or a flat positional
    read - so the two are genuinely alternatives, not a conjunction
  - str is reachable without iteration and without size, so it rests on
    positional access and shape alone
  - none is what a type with no inspection surface resolves, and is also what a
    type whose whole surface is non-const resolves
*/
bool
tests_observational_level_of_cascade()
{
    bool ok = true;

    // str - positional access into a nested shape
    D_OC_CHECK(observational_level_of<oct_hierarchical_indexed>::value
                   == observational_level::str);

    // seq - by iteration
    D_OC_CHECK(observational_level_of<oct_iterable_only>::value
                   == observational_level::seq);

    // seq - by a flat positional read, either operand
    D_OC_CHECK(observational_level_of<oct_subscript_only>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_at_only>::value
                   == observational_level::seq);

    // bag - per-class counts
    D_OC_CHECK(observational_level_of<oct_count_only>::value
                   == observational_level::bag);

    // set - membership
    D_OC_CHECK(observational_level_of<oct_contains_only>::value
                   == observational_level::set);

    // size - equal size only
    D_OC_CHECK(observational_level_of<oct_size_only>::value
                   == observational_level::size);

    // none
    D_OC_CHECK(observational_level_of<oct_bare>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<int>::value
                   == observational_level::none);
    D_OC_CHECK(observational_level_of<void>::value
                   == observational_level::none);

    // str needs neither iteration nor size
    D_OC_CHECK(   ( is_iterable_container<oct_hierarchical_indexed>::value
                        == false )
               && ( has_size_member<oct_hierarchical_indexed>::value
                        == false )
               && ( observational_level_of<oct_hierarchical_indexed>::value
                        == observational_level::str ));

    // a wholly non-const surface observes nothing
    D_OC_CHECK(observational_level_of<oct_nonconst_subscript>::value
                   == observational_level::none);

    return ok;
}

/*
tests_observational_level_of_precedence
  Verifies the ORDER of the cascade, on fixtures that satisfy two rungs at
  once.  Each check names the step it pins, so a reordering of the cascade
  fails at the step that moved.
  Tests the following:
  - str beats seq: indexed access into a shape wins over the positional read it
    would otherwise supply
  - seq beats bag: iteration wins over count
  - bag beats set: count wins over contains
  - set beats size: contains wins over size
  - str requires BOTH conjuncts - shape without positional access falls to seq,
    and positional access without shape falls to seq, so neither alone reaches
    the finest rung
  - the shape signal contributes NOTHING on its own: a hierarchical container
    with neither positional access nor iteration lands wherever its remaining
    operations put it
  - the level equals the cascade re-derived term by term, on a carrier that
    lights every probe
*/
bool
tests_observational_level_of_precedence()
{
    bool ok = true;

    // str beats seq
    D_OC_CHECK(   ( has_indexed_access<oct_nested>::value        == true )
               && ( is_iterable_container<oct_nested>::value     == true )
               && ( is_hierarchical_container<oct_nested>::value == true )
               && ( observational_level_of<oct_nested>::value
                        == observational_level::str ));

    // seq beats bag
    D_OC_CHECK(   ( is_iterable_container<oct_iterable_and_count>::value
                        == true )
               && ( has_count_member<oct_iterable_and_count>::value == true )
               && ( observational_level_of<oct_iterable_and_count>::value
                        == observational_level::seq ));

    // bag beats set
    D_OC_CHECK(   ( has_count_member<oct_count_and_contains>::value == true )
               && ( has_contains_member<oct_count_and_contains>::value
                        == true )
               && ( observational_level_of<oct_count_and_contains>::value
                        == observational_level::bag ));

    // set beats size
    D_OC_CHECK(   ( has_contains_member<oct_contains_and_size>::value == true )
               && ( has_size_member<oct_contains_and_size>::value     == true )
               && ( observational_level_of<oct_contains_and_size>::value
                        == observational_level::set ));

    // str needs BOTH conjuncts - shape without positional access
    D_OC_CHECK(   ( is_hierarchical_container<oct_hierarchical_iterable>::value
                        == true )
               && ( has_indexed_access<oct_hierarchical_iterable>::value
                        == false )
               && ( observational_level_of<oct_hierarchical_iterable>::value
                        == observational_level::seq ));

    // ... and positional access without shape
    D_OC_CHECK(   ( has_indexed_access<oct_indexed_flat>::value == true )
               && ( is_hierarchical_container<oct_indexed_flat>::value
                        == false )
               && ( observational_level_of<oct_indexed_flat>::value
                        == observational_level::seq ));

    // the shape signal alone contributes nothing
    D_OC_CHECK(   ( is_hierarchical_container<oct_hierarchical_count>::value
                        == true )
               && ( observational_level_of<oct_hierarchical_count>::value
                        == observational_level::bag ));

    // the cascade, re-derived term by term on a carrier lighting every probe
    D_OC_CHECK(observational_level_of<oct_full_interface>::value
                   == (    (    has_indexed_access<oct_full_interface>::value
                             && is_hierarchical_container<
                                    oct_full_interface>::value )
                             ? observational_level::str
                       : (    is_iterable_container<oct_full_interface>::value
                           || has_indexed_access<oct_full_interface>::value )
                             ? observational_level::seq
                       : ( has_count_member<oct_full_interface>::value )
                             ? observational_level::bag
                       : ( has_contains_member<oct_full_interface>::value )
                             ? observational_level::set
                       : ( has_size_member<oct_full_interface>::value )
                             ? observational_level::size
                       :       observational_level::none ));

    // ... and on a carrier lighting only the coarsest
    D_OC_CHECK(observational_level_of<oct_size_only>::value
                   == (    (    has_indexed_access<oct_size_only>::value
                             && is_hierarchical_container<
                                    oct_size_only>::value )
                             ? observational_level::str
                       : (    is_iterable_container<oct_size_only>::value
                           || has_indexed_access<oct_size_only>::value )
                             ? observational_level::seq
                       : ( has_count_member<oct_size_only>::value )
                             ? observational_level::bag
                       : ( has_contains_member<oct_size_only>::value )
                             ? observational_level::set
                       : ( has_size_member<oct_size_only>::value )
                             ? observational_level::size
                       :       observational_level::none ));

    return ok;
}

/*
tests_observational_level_of_std_types
  Verifies the levels the standard containers resolve, which is where the
  header's claims meet real interfaces.
  Tests the following:
  - the sequence containers resolve seq, whether by iteration, by subscript, or
    by both
  - a nested sequence container resolves str, the depth heuristic supplying the
    shape without any tag
  - the associative containers resolve seq, not set or bag - see the dedicated
    gap test below
  - std::forward_list resolves seq DESPITE having no size(), so the rungs are
    not cumulative in the interface, only in what they decide
  - a raw array resolves seq through the built-in subscript, with no members at
    all
  - std::pair resolves none: it is a product, not a container, and exposes no
    inspection surface the header recognises
*/
bool
tests_observational_level_of_std_types()
{
    bool ok = true;

    // sequence containers
    D_OC_CHECK(observational_level_of<oct_vector>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_deque>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_array>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_string>::value
                   == observational_level::seq);

    // nesting reaches str with no tag, by the depth heuristic
    D_OC_CHECK(observational_level_of<oct_nested>::value
                   == observational_level::str);

    // associative containers
    D_OC_CHECK(observational_level_of<oct_set>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_multiset>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_map_int>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<oct_map_string>::value
                   == observational_level::seq);

    // seq without size()
    D_OC_CHECK(   ( has_size_member<oct_flist>::value == false )
               && ( observational_level_of<oct_flist>::value
                        == observational_level::seq ));

    // seq with no members at all
    D_OC_CHECK(   ( has_size_member<oct_raw_array>::value == false )
               && ( is_iterable_container<oct_raw_array>::value == false )
               && ( has_subscript_index<oct_raw_array>::value == true )
               && ( observational_level_of<oct_raw_array>::value
                        == observational_level::seq ));

    // a product type observes nothing
    D_OC_CHECK(observational_level_of<oct_pair>::value
                   == observational_level::none);

    return ok;
}

/*
tests_observational_level_of_set_gap
  Verifies the gap this header exists to describe: the distance between a
  container's NATIVE content level and what its interface OBSERVATIONALLY
  resolves.

  A std::set natively compares at =set - membership is the whole of its
  content.  But its interface exposes a const iteration, and iterating a set
  yields its canonical order, so the interface resolves seq, two rungs finer
  than the container's own notion of content.  Anything comparing two sets
  through this header may therefore use ordered comparison, and any pairing of
  a set with a coarser interface falls back accordingly.

  Tests the following:
  - a std::set resolves seq, and does so BY ITERATION rather than by any of the
    membership operations it also has
  - it resolves strictly finer than the set rung, even though contains() and
    count() are both present and both would have sufficed for a coarser answer
  - the same holds for a multiset, whose native level is bag
  - the gap is a property of the interface, not of the container: a fixture
    with the same membership operations but NO iteration resolves set, which is
    the answer a set would give if its order were not readable
*/
bool
tests_observational_level_of_set_gap()
{
    bool ok = true;

    // the set resolves seq, by iteration
    D_OC_CHECK(observational_level_of<oct_set>::value
                   == observational_level::seq);
    D_OC_CHECK(is_iterable_container<oct_set>::value == true);
    D_OC_CHECK(has_indexed_access<oct_set>::value    == false);

    // strictly finer than the rung its membership operations would give
    D_OC_CHECK(observational_level_subsumes(
                   observational_level_of<oct_set>::value,
                   observational_level::set) == true);
    D_OC_CHECK(observational_level_of<oct_set>::value
                   != observational_level::set);
    D_OC_CHECK(observational_level_rank(
                   observational_level_of<oct_set>::value)
                   > observational_level_rank(observational_level::set));

    // ... and those operations really are present, so the finer answer is a
    // choice of the cascade rather than the absence of an alternative
    D_OC_CHECK(has_count_member<oct_set>::value == true);

    // a multiset, whose native level is bag, likewise resolves seq
    D_OC_CHECK(observational_level_of<oct_multiset>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_rank(
                   observational_level_of<oct_multiset>::value)
                   > observational_level_rank(observational_level::bag));

    // the gap belongs to the INTERFACE: remove the iteration and the same
    // membership operations resolve set
    D_OC_CHECK(   ( has_contains_member<oct_contains_only>::value == true )
               && ( is_iterable_container<oct_contains_only>::value == false )
               && ( observational_level_of<oct_contains_only>::value
                        == observational_level::set ));

    // and with counts but no iteration, bag
    D_OC_CHECK(   ( has_count_member<oct_count_only>::value == true )
               && ( is_iterable_container<oct_count_only>::value == false )
               && ( observational_level_of<oct_count_only>::value
                        == observational_level::bag ));

    return ok;
}

/*
tests_observational_level_of_cv_ref
  Verifies that the trait honours the clean_t contract.

  Worth recording: the invariance is enforced TWICE over.  This trait writes
  clean_t into its own clean_type alias, and every trait it then consults -
  has_indexed_access, is_iterable_container, has_count_member,
  has_contains_member, is_hierarchical_container - cleans its argument again
  for itself.  The outer layer is therefore redundant, and removing it changes
  no reading of any type in any spelling.  No test can distinguish the two, so
  this one pins the RESULT rather than the mechanism, and the redundancy is
  noted here so a reader does not mistake it for load-bearing.

  Tests the following:
  - the resolved level is identical for all seven cv- and reference-qualified
    spellings, across carriers at several rungs
  - the volatile spellings in particular, which a trait phrased on the
    unstripped type would lose - here the qualifier is removed before the const
    lvalue is formed, so a volatile container is as observable as a plain one
*/
bool
tests_observational_level_of_cv_ref()
{
    bool ok = true;

    D_OC_CV_REF_AGREE(observational_level_of, oct_hierarchical_indexed,
                      observational_level::str);
    D_OC_CV_REF_AGREE(observational_level_of, oct_nested,
                      observational_level::str);
    D_OC_CV_REF_AGREE(observational_level_of, oct_vector,
                      observational_level::seq);
    D_OC_CV_REF_AGREE(observational_level_of, oct_set,
                      observational_level::seq);
    D_OC_CV_REF_AGREE(observational_level_of, oct_count_only,
                      observational_level::bag);
    D_OC_CV_REF_AGREE(observational_level_of, oct_contains_only,
                      observational_level::set);
    D_OC_CV_REF_AGREE(observational_level_of, oct_size_only,
                      observational_level::size);
    D_OC_CV_REF_AGREE(observational_level_of, oct_bare,
                      observational_level::none);

    // spelled out on one carrier, since surviving the volatile spelling is the
    // point of writing clean_t
    D_OC_CHECK(observational_level_of<volatile oct_vector>::value
                   == observational_level_of<oct_vector>::value);
    D_OC_CHECK(observational_level_of<const volatile oct_nested>::value
                   == observational_level::str);

    return ok;
}

/*
tests_observational_level_of_companions
  Verifies the `_t` alias and the `_v` companion.
  Tests the following:
  - the `_t` alias is the integral_constant naming the resolved level, so it
    carries the level in the type system and not merely as a value
  - the `_v` companion agrees with ::value across the rungs
  - both are usable in a constant expression
  - the `_v` half is gated on variable-template support
*/
bool
tests_observational_level_of_companions()
{
    bool ok = true;

    // the _t alias is the integral_constant, exactly
    D_OC_STATIC_CHECK(
        std::is_same<observational_level_of_t<oct_vector>,
                     std::integral_constant<observational_level,
                                            observational_level::seq> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<observational_level_of_t<oct_bare>,
                     std::integral_constant<
                         observational_level,
                         observational_level::none> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<observational_level_of_t<oct_nested>,
                     std::integral_constant<observational_level,
                                            observational_level::str> >::value);

    // ... and carries the same value the trait reports
    D_OC_CHECK(observational_level_of_t<oct_set>::value
                   == observational_level_of<oct_set>::value);
    D_OC_CHECK(observational_level_of_t<oct_count_only>::value
                   == observational_level_of<oct_count_only>::value);

    // usable in a constant expression
    D_OC_STATIC_CHECK(observational_level_of<oct_vector>::value
                          == observational_level::seq);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OC_STATIC_CHECK(observational_level_of_v<oct_nested>
                          == observational_level::str);

    D_OC_CHECK(observational_level_of_v<oct_vector>
                   == observational_level_of<oct_vector>::value);
    D_OC_CHECK(observational_level_of_v<oct_set>
                   == observational_level_of<oct_set>::value);
    D_OC_CHECK(observational_level_of_v<oct_count_only>
                   == observational_level_of<oct_count_only>::value);
    D_OC_CHECK(observational_level_of_v<oct_contains_only>
                   == observational_level_of<oct_contains_only>::value);
    D_OC_CHECK(observational_level_of_v<oct_size_only>
                   == observational_level_of<oct_size_only>::value);
    D_OC_CHECK(observational_level_of_v<oct_bare>
                   == observational_level_of<oct_bare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
