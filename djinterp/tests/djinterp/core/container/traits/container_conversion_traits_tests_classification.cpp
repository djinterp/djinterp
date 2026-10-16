/*******************************************************************************
* djinterp [testing]        container_conversion_traits_tests_classification.cpp
*
*   Section IV (classification) - conversion_tier_of, the verdict itself, driven
* across every tier and every decision branch, plus the boolean projections
* built on it (is_convertible_between, is_view_conversion, is_lossless_conversion,
* is_lossy_conversion, conversion_requires_restructuring).
*
*   The verdict's precedence is exercised end to end:
*     1. a TOPOLOGY change is judged FIRST - it relates the leaves, not the
*        top-level elements - so a flat <-> hierarchical crossing is `structural`
*        even when the elements are themselves incompatible.
*     2. otherwise incompatible elements admit `none`.
*     3. among compatible elements a forgetting or a capacity clamp is `lossy`.
*     4. then a free reinterpretation is `view` - including the
*        discipline-strictly-GREATER case, reachable only via a custom type
*        (sorted_vec -> std::vector<int>).
*     5. failing those a data-preserving construction is `constructive`.
*     6. and with none of the above (compatible, not lossy, not a view, no
*        construction path) the verdict falls through to `none`
*        (std::vector<int> -> sorted_vec).
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_conversion_tier_of_view
//   zero-cost reinterpretations: identities across the backing shapes, a
//   discipline-stronger source through a weaker interface, and the type-level
//   view between two distinct-but-equivalent array sizes.
bool
tests_conversion_tier_of_view()
{
    bool _ccv_ok = true;

    // identity is a view for every backing shape / overlay
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::vector<int>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::list<int>, std::list<int>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::deque<int>, std::deque<int>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::array<int, 4>, std::array<int, 4>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::string, std::string>::value
                      == conversion_tier::view) );
    // associative identity: layout is `none` for both, and every axis agrees
    D_CCV_CHECK( (conversion_tier_of<std::set<int>, std::set<int>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::multiset<int>, std::multiset<int>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::map<int, int>, std::map<int, int>>::value
                      == conversion_tier::view) );
    // hierarchical identity
    D_CCV_CHECK( (conversion_tier_of<std::vector<std::vector<int>>,
                                     std::vector<std::vector<int>>>::value
                      == conversion_tier::view) );

    // discipline strictly GREATER: sorted_vec wears every restriction a plain
    // sequence does and one more (its sorted invariant); dropping a restriction
    // is free, so viewing it AS a std::vector<int> is a reinterpretation.  This
    // is the only branch of is_reinterpretation_helper the standard library
    // cannot reach unaided.
    D_CCV_CHECK( (conversion_tier_of<sorted_vec, std::vector<int>>::value
                      == conversion_tier::view) );

    // type-level view across array sizes: same element, content level, backing
    // shape, lifetime, storage and discipline - the framework's honest-at-the-
    // sequence-layouts approximation classifies this as a view.
    D_CCV_CHECK( (conversion_tier_of<std::array<int, 4>, std::array<int, 8>>::value
                      == conversion_tier::view) );

    return _ccv_ok;
}


// tests_conversion_tier_of_constructive
//   O(n) / O(n log n) moves that preserve content: a differing backing, a
//   converting element copy, or a lossless invariant added.
bool
tests_conversion_tier_of_constructive()
{
    bool _ccv_ok = true;

    // differing backing shape, same content level
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::list<int>>::value
                      == conversion_tier::constructive) );
    D_CCV_CHECK( (conversion_tier_of<std::list<int>, std::vector<int>>::value
                      == conversion_tier::constructive) );
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::deque<int>>::value
                      == conversion_tier::constructive) );
    D_CCV_CHECK( (conversion_tier_of<std::deque<int>, std::vector<int>>::value
                      == conversion_tier::constructive) );

    // array -> vector: a std::array is contiguous like a vector yet compile-
    // staged and inline, so this materialises (a copy), not a view
    D_CCV_CHECK( (conversion_tier_of<std::array<int, 4>, std::vector<int>>::value
                      == conversion_tier::constructive) );

    // enumerating a set into a sequence (finer target, faithful at the set level)
    D_CCV_CHECK( (conversion_tier_of<std::set<int>, std::vector<int>>::value
                      == conversion_tier::constructive) );
    // a set embeds into a multiset (a lossless multiplicity relaxation)
    D_CCV_CHECK( (conversion_tier_of<std::set<int>, std::multiset<int>>::value
                      == conversion_tier::constructive) );

    // a converting element copy (int -> double)
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::list<double>>::value
                      == conversion_tier::constructive) );

    // string (string_like) -> vector<char> (array_like): backing differs, so a
    // build, not a view
    D_CCV_CHECK( (conversion_tier_of<std::string, std::vector<char>>::value
                      == conversion_tier::constructive) );

    // forward_list -> vector: forward_list's native level is `none` (no size());
    // moving to the finer seq level is not a loss, and a range build exists
    D_CCV_CHECK( (conversion_tier_of<std::forward_list<int>, std::vector<int>>::value
                      == conversion_tier::constructive) );

    return _ccv_ok;
}


// tests_conversion_tier_of_lossy
//   the conversion forgets (a strict coarsening) or clamps a capacity.
bool
tests_conversion_tier_of_lossy()
{
    bool _ccv_ok = true;

    // forget order (and, into a set, multiplicity too)
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::set<int>>::value
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::multiset<int>>::value
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_tier_of<std::deque<int>, std::set<int>>::value
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_tier_of<std::list<int>, std::multiset<int>>::value
                      == conversion_tier::lossy) );
    // forget multiplicity (deduplication)
    D_CCV_CHECK( (conversion_tier_of<std::multiset<int>, std::set<int>>::value
                      == conversion_tier::lossy) );
    // keyed forget multiplicity
    D_CCV_CHECK( (conversion_tier_of<std::multimap<int, int>, std::map<int, int>>::value
                      == conversion_tier::lossy) );

    // capacity clamp: an unbounded source narrowed to a fixed array
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::array<int, 4>>::value
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_tier_of<std::deque<int>, std::array<int, 4>>::value
                      == conversion_tier::lossy) );
    D_CCV_CHECK( (conversion_tier_of<std::list<int>, std::array<int, 4>>::value
                      == conversion_tier::lossy) );

    // edge: seq -> forward_list drops to the `none` level (forward_list is not
    // countable), a content-forgetting move
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::forward_list<int>>::value
                      == conversion_tier::lossy) );

    return _ccv_ok;
}


// tests_conversion_tier_of_structural
//   flat <-> hierarchical: a full reconstruction, judged before element
//   compatibility (so it stands even when the leaves do not match).
bool
tests_conversion_tier_of_structural()
{
    bool _ccv_ok = true;

    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::vector<std::vector<int>>>::value
                      == conversion_tier::structural) );
    D_CCV_CHECK( (conversion_tier_of<std::vector<std::vector<int>>, std::vector<int>>::value
                      == conversion_tier::structural) );

    // a string element nests the outer container (depth 2), so these cross the
    // divide - and are `structural` even though int and std::string are
    // themselves incompatible elements (topology is judged first)
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::vector<std::string>>::value
                      == conversion_tier::structural) );
    D_CCV_CHECK( (conversion_tier_of<std::vector<std::string>, std::vector<int>>::value
                      == conversion_tier::structural) );
    D_CCV_CHECK( (conversion_tier_of<std::array<std::string, 4>, std::vector<int>>::value
                      == conversion_tier::structural) );

    return _ccv_ok;
}


// tests_conversion_tier_of_none
//   no conversion path - reached two distinct ways: incompatible elements
//   (step 2), and a compatible-but-unbuildable, non-view target (the final
//   fallthrough, step 6).
bool
tests_conversion_tier_of_none()
{
    bool _ccv_ok = true;

    // incompatible elements, same (flat) topology
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::vector<opaque>>::value
                      == conversion_tier::none) );
    D_CCV_CHECK( (conversion_tier_of<std::vector<opaque>, std::vector<int>>::value
                      == conversion_tier::none) );
    // a map's pair element vs a set's plain element - incompatible, both flat
    D_CCV_CHECK( (conversion_tier_of<std::map<int, int>, std::set<int>>::value
                      == conversion_tier::none) );

    // the final fallthrough: elements ARE compatible (both int), it is neither
    // forgetting nor a clamp, it is NOT a view (sorted_vec's discipline is
    // strictly stronger than vector's, so vector -> sorted_vec runs the wrong
    // way for the free-reinterpretation rule), and sorted_vec exposes no range
    // constructor or range insert - so there is no construction path either.
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, sorted_vec>::value
                      == conversion_tier::none) );

    return _ccv_ok;
}


// tests_conversion_tier_of_cvref
//   the verdict is taken on clean_t of each operand, so cv-qualification and
//   references never change the tier.
bool
tests_conversion_tier_of_cvref()
{
    bool _ccv_ok = true;

    // const / ref / rvalue-ref all strip to the same view verdict
    D_CCV_CHECK( (conversion_tier_of<const std::vector<int>&, std::vector<int>>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<std::vector<int>, std::vector<int>&&>::value
                      == conversion_tier::view) );
    D_CCV_CHECK( (conversion_tier_of<const std::vector<int>&, volatile std::vector<int>&>::value
                      == conversion_tier::view) );

    // a cv-ref-qualified lossy pair still reads lossy
    D_CCV_CHECK( (conversion_tier_of<const std::vector<int>&, std::set<int>&>::value
                      == conversion_tier::lossy) );

    // and the qualified verdict matches the unqualified one exactly
    D_CCV_CHECK( (conversion_tier_of<const std::list<int>&, std::vector<int>&>::value
                      == conversion_tier_of<std::list<int>, std::vector<int>>::value) );

    return _ccv_ok;
}


// tests_conversion_predicates
//   the boolean projections of the tier, checked as a consistent set on one
//   representative pair per tier.
bool
tests_conversion_predicates()
{
    bool _ccv_ok = true;

    // -- view: vector -> vector --
    D_CCV_CHECK( (is_convertible_between<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (is_view_conversion<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (is_lossless_conversion<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (!is_lossy_conversion<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (!conversion_requires_restructuring<std::vector<int>, std::vector<int>>::value) );

    // discipline-greater view also reads lossless and non-lossy
    D_CCV_CHECK( (is_view_conversion<sorted_vec, std::vector<int>>::value) );
    D_CCV_CHECK( (is_lossless_conversion<sorted_vec, std::vector<int>>::value) );

    // -- constructive: vector -> list -- (lossless, but NOT a view)
    D_CCV_CHECK( (is_convertible_between<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (!is_view_conversion<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (is_lossless_conversion<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (!is_lossy_conversion<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (!conversion_requires_restructuring<std::vector<int>, std::list<int>>::value) );

    // -- lossy: vector -> set --
    D_CCV_CHECK( (is_convertible_between<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (!is_view_conversion<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (!is_lossless_conversion<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (is_lossy_conversion<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (!conversion_requires_restructuring<std::vector<int>, std::set<int>>::value) );

    // -- structural: vector -> vector<vector<int>> --
    D_CCV_CHECK( (is_convertible_between<std::vector<int>, std::vector<std::vector<int>>>::value) );
    D_CCV_CHECK( (!is_view_conversion<std::vector<int>, std::vector<std::vector<int>>>::value) );
    D_CCV_CHECK( (!is_lossless_conversion<std::vector<int>, std::vector<std::vector<int>>>::value) );
    D_CCV_CHECK( (!is_lossy_conversion<std::vector<int>, std::vector<std::vector<int>>>::value) );
    D_CCV_CHECK( (conversion_requires_restructuring<std::vector<int>, std::vector<std::vector<int>>>::value) );

    // -- none: vector -> vector<opaque> (incompatible) and vector -> sorted_vec
    //    (no path) -- every predicate false
    D_CCV_CHECK( (!is_convertible_between<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (!is_view_conversion<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (!is_lossless_conversion<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (!is_lossy_conversion<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (!conversion_requires_restructuring<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (!is_convertible_between<std::vector<int>, sorted_vec>::value) );

    return _ccv_ok;
}


NS_END  // testing
NS_END  // djinterp
