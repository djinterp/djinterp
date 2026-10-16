/*******************************************************************************
* djinterp [core]                   container_comparison_traits_tests_stdlib.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_comparison_traits_tests_stdlib.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_stdlib_content_rungs
  Where the standard library sits on the content hierarchy.
  Tests the following:
  - the sequences compare at `seq`
  - std::set and std::unordered_set compare at `set` - a unique discipline
  - std::multiset compares at `bag`
  - std::map compares at `set`: its multiplicity is unique (one value per key),
    and its keyed-ness is a separate overlay flag, not a content rung
  - a vector of vectors compares at `str` - the nesting is structural
  - a non-container has no rung
*/
bool
tests_stdlib_content_rungs()
{
    bool ok = true;

    // sequences
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<std_vector>::value);
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<std_list>::value);
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<std_deque>::value);
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<std_array>::value);
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<std_str>::value);

    // unique disciplines
    D_CCM_LEVEL(content_level::set,
                native_content_level_of<std_set>::value);
    D_CCM_LEVEL(content_level::set,
                native_content_level_of<std_unord_set>::value);
    D_CCM_LEVEL(content_level::set,
                native_content_level_of<std_map>::value);

    // a multiset
    D_CCM_LEVEL(content_level::bag,
                native_content_level_of<std_multiset>::value);

    // nesting is structural
    D_CCM_LEVEL(content_level::str,
                native_content_level_of<std_vec_vec>::value);
    D_CCM_CHECK(is_hierarchical_container<
                    std_vec_vec>::value);

    // a map is KEYED and its rung is still `set` - the keyed-ness is an overlay
    // flag, not a content rung, and the two are different axes
    D_CCM_CHECK(overlay_of<std_map>::keyed);
    D_CCM_CHECK(!overlay_of<std_set>::keyed);
    D_CCM_LEVEL(content_level::set,
                native_content_level_of<std_map>::value);

    // and nothing at all
    D_CCM_LEVEL(content_level::none, native_content_level_of<int>::value);

    return ok;
}

/*
tests_stdlib_sequences
  The standard sequences against each other.
  Tests the following:
  - std::vector, std::list and std::deque are REALIZATION-EQUAL.  They differ in
    every way an engineer cares about - complexity, invalidation, layout - and in
    no way this module reads: same discipline, same runtime lifetime, same
    non-const iterability, same (unknown) storage.  That is not a bug.  It is what
    the module is FOR, and it is also the sharpest statement of what it does not
    see
  - std::array REFINES all of them: its extent makes it capacity-bounded (a
    stronger discipline) AND compile-time (a more static lifetime and a
    higher iterability stage).  Three axes move at once, all the same way, so the
    meet is `greater`
  - the direction, both ways
*/
bool
tests_stdlib_sequences()
{
    bool ok = true;

    // vector, list and deque are indistinguishable to this module
    D_CCM_DUAL(order_relation::equivalent, std_vector, std_list);
    D_CCM_DUAL(order_relation::equivalent, std_list, std_deque);
    D_CCM_DUAL(order_relation::equivalent, std_vector, std_deque);
    D_CCM_DUAL(order_relation::equivalent, std_vector, std_str);

    D_CCM_CHECK((realization_equal<std_vector, std_list>::value));
    D_CCM_CHECK((realization_equal<std_vector, std_deque>::value));

    // std::array refines them all - and on THREE axes at once
    D_CCM_REL(order_relation::greater,
              (discipline_relation<std_array,
                                   std_vector>::value));
    D_CCM_REL(order_relation::greater,
              (lifetime_relation<std_array,
                                 std_vector>::value));
    D_CCM_REL(order_relation::greater,
              (iterability_relation<std_array,
                                    std_vector>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<std_array, std_vector>::value));

    D_CCM_DUAL(order_relation::greater, std_array, std_vector);
    D_CCM_DUAL(order_relation::greater, std_array, std_list);

    D_CCM_CHECK((realization_comparable<std_array,
                                        std_vector>::value));
    D_CCM_CHECK((!realization_equal<std_array,
                                    std_vector>::value));

    // ...and the reason: the extent is a capacity bound AND a compile-time
    // lifetime.  The two are coupled, and there is no way to have one without the
    // other for a std::array
    D_CCM_CHECK(overlay_of<std_array>::capacity_bounded);
    D_CCM_CHECK(!overlay_of<std_vector>::capacity_bounded);
    D_CCM_CHECK(is_compile_time(container_lifetime<std_array>::value));
    D_CCM_CHECK(!is_compile_time(container_lifetime<std_vector>::value));

    // two arrays of different extent are realization-EQUAL: the extent's VALUE is
    // not on any axis, only its existence
    D_CCM_DUAL(order_relation::equivalent,
               std_array, std_array_64);

    return ok;
}

