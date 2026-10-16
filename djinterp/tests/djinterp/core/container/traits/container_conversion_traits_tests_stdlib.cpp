/*******************************************************************************
* djinterp [core]                   container_conversion_traits_tests_stdlib.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_stdlib.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_stdlib_identity
  Every standard container converts to ITSELF as a VIEW.
  Tests the following:
  - the identity is a view for every kind of container - sequence, associative,
    string, nested
  - and it is lossless, and not lossy, and not structural
  - which is reflexivity, and is the one property of the whole module a caller can
    take entirely for granted - and which is therefore the one nobody checks
*/
bool
tests_stdlib_identity()
{
    bool ok = true;

    D_CVT_TIER(conversion_tier::view, std_vector, std_vector);
    D_CVT_TIER(conversion_tier::view, std_list, std_list);
    D_CVT_TIER(conversion_tier::view, std_deque, std_deque);
    D_CVT_TIER(conversion_tier::view, std_set, std_set);
    D_CVT_TIER(conversion_tier::view, std_multiset, std_multiset);
    D_CVT_TIER(conversion_tier::view, std_map, std_map);
    D_CVT_TIER(conversion_tier::view, std_str, std_str);
    D_CVT_TIER(conversion_tier::view, std_array, std_array);
    D_CVT_TIER(conversion_tier::view, std_vec_vec, std_vec_vec);
    D_CVT_TIER(conversion_tier::view, std_fwd_list, std_fwd_list);

    // and each is lossless, and not lossy, and not structural
    #define D_CVT_IDENTITY(TYPE)                                              \
        D_CVT_CHECK((is_view_conversion<TYPE, TYPE>::value));                 \
        D_CVT_CHECK((is_lossless_conversion<TYPE, TYPE>::value));             \
        D_CVT_CHECK((!is_lossy_conversion<TYPE, TYPE>::value));               \
        D_CVT_CHECK((!conversion_requires_restructuring<TYPE, TYPE>::value)); \
        D_CVT_CHECK((is_convertible_between<TYPE, TYPE>::value))

    D_CVT_IDENTITY(std_vector);
    D_CVT_IDENTITY(std_set);
    D_CVT_IDENTITY(std_str);
    D_CVT_IDENTITY(std_array);
    D_CVT_IDENTITY(std_vec_vec);

    #undef D_CVT_IDENTITY

    return ok;
}

/*
tests_stdlib_sequences
  The standard sequences against each other.
  Tests the following:
  - vector <-> list <-> deque are all CONSTRUCTIVE, in both directions.  They are
    realization-EQUAL in the comparison module's terms - same elements, same content
    rung, same discipline, same lifetime, same storage, same iterability - and differ
    ONLY in their backing layout.  That difference is what stops each of them being a
    zero-cost view of the others, and it is the whole reason the layout proxy is in
    the view predicate
  - the element conversion: vector<int> -> vector<long> is constructive, not a view -
    the bits change
  - vector<int> -> string is CONSTRUCTIVE, because an int converts to a char.  A
    semantically nonsensical conversion that the model reports honestly: the element
    relation is convertibility, and int converts to char
*/
bool
tests_stdlib_sequences()
{
    bool ok = true;

    // vector, list and deque: all constructive, both ways
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_list);
    D_CVT_TIER(conversion_tier::constructive, std_list, std_vector);
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_deque);
    D_CVT_TIER(conversion_tier::constructive, std_deque, std_vector);
    D_CVT_TIER(conversion_tier::constructive, std_list, std_deque);
    D_CVT_TIER(conversion_tier::constructive, std_deque, std_list);

    // ...and the ONLY thing separating them is the layout
    D_CVT_CHECK((realization_equal<std_vector, std_list>::value));
    D_CVT_CHECK((realization_equal<std_vector, std_deque>::value));
    D_CVT_CHECK((sequential_layout_of<std_vector>::value !=
                 sequential_layout_of<std_list>::value));
    D_CVT_CHECK((sequential_layout_of<std_vector>::value !=
                 sequential_layout_of<std_deque>::value));
    D_CVT_CHECK((sequential_layout_of<std_list>::value !=
                 sequential_layout_of<std_deque>::value));

    // ...so none of them is a view of another
    D_CVT_CHECK((!is_view_conversion<std_vector, std_list>::value));
    D_CVT_CHECK((!is_view_conversion<std_vector, std_deque>::value));

    // ...and all of them are LOSSLESS
    D_CVT_CHECK((is_lossless_conversion<std_vector, std_list>::value));
    D_CVT_CHECK((is_lossless_conversion<std_list, std_deque>::value));

    // a CONVERTING element copy: the bits change, so it is not a view
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_vector_long);
    D_CVT_TIER(conversion_tier::constructive, std_vector_long, std_vector);
    D_CVT_CHECK((!elements_same_type<std_vector, std_vector_long>::value));
    D_CVT_CHECK((elements_convertible<std_vector, std_vector_long>::value));

    // vector<int> -> string: an int converts to a char, so the model says
    // constructive.  Semantically absurd, and honestly reported - the element
    // relation is CONVERTIBILITY, and it is doing exactly what it says
    D_CVT_TIER(conversion_tier::constructive, std_vector, std_str);
    D_CVT_CHECK((elements_convertible<std_vector, std_str>::value));
    D_CVT_CHECK((!elements_same_type<std_vector, std_str>::value));

    return ok;
}

