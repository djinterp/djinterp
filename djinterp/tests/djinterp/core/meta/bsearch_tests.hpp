/******************************************************************************
* djinterp [testing]                                         bsearch_tests.hpp
*
* Unit test suite for bsearch.hpp -- the compile-time binary-search engine.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_BS_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       bsearch_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  bsearch.hpp is a pure compile-time header: a
* bounds-guarded indexed accessor (pack_element_or) and one engine
* (bsearch_by) that binary-searches a typename pack.  The engine is needle-
* AGNOSTIC -- the search target lives entirely inside two caller-supplied
* template-template predicates, _Below<E> and _Above<E>, so ONE engine serves
* an NTTP needle, a type needle, a reversed order, or a custom projection with
* no change.  Every assertion in this suite is therefore a compile-time fact;
* the tests_* bodies read those facts back through the runtime D_BS_CHECK so a
* failure is reportable rather than a translation-abort.  Where a property can
* only be stated at compile time (a value used as a template argument, a
* deferral that must not instantiate) the body carries a static_assert too.
*
*   THE FIXTURE VOCABULARY (normal mode only)
*     ikey<K>            an entry carrying `static constexpr int key = K`; the
*                        canonical value-keyed entry, ordered by `<` on ::key.
*     tag_key<K, T>      an entry with the same ::key plus a distinct tag T, so
*                        two entries can share a key yet be different types --
*                        the material for the duplicate-key landing test.
*     asc_preds<N>       the ASCENDING needle predicates for an int needle N:
*                        below(E) = E::key < N, above(E) = N < E::key.  This is
*                        exactly the shape lookup.hpp's nttp_key_preds forms.
*     desc_preds<N>      the DESCENDING predicates (arrows flipped), for the
*                        reversed-order test.
*     sized<S>           a type of size S (no ::key at all), for a TYPE needle
*                        ordered by sizeof.
*     size_preds<N>      predicates for a sizeof needle: below(E)= sizeof(E)<N,
*                        above(E)= N<sizeof(E).
*     scaled<K>          an entry whose key is K, used to build a large sorted
*                        pack (odd keys) for the depth / scale checks.
*     always_below / always_above / never_either
*                        degenerate predicate pairs used to pin the branch
*                        directions and the all-miss / vacuous-hit corners.
*
*     search<N, Es...>       = bsearch_by<asc_preds<N>::below,  ...::above, Es...>
*     search_desc<N, Es...>  the descending counterpart
*     search_size<N, Es...>  the sizeof counterpart
*   -- convenience aliases so a body reads `search<5, e1, e5, e9>::index` rather
*   than restating the predicate plumbing every line.
*
*   MODULE NOTES (behaviour this suite pins, not defects)
*     1. DUPLICATE KEYS.  When several entries compare equal to the needle, the
*        engine reports whichever equal entry the bisection first lands on --
*        NOT necessarily the first (leftmost) occurrence.  For three equal keys
*        at indices {0,1,2} the first probe is mid = 0 + (3-0)/2 = 1, a hit, so
*        index 1 is reported.  This is stable and deterministic but is a
*        landing position, not a lower_bound; the test pins the landing index.
*     2. INSTANTIATION DEFERRAL.  pack_element_or with _Ok == false must NOT
*        instantiate pack_element_t.  The evidence is constructive: naming
*        pack_element_t<lookup_npos, ...> directly is a hard error (runaway
*        recursion), whereas pack_element_or_t<false, lookup_npos, ...> compiles
*        and yields the fallback.  The suite exercises the guarded form on an
*        index that would be catastrophic if the guard leaked, and relies on the
*        TU compiling at all as half the assertion.
*     3. MISS SENTINELS.  A miss yields ::index == lookup_npos and
*        ::type == lookup_not_found; ::found is the derived bool.  lookup_npos is
*        (std::size_t)-1.  These come from lookup_sentinels.hpp, shared with
*        lookup.hpp.
*     4. COMPLEXITY.  The engine bounds recursion DEPTH at O(log N); the
*        instantiation COUNT is not log N (each probe is an O(index) pack walk).
*        The suite does not measure timing -- it pins correctness at a size large
*        enough to exercise several levels of bisection.
*
*   BUILD PREREQUISITE: none.  bsearch.hpp compiles as shipped, given its
*   siblings pack_element.hpp and lookup_sentinels.hpp on the include path (both
*   are part of the meta/util core, not of this suite).
*
* CONTENTS
*   I.    GUARD        (bsearch_tests_guard.cpp)
*   II.   HITS         (bsearch_tests_hits.cpp)
*   III.  MISSES       (bsearch_tests_misses.cpp)
*   IV.   ORDERING     (bsearch_tests_ordering.cpp)
*   V.    ROBUSTNESS   (bsearch_tests_robustness.cpp)
*
* path:      /tests/djinterp/core/meta/bsearch_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TESTS_BSEARCH_TESTS_
#define DJINTERP_TESTS_BSEARCH_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <type_traits>

// -- (part 1) mode-gated includes --------------------------------------------
// djinterp core is ALWAYS first and unconditional (NS_*, D_* qualifiers,
// language gates), so both faces have it.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "bsearch.hpp"                          // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY,
// because the spec provider (spec mode) needs dt::module_spec too.
namespace dt = ::djinterp::test;


// bsearch_check
//   function: routes one D_BS_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>
// only), so it lives above the fixture guard.
inline bool
bsearch_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_BS_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression (std::is_same<A, B>::value) passes through
// whole.  The `BS` suffix is this suite's unique two letters.
#define D_BS_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::bsearch_check(                              \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   VALUE-KEYED ENTRIES AND PREDICATES                    ///
///////////////////////////////////////////////////////////////////////////////

// ikey
//   struct: the canonical value-keyed entry -- an ::key NTTP ordered by `<`.
template<int _Key>
struct ikey
{
    static constexpr int key = _Key;
};

// tag_key
//   struct: an entry with an ::key plus a distinct tag, so two entries may
// share a key yet remain different types (the duplicate-key material).
template<int _Key,
         int _Tag>
struct tag_key
{
    static constexpr int key = _Key;
    static constexpr int tag = _Tag;
};

// asc_preds
//   struct: the ascending needle predicates for an int needle -- exactly the
// shape lookup.hpp's nttp_key_preds forms.  below(E) sorts E before the needle;
// above(E) sorts E after it; a hit is neither.
template<int _Needle>
struct asc_preds
{
    template<typename _Entry>
    struct below
        : std::integral_constant<bool, (_Entry::key < _Needle)>
    {};

    template<typename _Entry>
    struct above
        : std::integral_constant<bool, (_Needle < _Entry::key)>
    {};
};

// desc_preds
//   struct: the descending predicates -- arrows flipped, for a pack sorted
// high-to-low.  below(E) is true when E::key > needle (E is earlier in a
// descending pack); above(E) when needle > E::key.
template<int _Needle>
struct desc_preds
{
    template<typename _Entry>
    struct below
        : std::integral_constant<bool, (_Entry::key > _Needle)>
    {};

    template<typename _Entry>
    struct above
        : std::integral_constant<bool, (_Needle > _Entry::key)>
    {};
};

// scaled
//   struct: an ::key entry, distinguished from ikey only by name, used to build
// the wide sorted packs of the depth / scale checks.
template<int _Key>
struct scaled
{
    static constexpr int key = _Key;
};


///////////////////////////////////////////////////////////////////////////////
///             F.2   TYPE-KEYED ENTRIES AND PREDICATES                     ///
///////////////////////////////////////////////////////////////////////////////

// sized
//   struct: a type of a chosen size and NO ::key, ordered by sizeof -- the
// material for a TYPE needle, proving the engine never touches ::key.
template<std::size_t _Size>
struct sized
{
    char storage[_Size];
};

// size_preds
//   struct: predicates for a sizeof needle.  The needle is a size N; below(E)
// holds when sizeof(E) < N and above(E) when N < sizeof(E).
template<std::size_t _Needle>
struct size_preds
{
    template<typename _Entry>
    struct below
        : std::integral_constant<bool, (sizeof(_Entry) < _Needle)>
    {};

    template<typename _Entry>
    struct above
        : std::integral_constant<bool, (_Needle < sizeof(_Entry))>
    {};
};


///////////////////////////////////////////////////////////////////////////////
///             F.3   DEGENERATE PREDICATE PAIRS                            ///
///////////////////////////////////////////////////////////////////////////////

// always_below
//   trait: every entry sorts before the needle -> the engine always steps
// RIGHT, walks off the top, and reports a miss.  Pins the go-right direction.
template<typename _Entry>
struct always_below
    : std::true_type
{};

// always_above
//   trait: every entry sorts after the needle -> the engine always steps LEFT,
// collapses to the bottom, and reports a miss.  Pins the go-left direction.
template<typename _Entry>
struct always_above
    : std::true_type
{};

// never_either
//   trait: no entry sorts either way -> the very first probe is a hit.  Pins
// the vacuous-hit corner (any non-empty pack "matches" at the first mid).
template<typename _Entry>
struct never_either
    : std::false_type
{};


///////////////////////////////////////////////////////////////////////////////
///             F.4   SEARCH ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////

// search
//   alias: bsearch_by with the ascending int-needle predicates baked in, so a
// body reads search<5, e1, e5, e9> rather than restating the plumbing.
template<int         _Needle,
         typename... _Entries>
using search =
    bsearch_by<asc_preds<_Needle>::template below,
               asc_preds<_Needle>::template above,
               _Entries...>;

// search_desc
//   alias: the descending counterpart of search.
template<int         _Needle,
         typename... _Entries>
using search_desc =
    bsearch_by<desc_preds<_Needle>::template below,
               desc_preds<_Needle>::template above,
               _Entries...>;

// search_size
//   alias: the sizeof-needle counterpart of search, over sized<> entries.
template<std::size_t _Needle,
         typename... _Entries>
using search_size =
    bsearch_by<size_preds<_Needle>::template below,
               size_preds<_Needle>::template above,
               _Entries...>;


///////////////////////////////////////////////////////////////////////////////
///             F.5   COMPILE-TIME PROBES                                   ///
///////////////////////////////////////////////////////////////////////////////

// index_is_constexpr
//   trait: instantiable ONLY if _Index is a genuine constant expression -- the
// std::size_t template parameter forces constant evaluation, so its mere
// formation is the proof.  A body forms index_is_constexpr<search<...>::index>
// to show ::index is usable as a template argument.
template<std::size_t _Index>
struct index_is_constexpr
    : std::integral_constant<std::size_t, _Index>
{};

// consumes_bool
//   trait: instantiable only for a constant bool -- the analogue for ::found.
template<bool _Value>
struct consumes_bool
    : std::integral_constant<bool, _Value>
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   GUARD   (bsearch_tests_guard.cpp)
bool tests_guard_true_branch_indexes();
bool tests_guard_false_branch_returns_fallback();
bool tests_guard_defers_instantiation_at_invalid_index();
bool tests_guard_alias_matches_trait();
bool tests_guard_independent_of_pack_contents();

// II.  HITS   (bsearch_tests_hits.cpp)
bool tests_hits_single_element();
bool tests_hits_first_last_and_middle();
bool tests_hits_every_position_odd_count();
bool tests_hits_every_position_even_count();
bool tests_hits_result_members_are_consistent();
bool tests_hits_type_alias_matches_entry();
bool tests_hits_two_element_pack();
bool tests_hits_index_is_a_constant_expression();

// III. MISSES   (bsearch_tests_misses.cpp)
bool tests_misses_empty_pack();
bool tests_misses_below_all();
bool tests_misses_above_all();
bool tests_misses_interior_gaps();
bool tests_misses_single_element_low_and_high();
bool tests_misses_sentinels_are_shared_values();
bool tests_misses_result_members_are_consistent();
bool tests_misses_alias_yields_not_found();

// IV.  ORDERING   (bsearch_tests_ordering.cpp)
bool tests_ordering_descending_pack();
bool tests_ordering_type_needle_by_sizeof();
bool tests_ordering_duplicate_keys_land_deterministically();
bool tests_ordering_branch_directions_go_right();
bool tests_ordering_branch_directions_go_left();
bool tests_ordering_vacuous_hit_at_first_probe();
bool tests_ordering_needle_absent_from_engine_signature();

// V.   ROBUSTNESS   (bsearch_tests_robustness.cpp)
bool tests_robustness_large_pack_all_hits();
bool tests_robustness_large_pack_boundaries_and_gaps();
bool tests_robustness_members_usable_as_template_arguments();
bool tests_robustness_repeated_instantiation_is_stable();
bool tests_robustness_distinct_needles_distinct_results();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// bsearch_spec
//   function: the authoritative description of this suite -- one block per
// section TU, one row per tests_* body, every node named and described.
inline dt::module_spec
bsearch_spec()
{
    return dt::module_spec{
        "bsearch",
        "The compile-time binary-search engine over a typename pack. The needle "
        "never appears in the engine's signature: the caller injects the order "
        "as two template-template predicates, _Below<E> and _Above<E>, with the "
        "target baked in, and a hit is (!_Below && !_Above). One engine thus "
        "serves an NTTP needle, a type needle, a reversed order, or a custom "
        "projection alike. The header supplies pack_element_or (a bounds-guarded "
        "indexed accessor whose whole point is to DEFER instantiating "
        "pack_element_t on the miss path) and bsearch_by (the engine, reporting "
        "::type / ::found / ::index against the lookup_sentinels shared with "
        "lookup.hpp). Preconditions: the pack is sorted ascending under the same "
        "order the predicates encode. This suite pins the guard, hits at every "
        "position, the full family of misses, needle-agnostic ordering "
        "including duplicate-key landing, and behaviour at scale.",
        {
            dt::block_spec{
                "guard",
                "pack_element_or / pack_element_or_t: the bounds-guarded indexed "
                "accessor that fronts the engine's miss path. What it returns on "
                "each branch, and -- centrally -- that the false branch never "
                "instantiates pack_element_t, even at an index that would be "
                "catastrophic if the guard leaked.",
                {
                    { "tests_guard_true_branch_indexes",
                      "With _Ok true, pack_element_or_t is pack_element_t at the "
                      "given index -- the head at 0, an interior element, and "
                      "the last element.",
                      &tests_guard_true_branch_indexes },
                    { "tests_guard_false_branch_returns_fallback",
                      "With _Ok false, pack_element_or_t is the fallback type "
                      "verbatim, regardless of the index or the pack.",
                      &tests_guard_false_branch_returns_fallback },
                    { "tests_guard_defers_instantiation_at_invalid_index",
                      "The false branch does not touch pack_element_t: the "
                      "guarded form compiles and yields the fallback even at "
                      "lookup_npos, an index whose direct access is a hard error "
                      "-- so the TU compiling is itself half the assertion.",
                      &tests_guard_defers_instantiation_at_invalid_index },
                    { "tests_guard_alias_matches_trait",
                      "pack_element_or_t<...> names the same type as "
                      "pack_element_or<...>::type on both branches.",
                      &tests_guard_alias_matches_trait },
                    { "tests_guard_independent_of_pack_contents",
                      "The fallback branch ignores the pack entirely: an empty "
                      "pack and a populated one give the same fallback, and the "
                      "true branch reads the pack that is actually present.",
                      &tests_guard_independent_of_pack_contents },
                }
            },
            dt::block_spec{
                "hits",
                "bsearch_by locating a needle that is present: single-element "
                "packs, first / last / middle, every position across odd and "
                "even counts, and the internal consistency of the reported "
                "::type / ::found / ::index.",
                {
                    { "tests_hits_single_element",
                      "A one-element pack whose only key equals the needle "
                      "reports index 0, found true, and that element as ::type.",
                      &tests_hits_single_element },
                    { "tests_hits_first_last_and_middle",
                      "In a five-element pack, the needles at the first, middle "
                      "and last keys are each found at their own index.",
                      &tests_hits_first_last_and_middle },
                    { "tests_hits_every_position_odd_count",
                      "Across a five-element pack every key is found at exactly "
                      "its position, sweeping all indices 0..4.",
                      &tests_hits_every_position_odd_count },
                    { "tests_hits_every_position_even_count",
                      "Across a six-element pack every key is found at exactly "
                      "its position, so the even-count bisection is exercised at "
                      "each index.",
                      &tests_hits_every_position_even_count },
                    { "tests_hits_result_members_are_consistent",
                      "On a hit the three members agree: found is true, index is "
                      "not lookup_npos, and pack_element_t at that index is the "
                      "reported ::type.",
                      &tests_hits_result_members_are_consistent },
                    { "tests_hits_type_alias_matches_entry",
                      "bsearch_by_t equals bsearch_by<...>::type, and both name "
                      "the matched entry.",
                      &tests_hits_type_alias_matches_entry },
                    { "tests_hits_two_element_pack",
                      "A two-element pack finds each key at its index and misses "
                      "cleanly between and around them -- the smallest case with "
                      "a left / right choice.",
                      &tests_hits_two_element_pack },
                    { "tests_hits_index_is_a_constant_expression",
                      "A hit's ::index is a genuine constant expression: it "
                      "instantiates a std::size_t-parameterised template, which "
                      "only a constant can do.",
                      &tests_hits_index_is_a_constant_expression },
                }
            },
            dt::block_spec{
                "misses",
                "bsearch_by when the needle is absent: the empty pack, a needle "
                "below all / above all keys, needles in the interior gaps, "
                "single-element misses on both sides, and the shared miss "
                "sentinels the result carries.",
                {
                    { "tests_misses_empty_pack",
                      "Searching an empty pack reports lookup_npos, found false, "
                      "and lookup_not_found as ::type.",
                      &tests_misses_empty_pack },
                    { "tests_misses_below_all",
                      "A needle smaller than every key misses: the search steps "
                      "left off the bottom and reports lookup_npos.",
                      &tests_misses_below_all },
                    { "tests_misses_above_all",
                      "A needle larger than every key misses: the search steps "
                      "right off the top and reports lookup_npos.",
                      &tests_misses_above_all },
                    { "tests_misses_interior_gaps",
                      "Needles falling between adjacent keys miss at several "
                      "interior gaps, each reporting lookup_npos and "
                      "lookup_not_found.",
                      &tests_misses_interior_gaps },
                    { "tests_misses_single_element_low_and_high",
                      "Against a one-element pack, a needle below the key and a "
                      "needle above it both miss.",
                      &tests_misses_single_element_low_and_high },
                    { "tests_misses_sentinels_are_shared_values",
                      "The miss sentinels are the shared ones: lookup_npos is "
                      "(std::size_t)-1 and lookup_not_found is the sentinel "
                      "type, so the engine and lookup.hpp agree on misses.",
                      &tests_misses_sentinels_are_shared_values },
                    { "tests_misses_result_members_are_consistent",
                      "On a miss the three members agree: found is false, index "
                      "is lookup_npos, and ::type is lookup_not_found.",
                      &tests_misses_result_members_are_consistent },
                    { "tests_misses_alias_yields_not_found",
                      "bsearch_by_t equals lookup_not_found on a miss, matching "
                      "bsearch_by<...>::type.",
                      &tests_misses_alias_yields_not_found },
                }
            },
            dt::block_spec{
                "ordering",
                "The engine's needle-agnosticism -- the property that makes one "
                "engine serve every order. A descending pack, a type needle "
                "ordered by sizeof, deterministic duplicate-key landing, the two "
                "branch directions in isolation, the vacuous first-probe hit, "
                "and the absence of the needle from the engine's own signature.",
                {
                    { "tests_ordering_descending_pack",
                      "With the arrows flipped, a pack sorted high-to-low is "
                      "searched correctly: present keys are found at their "
                      "descending positions and absent ones miss.",
                      &tests_ordering_descending_pack },
                    { "tests_ordering_type_needle_by_sizeof",
                      "A TYPE needle with no ::key at all -- entries ordered by "
                      "sizeof -- is found and missed correctly, proving the "
                      "engine never assumes ::key.",
                      &tests_ordering_type_needle_by_sizeof },
                    { "tests_ordering_duplicate_keys_land_deterministically",
                      "When several entries share the needle's key, the engine "
                      "reports the entry the bisection lands on (index 1 of "
                      "three equal keys), deterministically -- a landing "
                      "position, not a lower_bound.",
                      &tests_ordering_duplicate_keys_land_deterministically },
                    { "tests_ordering_branch_directions_go_right",
                      "An always-below predicate pair drives the search right at "
                      "every step; it walks off the top of any pack and reports "
                      "a miss.",
                      &tests_ordering_branch_directions_go_right },
                    { "tests_ordering_branch_directions_go_left",
                      "An always-above predicate pair drives the search left at "
                      "every step; it collapses to the bottom and reports a "
                      "miss.",
                      &tests_ordering_branch_directions_go_left },
                    { "tests_ordering_vacuous_hit_at_first_probe",
                      "A never-either predicate pair makes the first probe a "
                      "hit; the engine reports that first midpoint index without "
                      "collapsing the range.",
                      &tests_ordering_vacuous_hit_at_first_probe },
                    { "tests_ordering_needle_absent_from_engine_signature",
                      "One engine instantiation, two different needle values: "
                      "searching the same pack for two needles reaches the same "
                      "bsearch_by template with only the predicates differing.",
                      &tests_ordering_needle_absent_from_engine_signature },
                }
            },
            dt::block_spec{
                "robustness",
                "Behaviour at scale and the shape of the result surface: a wide "
                "sorted pack exercising several bisection levels, members used "
                "as template arguments, stability under repeated instantiation, "
                "and independence of results for distinct needles.",
                {
                    { "tests_robustness_large_pack_all_hits",
                      "Every key in a sixteen-element sorted pack is found at its "
                      "own index, exercising four levels of bisection.",
                      &tests_robustness_large_pack_all_hits },
                    { "tests_robustness_large_pack_boundaries_and_gaps",
                      "In the same wide pack, the extremes are found and needles "
                      "below all, above all, and in interior gaps all miss.",
                      &tests_robustness_large_pack_boundaries_and_gaps },
                    { "tests_robustness_members_usable_as_template_arguments",
                      "Both ::index and ::found are constant expressions: each "
                      "instantiates a template parameterised on its type, on "
                      "both a hit and a miss.",
                      &tests_robustness_members_usable_as_template_arguments },
                    { "tests_robustness_repeated_instantiation_is_stable",
                      "The same search named several times yields one identical "
                      "type and one identical index -- the trait is a pure "
                      "function of its arguments.",
                      &tests_robustness_repeated_instantiation_is_stable },
                    { "tests_robustness_distinct_needles_distinct_results",
                      "Distinct needles over one pack give distinct indices and "
                      "distinct matched types, so results are not being shared "
                      "across instantiations.",
                      &tests_robustness_distinct_needles_distinct_results },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_BSEARCH_TESTS_