/*
tests_stdlib_associative
  The associative containers.
  Tests the following:
  - std::set has a TIGHTER discipline than std::vector (unique beats sequence on
    the multiplicity rank) and a LESS capable iterability (its iterators are
    const, so it cannot be written through).  Two axes move in OPPOSITE
    directions, and the refinement is therefore INCOMPARABLE - which is exactly
    the situation a product order exists to report and a scalar "distance" would
    have averaged away
  - std::multiset sits between them on the multiplicity rank and is likewise
    incomparable with the sequences
  - std::map REFINES std::set: same multiplicity, plus the keyed flag
  - std::set and std::unordered_set are realization-EQUAL - the module reads
    neither the ordering nor the hashing, and says so
*/
bool
tests_stdlib_associative()
{
    bool ok = true;

    // *** the headline: set against vector is INCOMPARABLE ***
    D_CCM_REL(order_relation::greater,
              (discipline_relation<std_set, std_vector>::value));
    D_CCM_REL(order_relation::less,
              (iterability_relation<std_set, std_vector>::value));
    D_CCM_REL(order_relation::equivalent,
              (lifetime_relation<std_set, std_vector>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<std_set, std_vector>::value));

    D_CCM_DUAL(order_relation::incomparable,
               std_set, std_vector);
    D_CCM_CHECK((!realization_comparable<std_set,
                                         std_vector>::value));

    // ...and the reason, in one line: a set's iterators are const
    D_CCM_CHECK(iteration_mode_of<std_set>::value ==
                iteration_mode::const_only);
    D_CCM_CHECK(iteration_mode_of<std_vector>::value ==
                iteration_mode::non_const);

    // A MULTISET IS *LESS* THAN A SEQUENCE, not incomparable - and the reason is
    // a finding.  multiplicity_kind_rank gives `sequence` and `unbounded_multiset`
    // THE SAME RANK (2), because both have m = inf.  So a multiset and a vector
    // are DISCIPLINE-EQUAL, even though a multiset carries a genuine equivalence
    // relation and a sequence carries none - the rank measures the multiplicity
    // BOUND and erases whether there is an equivalence at all.
    //
    //   With the disciplines equal, the only axis that moves is iterability (a
    // multiset's iterators are const), and the verdict is a clean `less`: the
    // module reports that a multiset is a strictly WEAKER container than a vector,
    // and that its equivalence relation buys it nothing
    D_CCM_CHECK(multiplicity_kind_rank(multiplicity_kind::sequence) ==
                multiplicity_kind_rank(multiplicity_kind::unbounded_multiset));
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<std_multiset, std_vector>::value));
    D_CCM_REL(order_relation::less,
              (iterability_relation<std_multiset, std_vector>::value));
    D_CCM_DUAL(order_relation::less, std_multiset, std_vector);

    // std::map refines std::set: same multiplicity, plus the keyed flag
    D_CCM_CHECK(overlay_of<std_map>::mult ==
                overlay_of<std_set>::mult);
    D_CCM_CHECK(overlay_of<std_map>::keyed &&
                !overlay_of<std_set>::keyed);
    D_CCM_REL(order_relation::greater,
              (discipline_relation<std_map,
                                   std_set>::value));
    D_CCM_DUAL(order_relation::greater, std_map, std_set);

    // set and unordered_set are realization-EQUAL: the module reads neither the
    // ordering nor the hashing
    D_CCM_DUAL(order_relation::equivalent,
               std_set, std_unord_set);
    D_CCM_CHECK((realization_equal<std_set,
                                   std_unord_set>::value));

    // ...and neither of them declares the sorted invariant, which is opt-in and
    // is NOT inferred from associativity - the header says so outright, and this
    // is the check
    D_CCM_CHECK(!overlay_of<std_set>::sorted);
    D_CCM_CHECK(!overlay_of<std_map>::sorted);

    return ok;
}