/*
tests_stdlib_associative
  The associative containers, and the two canonical quotients.
  Tests the following:
  - vector -> set is LOSSY: it forgets the order AND the multiplicity.  Deduplication
  - vector -> multiset is LOSSY: it forgets the order
  - multiset -> set is LOSSY: it forgets the multiplicity.  Deduplication, in its
    purest form
  - set -> multiset is CONSTRUCTIVE: To is FINER, nothing is lost
  - set -> vector is CONSTRUCTIVE: To is finer still
  - so the three rungs seq > bag > set order the lossiness exactly as the content
    hierarchy says they should, and the direction of the arrow is what decides
*/
bool
tests_stdlib_associative()
{
    bool ok = true;

    // *** DOWN the hierarchy: lossy ***
    D_CVT_TIER(conversion_tier::lossy, std_vector, std_set);
    D_CVT_TIER(conversion_tier::lossy, std_vector, std_multiset);
    D_CVT_TIER(conversion_tier::lossy, std_multiset, std_set);
    D_CVT_TIER(conversion_tier::lossy, std_list, std_set);
    D_CVT_TIER(conversion_tier::lossy, std_deque, std_multiset);

    // *** UP the hierarchy: constructive ***
    D_CVT_TIER(conversion_tier::constructive, std_set, std_multiset);
    D_CVT_TIER(conversion_tier::constructive, std_set, std_vector);
    D_CVT_TIER(conversion_tier::constructive, std_multiset, std_vector);
    D_CVT_TIER(conversion_tier::constructive, std_set, std_list);

    // ...and the rungs really are the rungs
    D_CVT_LEVEL(content_level::seq, native_content_level_of<std_vector>::value);
    D_CVT_LEVEL(content_level::bag,
                native_content_level_of<std_multiset>::value);
    D_CVT_LEVEL(content_level::set, native_content_level_of<std_set>::value);

    // so the DIRECTION of the arrow is what decides
    D_CVT_CHECK((is_lossy_conversion<std_vector, std_set>::value));
    D_CVT_CHECK((is_lossless_conversion<std_set, std_vector>::value));
    D_CVT_CHECK((is_lossy_conversion<std_multiset, std_set>::value));
    D_CVT_CHECK((is_lossless_conversion<std_set, std_multiset>::value));

    // ...and the shared rung a lossless conversion would preserve is the COARSER
    D_CVT_LEVEL(content_level::set,
                (conversion_class<std_vector, std_set>::shared_content_level));
    D_CVT_LEVEL(content_level::bag,
                (conversion_class<std_vector,
                                  std_multiset>::shared_content_level));

    // std::map is at `set` too - one value per key - so a vector of pairs would
    // deduplicate into it, and a map fans out into a vector losslessly
    D_CVT_LEVEL(content_level::set, native_content_level_of<std_map>::value);

    return ok;
}