/*
tests_stdlib_refinement
  The whole module, on the standard library, as a caller would read it.
  Tests the following:
  - the refinement is REFLEXIVE and obeys the direction contract over the stdlib,
    not merely over the fixtures
  - std::array is the only standard container that refines anything
  - the associatives are incomparable with the sequences, in both directions
  - a non-container is incomparable with all of them - which is the rescue that
    tests_refinement_non_container shows does NOT always hold
*/
bool
tests_stdlib_refinement()
{
    bool ok = true;

    #define D_CCM_STD_REFLEXIVE(...)                                          \
        D_CCM_REL(order_relation::equivalent,                                 \
                  (refinement_of<__VA_ARGS__, __VA_ARGS__>::value));                        \
        D_CCM_CHECK((realization_equal<__VA_ARGS__, __VA_ARGS__>::value))

    D_CCM_STD_REFLEXIVE(std_vector);
    D_CCM_STD_REFLEXIVE(std_array);
    D_CCM_STD_REFLEXIVE(std_set);
    D_CCM_STD_REFLEXIVE(std_map);
    D_CCM_STD_REFLEXIVE(std_multiset);
    D_CCM_STD_REFLEXIVE(std_str);
    D_CCM_STD_REFLEXIVE(std_vec_vec);

    #undef D_CCM_STD_REFLEXIVE

    // std::array is the only one that refines anything
    D_CCM_DUAL(order_relation::greater, std_array, std_vector);
    D_CCM_DUAL(order_relation::greater, std_array, std_str);

    // the associatives are incomparable with the sequences
    D_CCM_DUAL(order_relation::incomparable,
               std_set, std_vector);
    D_CCM_DUAL(order_relation::incomparable,
               std_map, std_list);
    D_CCM_DUAL(order_relation::less, std_multiset, std_deque);

    // ...and with std::array, which is more static AND less mutable-capable than
    // they are on one axis and more disciplined on another.  Three-way mismatch
    D_CCM_DUAL(order_relation::incomparable,
               std_set, std_array);

    // a non-container is incomparable with every real one.  The iterability axis
    // is what saves it - see tests_refinement_non_container for the case it does
    // not save
    D_CCM_DUAL(order_relation::incomparable, int, std_vector);
    D_CCM_DUAL(order_relation::incomparable, void, std_set);
    D_CCM_DUAL(order_relation::incomparable, int, std_array);

    // ...and the DISCIPLINE axis alone still says the non-container is stronger -
    // but ONLY against a container that wears no flag-restriction.  std::array
    // wears `capacity_bounded`, and the non-container does not, so the flag clause
    // of overlay_subsumes fails and the two come out incomparable.  The bogus
    // `greater` needs the flag clauses to be VACUOUS, which they are for every
    // standard container except std::array
    D_CCM_REL(order_relation::greater,
              (discipline_relation<int, std_vector>::value));
    D_CCM_REL(order_relation::greater,
              (discipline_relation<int, std_set>::value));
    D_CCM_REL(order_relation::greater,
              (discipline_relation<int, std_list>::value));

    D_CCM_CHECK(overlay_of<std_array>::capacity_bounded);
    D_CCM_CHECK(!overlay_of<int>::capacity_bounded);
    D_CCM_REL(order_relation::incomparable,
              (discipline_relation<int, std_array>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