/*
tests_stdlib_array
  std::array - the archetypal capacity-bounded container, and the one the `lossy`
  clamp rung exists to describe.

  THE FINDING, in the standard library.  std::array is an AGGREGATE: it has no range
  constructor and no insert.  So there is NO CONSTRUCTIVE PATH to it - not from a
  vector, not from a list, not from another std::array.  And the module says a
  conversion exists anyway, because the `lossy` rung never asks:

      conversion_tier_of<std::vector<int>, std::array<int, 3>>     ==  lossy
      is_convertible_between<std::vector<int>, std::array<int, 3>> ==  TRUE
      has_constructive_path<std::vector<int>, std::array<int, 3>>  ==  FALSE

  Tests the following:
  - array -> vector is CONSTRUCTIVE, and correctly so - a copy
  - ...even though they share a LAYOUT.  A std::array is contiguous exactly like a
    std::vector, and it is still a copy - because the array is compile-staged and
    inline where the vector is heap-backed and runtime.  This is the case the view
    predicate's realization clauses exist for, and the header's own comment names it
  - vector -> array is LOSSY (the clamp) and has NO PATH
  - array<int,3> -> array<int,64> does not clamp - both are bounded - and has no path
    either, so it is `none`.  The extent's VALUE is never read
*/
bool
tests_stdlib_array()
{
    bool ok = true;

    // array -> vector: a COPY, and correctly so
    D_CVT_TIER(conversion_tier::constructive, std_array, std_vector);
    D_CVT_CHECK((has_constructive_path<std_array, std_vector>::value));
    D_CVT_CHECK((is_lossless_conversion<std_array, std_vector>::value));

    // ...even though they share a LAYOUT.  The realization axes are what separate
    // them - the array is compile-staged and inline; the vector is not
    D_CVT_CHECK((sequential_layout_of<std_array>::value ==
                 sequential_layout_of<std_vector>::value));
    D_CVT_CHECK(sequential_layout_of<std_array>::value ==
                sequential_layout::array_like);
    D_CVT_CHECK((elements_same_type<std_array, std_vector>::value));
    D_CVT_CHECK((lifetime_relation<std_array, std_vector>::value !=
                 order_relation::equivalent));
    D_CVT_CHECK((!is_view_conversion<std_array, std_vector>::value));

    // *** vector -> array: LOSSY, and NO WAY TO DO IT ***
    D_CVT_TIER(conversion_tier::lossy, std_vector, std_array);
    D_CVT_CHECK((conversion_clamps_capacity<std_vector, std_array>::value));
    D_CVT_CHECK((is_convertible_between<std_vector, std_array>::value));
    D_CVT_CHECK((!has_constructive_path<std_vector, std_array>::value));

    // ...and it is EVERY conversion to a std::array
    D_CVT_CHECK((!has_constructive_path<std_list, std_array>::value));
    D_CVT_CHECK((!has_constructive_path<std_set, std_array>::value));
    D_CVT_CHECK((!has_constructive_path<std_str, std_array>::value));
    D_CVT_CHECK((!is_range_constructible<std_vector, std_array>::value));
    D_CVT_CHECK((!is_range_insertable<std_vector, std_array>::value));

    // array -> array of a different extent: no clamp (BOTH are bounded), so the
    // ladder falls straight through to the view predicate - and every clause of it
    // passes.  See tests_stdlib_array_extent, which is what that costs
    D_CVT_CHECK(is_bounded_container<std_array>::value);
    D_CVT_CHECK(is_bounded_container<std_array_64>::value);
    D_CVT_CHECK((!conversion_clamps_capacity<std_array,
                                             std_array_64>::value));
    D_CVT_CHECK((!has_constructive_path<std_array, std_array_64>::value));

    return ok;
}

/*
tests_stdlib_array_extent
  THE FINDING.  std::array<int, 3> is a ZERO-COST VIEW of std::array<int, 64>.

      is_view_conversion<std::array<int, 3>, std::array<int, 64>>   ==   TRUE
      is_lossless_conversion<...>                                   ==   TRUE
      conversion_tier_of<...>                                       ==   view

  The trait's contract is that no data moves: reinterpret the one as the other.  A
  caller who acts on it reads SIXTY-ONE INTS PAST THE END of a three-element buffer.

  And the other direction is no better:

      conversion_tier_of<std::array<int, 64>, std::array<int, 3>>   ==   view
      is_lossy_conversion<...>                                      ==   FALSE

  Sixty-one elements vanish, and the module does not even call it lossy.

  THE MECHANISM.  Every one of the eight clauses passes, and none of them is wrong -
  each is reading a property the two arrays genuinely share:

      1. elements     both int
      2. content      both seq
      3. topology     both flat
      4. layout       both array_like     <-- a SHAPE, not a SIZE
      5. lifetime     both compile-staged
      6. storage      both unknown
      7. iterability  both compile / non-const
      8. discipline   both capacity_bounded, mult = sequence   -> EQUIVALENT

  THE EXTENT'S VALUE IS NEVER READ.  is_bounded_container is a BOOL - it says
  whether there IS a bound, not what it is - and overlay_of<T>::capacity_bounded is
  a bool for the same reason.  conversion_clamps_capacity is
  `!bounded(From) && bounded(To)`, so it fires only when a bound is ADDED, and both
  arrays already have one.  Nowhere in the module, or in the comparison model
  underneath it, is one extent ever compared with another.

  The header's own RELIABILITY note anticipates exactly the shape of this bug and
  stops one step short of it: VIEW "additionally requires an identical sequential
  layout, the framework's backing-shape proxy".  A layout is a shape.  Two arrays of
  different length have the same shape.

  THE FIX.  The view predicate needs an extent-equality clause, and
  bounded_container_traits.hpp already detects the static `extent` it would compare.
  Alternatively conversion_clamps_capacity should fire whenever To's bound is
  TIGHTER than From's - not merely when From has none - which would make the
  narrowing direction `lossy` and leave the widening one to be caught by the view
  clause.  Both are needed: one closes the overrun, the other closes the silent
  truncation.

  Tests the following:
  - the verdict, in both directions
  - the eight clauses, one by one, so the finding is attributable
  - the mechanism: boundedness is a bool, and the extent's value is never compared
  - what the module DOES catch, so the finding is bounded: an unbounded source
    against a bounded target is `lossy`, correctly, because there the bound is being
    ADDED and the conjunct fires
*/
bool
tests_stdlib_array_extent()
{
    bool ok = true;

    // *** THE VERDICT ***
    D_CVT_TIER(conversion_tier::view, std_array, std_array_64);
    D_CVT_CHECK((is_view_conversion<std_array, std_array_64>::value));
    D_CVT_CHECK((is_lossless_conversion<std_array, std_array_64>::value));

    // *** AND THE OTHER DIRECTION, WHICH DISCARDS 61 ELEMENTS ***
    D_CVT_TIER(conversion_tier::view, std_array_64, std_array);
    D_CVT_CHECK((is_view_conversion<std_array_64, std_array>::value));
    D_CVT_CHECK((!is_lossy_conversion<std_array_64, std_array>::value));

    // THE EIGHT CLAUSES, one by one - every one of them passes
    D_CVT_CHECK((elements_same_type<std_array, std_array_64>::value));
    D_CVT_CHECK((native_content_level_of<std_array>::value ==
                 native_content_level_of<std_array_64>::value));
    D_CVT_CHECK((!conversion_changes_topology<std_array,
                                              std_array_64>::value));
    D_CVT_CHECK((sequential_layout_of<std_array>::value ==
                 sequential_layout_of<std_array_64>::value));
    D_CVT_CHECK((lifetime_relation<std_array, std_array_64>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((storage_relation<std_array, std_array_64>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((iterability_relation<std_array, std_array_64>::value ==
                 order_relation::equivalent));
    D_CVT_CHECK((discipline_relation<std_array, std_array_64>::value ==
                 order_relation::equivalent));

    // THE MECHANISM: boundedness is a BOOL, and the extent's value is never read
    D_CVT_CHECK(is_bounded_container<std_array>::value);
    D_CVT_CHECK(is_bounded_container<std_array_64>::value);
    D_CVT_CHECK(is_bounded_container<std_array>::value ==
                is_bounded_container<std_array_64>::value);

    D_CVT_CHECK(overlay_of<std_array>::capacity_bounded);
    D_CVT_CHECK(overlay_of<std_array_64>::capacity_bounded);
    D_CVT_CHECK(overlay_of<std_array>::capacity_bounded ==
                overlay_of<std_array_64>::capacity_bounded);

    // ...so the clamp never fires: it wants a bound ADDED, and both have one
    D_CVT_CHECK((!conversion_clamps_capacity<std_array,
                                             std_array_64>::value));
    D_CVT_CHECK((!conversion_clamps_capacity<std_array_64,
                                             std_array>::value));

    // ...and the LAYOUT, which the header's RELIABILITY note leans on, is a SHAPE
    // and not a SIZE.  Two arrays of different length have the same shape
    D_CVT_CHECK(sequential_layout_of<std_array>::value ==
                sequential_layout::array_like);
    D_CVT_CHECK(sequential_layout_of<std_array_64>::value ==
                sequential_layout::array_like);

    // WHAT THE MODULE DOES CATCH.  An UNBOUNDED source against a bounded target is
    // `lossy`, correctly - there the bound is being ADDED, and the conjunct fires.
    // The hole is exactly and only bounded -> bounded
    D_CVT_CHECK((conversion_clamps_capacity<std_vector, std_array>::value));
    D_CVT_TIER(conversion_tier::lossy, std_vector, std_array);
    D_CVT_CHECK((conversion_clamps_capacity<std_vector, std_array_64>::value));
    D_CVT_TIER(conversion_tier::lossy, std_vector, std_array_64);

    return ok;
}

/*
tests_stdlib_forward_list
  THE SECOND FINDING.  std::forward_list has no content rung at all - and so a
  faithful, range-constructible copy INTO it is classified LOSSY.

  A forward_list has no size().  So it is not a COUNTABLE container, so its
  multiplicity kind is `unknown`, so its native content rung is `none` - rank ZERO,
  the coarsest in the hierarchy.  And conversion_forgets_content is a strict rank
  comparison, so a conversion from ANY real container into it "factors through a
  coarsening":

      conversion_tier_of<std::vector<int>, std::forward_list<int>>  ==  lossy

  It does not.  std::forward_list has a range constructor, the copy is O(n), and it
  preserves every element in order.  The module reports that data is FORGOTTEN.

  The root cause is one level down - `none` should not be on the same scale as the
  four real rungs, and a forward_list should not be at `none` - but the damage lands
  here, in the one module whose whole job is to tell a caller whether a conversion
  costs them anything.

  Tests the following:
  - the verdict, and the range constructor that contradicts it
  - the mechanism: no size(), no rung, rank zero
  - the ASYMMETRY: forward_list -> vector is `constructive`, because the rank
    comparison is strict and 3 is not less than 0.  So the same faithful copy is
    lossy one way and lossless the other
*/
bool
tests_stdlib_forward_list()
{
    bool ok = true;

    // *** THE VERDICT ***
    D_CVT_TIER(conversion_tier::lossy, std_vector, std_fwd_list);
    D_CVT_CHECK((is_lossy_conversion<std_vector, std_fwd_list>::value));
    D_CVT_CHECK((!is_lossless_conversion<std_vector, std_fwd_list>::value));

    // *** AND THE COPY EXISTS, AND IS FAITHFUL ***
    D_CVT_CHECK((is_range_constructible<std_vector, std_fwd_list>::value));
    D_CVT_CHECK((has_constructive_path<std_vector, std_fwd_list>::value));
    D_CVT_CHECK((elements_same_type<std_vector, std_fwd_list>::value));

    // THE MECHANISM: no size(), so no rung, so rank zero - the coarsest
    D_CVT_LEVEL(content_level::none,
                native_content_level_of<std_fwd_list>::value);
    D_CVT_CHECK(content_level_rank(
                    native_content_level_of<std_fwd_list>::value) == 0);
    D_CVT_CHECK(content_level_rank(
                    native_content_level_of<std_vector>::value) == 3);
    D_CVT_CHECK((conversion_forgets_content<std_vector,
                                            std_fwd_list>::value));

    // ...and it really is iterable, and really is a container in every other sense
    D_CVT_CHECK(is_iterable_container<std_fwd_list>::value);
    D_CVT_CHECK(!is_hierarchical_container<std_fwd_list>::value);

    // *** THE ASYMMETRY.  The rank comparison is STRICT, so 3 < 0 is false and the
    // reverse copy is lossless.  The same faithful O(n) copy is lossy one way and
    // constructive the other ***
    D_CVT_CHECK((!conversion_forgets_content<std_fwd_list,
                                             std_vector>::value));
    D_CVT_TIER(conversion_tier::constructive, std_fwd_list, std_vector);
    D_CVT_CHECK((is_lossless_conversion<std_fwd_list, std_vector>::value));

    // ...so the two directions disagree about the same pair of types
    D_CVT_CHECK((is_lossy_conversion<std_vector, std_fwd_list>::value) &&
                (is_lossless_conversion<std_fwd_list, std_vector>::value));

    // and the shared rung the class reports is `none`, which is the one place a
    // caller could have noticed
    D_CVT_LEVEL(content_level::none,
                (conversion_class<std_vector,
                                  std_fwd_list>::shared_content_level));

    return ok;
}

/*
tests_stdlib_structural
  The flat / hierarchical divide, in the standard library.
  Tests the following:
  - vector<vector<int>> <-> vector<int> is STRUCTURAL, in both directions
  - and it is structural even though the ELEMENTS are incompatible - a
    vector<int> is not an int and does not convert to one.  Rung 1 beats rung 2,
    and the header says why: a topology change relates the LEAVES, not the top-level
    elements
  - it is structural even though a construction path exists in one direction - rung 1
    beats rung 5 as well
  - and it is NOT lossless, NOT lossy, and NOT a view.  `structural` is its own
    category, off the flat hierarchy entirely
  - a vector of vectors against a SET is structural too - the topology is judged
    before anything else, so no flat rung can override it
*/
bool
tests_stdlib_structural()
{
    bool ok = true;

    // across the divide, both ways
    D_CVT_TIER(conversion_tier::structural, std_vector, std_vec_vec);
    D_CVT_TIER(conversion_tier::structural, std_vec_vec, std_vector);
    D_CVT_TIER(conversion_tier::structural, std_vec_vec, std_list);
    D_CVT_TIER(conversion_tier::structural, std_set, std_vec_vec);

    // ...and the sides really are the sides
    D_CVT_CHECK(is_hierarchical_container<std_vec_vec>::value);
    D_CVT_CHECK(!is_hierarchical_container<std_vector>::value);
    D_CVT_LEVEL(content_level::str, native_content_level_of<std_vec_vec>::value);

    // *** rung 1 beats rung 2: the ELEMENTS are incompatible, and it is still
    // structural ***
    D_CVT_CHECK((!elements_compatible<std_vec_vec, std_vector>::value));
    D_CVT_TIER(conversion_tier::structural, std_vec_vec, std_vector);

    // *** and rung 1 beats rung 3: it also FORGETS content (str is rank 4, seq is 3),
    // and it is still structural ***
    D_CVT_CHECK((conversion_forgets_content<std_vec_vec, std_vector>::value));
    D_CVT_TIER(conversion_tier::structural, std_vec_vec, std_vector);

    // it is its own category: not a view, not lossless, not lossy
    D_CVT_CHECK((conversion_requires_restructuring<std_vector,
                                                   std_vec_vec>::value));
    D_CVT_CHECK((!is_view_conversion<std_vector, std_vec_vec>::value));
    D_CVT_CHECK((!is_lossless_conversion<std_vector, std_vec_vec>::value));
    D_CVT_CHECK((!is_lossy_conversion<std_vector, std_vec_vec>::value));

    // ...but a conversion DOES exist, in the module's terms
    D_CVT_CHECK((is_convertible_between<std_vector, std_vec_vec>::value));

    // and a nested container against another nested one is NOT structural - they are
    // on the same side of the divide
    D_CVT_CHECK((!conversion_changes_topology<std_vec_vec,
                                              std_vec_vec>::value));
    D_CVT_TIER(conversion_tier::view, std_vec_vec, std_vec_vec);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
