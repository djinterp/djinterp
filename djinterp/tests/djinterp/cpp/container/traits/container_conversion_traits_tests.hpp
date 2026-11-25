/*******************************************************************************
* djinterp [cpp][container]                container_conversion_traits_tests.hpp
*
*   Declarations, shared helper functions, and helper container TYPES for the
* container_conversion_traits.hpp unit-test suite.  The section translation
* units (container_conversion_traits_tests_*.cpp) DEFINE the tests_* functions
* declared here; the runner (in the config tree) calls them.  Every test
* function is flat in djinterp::testing and returns true on success.
*
*   MECHANISM.  The traits under test are compile-time facts, so each check
* compares a constant trait value against its expected result.  A failing
* check prints the offending expression and drives the function's bool result
* to false, which the runner reports and turns into a non-zero exit code - the
* runtime contract the DTest runner is built around.
*
*   HELPER TYPES.  Two custom container-shaped types exercise branches the
* standard library cannot reach on its own:
*     - `opaque`      an element type with NO conversion to/from int, so a
*                     std::vector<opaque> is element-INCOMPATIBLE with a
*                     std::vector<int> (the `none`-via-incompatible path).
*     - `DSortedVec`  a contiguous, runtime-lifetime, unbounded int sequence
*                     that differs from std::vector<int> on EXACTLY one axis -
*                     discipline - via an opt-in `sorted_invariant`.  It is the
*                     only way to drive is_reinterpretation_helper's
*                     "discipline is strictly GREATER" branch (a strictly
*                     stronger From viewed through a weaker To).
*
*
* path:      /tests/djinterp/cpp/container/traits/container_conversion_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.15
*                                                            revised: 2026.11.15
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONVERSION_TRAITS_TESTS_
#define DJINTERP_CONTAINER_CONVERSION_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/cpp/container/traits)
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/container_conversion_traits.hpp"
#include "../../../../../inc/djinterp/cpp/modules/test/test_traits.hpp"  // the type-testing kit
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// NS_TESTING / D_KEYWORD_TESTING may not be defined by the core headers; mirror
// the defensive guard test_common.hpp uses so this header stands alone.
#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_CCV_CHECK
//   macro: the per-suite runtime check (unique letters: CCV = Container
// Conversion traits).  Records a failure into the enclosing function's
// `_ccv_ok` accumulator and prints the offending expression, so the runner's
// FAIL line is actionable.  Each `_cond` is a compile-time-constant trait
// reading, so the check is deterministic.
#define D_CCV_CHECK(_cond)                                                    \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _ccv_ok = false;                                                  \
        }                                                                     \
    } while (false)


#endif  // !DTEST_SPEC_MODE
// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// ===========================================================================
// Shared helpers
// ===========================================================================

// ccv_cstr_eq
//   helper: constexpr C-string equality, so the conversion_tier_name spellings
// can be checked directly (std::strcmp is not constexpr before C++23).  Single-
// return recursive form, valid under the C++11 constexpr rules.
constexpr
bool ccv_cstr_eq(
    const char* _a,
    const char* _b
) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : ccv_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types
// ===========================================================================

// opaque
//   type: an element with no conversion to or from int.  A std::vector<opaque>
// is therefore element-INCOMPATIBLE with a std::vector<int>, and stays flat
// (opaque is not container-shaped: no value_type, no size()).
struct opaque
{
    int m_value;
};

// DSortedVec
//   type: a contiguous, mutable, runtime-lifetime, UNBOUNDED sequence of int
// that agrees with std::vector<int> on every conversion axis EXCEPT discipline.
//
//   - value_type / size() / begin()/end() / data(): a container-shaped,
//     order-carrying (no key_type) sequence with an array_like layout
//     (data() + random-access int* iterator) and non-const iteration.
//   - the user-provided destructor makes it a non-literal (runtime-lifetime)
//     type, and size() is deliberately NOT constexpr, so BOTH its size- and
//     contents-lifetimes are runtime - matching std::vector<int>.
//   - `sorted_invariant` opts the type into the sequence-level `sorted`
//     restriction, so its overlay strictly SUBSUMES a plain sequence's: the
//     discipline relation against std::vector<int> is `greater`.
//
//   The net effect: DSortedVec -> std::vector<int> is a VIEW driven by the
// discipline-greater branch, while std::vector<int> -> DSortedVec is `none`
// (compatible elements, but neither a reinterpretation nor a construction path,
// as DSortedVec exposes no range constructor or range insert).
struct DSortedVec
{
    using value_type = int;

    static constexpr bool sorted_invariant = true;   // opt-in sigma

    int         m_buf[8];
    std::size_t m_size;

    ~DSortedVec() noexcept {}                         // non-trivial -> runtime lifetime

    std::size_t size() const noexcept { return m_size; }

    int*        begin()       noexcept { return m_buf; }
    int*        end()         noexcept { return m_buf + m_size; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + m_size; }
    const int*  data()  const noexcept { return m_buf; }
};

// ===========================================================================
// The D_CVT_ fixture (the paths, robustness, stdlib and view sections)
// ===========================================================================
//   Rebuilt on 2026.11.15.  The four sections of 2026.10.15 were
// written against a fixture header that never reached the tree; this is it,
// reconstructed from what the sections ask of each name.  Every type below
// states the axes it sits on, so a section's expectation can be read against
// it.

// D_CVT_CHECK
//   macro: one check in a D_CVT_ section.  Records a failure into the
// enclosing function's `ok` and prints the expression.  Variadic, so a
// template argument list needs no extra parentheses.
#define D_CVT_CHECK(...)                                                      \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    - check failed: %s:%d: %s\n",                    \
                        __FILE__, __LINE__, #__VA_ARGS__);                    \
            ok = false;                                                       \
        }                                                                     \
    }                                                                         \
    while (0)

// D_CVT_TIER
//   macro: checks conversion_tier_of<FROM, TO> against an expected tier.
#define D_CVT_TIER(EXPECTED, FROM, TO)                                        \
    D_CVT_CHECK((::djinterp::conversion_tier_of<FROM, TO>::value ==           \
                 (EXPECTED)))

// D_CVT_LEVEL
//   macro: checks a content_level reading against an expected rung.
#define D_CVT_LEVEL(EXPECTED, ...)                                            \
    D_CVT_CHECK(((__VA_ARGS__) == (EXPECTED)))

// the type-testing kit (DTest's test_traits.hpp), by the names the sections use
using ::djinterp::test::count_holds;
using ::djinterp::test::holds_for_all;
using ::djinterp::test::is_bool_trait;
using ::djinterp::test::trait_across_cvref;
using ::djinterp::test::trait_ignores_cvref;
using ::djinterp::test::trait_is_well_formed;
namespace fixtures = ::djinterp::test::fixtures;

// the standard library, by short names
using std_vector      = std::vector<int>;
using std_vector_long = std::vector<long>;
using std_vec_vec     = std::vector<std::vector<int>>;
using std_list        = std::list<int>;
using std_deque       = std::deque<int>;
using std_fwd_list    = std::forward_list<int>;
using std_set         = std::set<int>;
using std_multiset    = std::multiset<int>;
using std_map         = std::map<int, int>;
using std_str         = std::string;
using std_array       = std::array<int, 4>;
using std_array_64    = std::array<int, 64>;
using raw_array       = int[4];

// cvt_seq
//   type: the shared body of the sequence fixtures.  A flat sequence of Elem
// (no key_type), array_like (data() and pointer iterators), iterable through
// a non-const object, unbounded, of runtime lifetime (a user-provided
// destructor makes it non-literal, and size() is not constexpr), with no
// storage siting declared.  It has no range constructor and no insert.
template<typename Elem>
struct cvt_seq
{
    using value_type = Elem;

    Elem        m_buf[8] = {};
    std::size_t m_size   = 0;

    ~cvt_seq() noexcept {}

    std::size_t size()  const noexcept { return m_size; }
    Elem*       begin()       noexcept { return m_buf; }
    Elem*       end()         noexcept { return m_buf + m_size; }
    const Elem* begin() const noexcept { return m_buf; }
    const Elem* end()   const noexcept { return m_buf + m_size; }
    Elem*       data()        noexcept { return m_buf; }
    const Elem* data()  const noexcept { return m_buf; }
};

// cvt_from_range
//   helper: fills a cvt_seq-shaped buffer from an iterator range (at most 8).
template<typename Seq,
         typename It>
void cvt_from_range(
    Seq& _seq,
    It   _first,
    It   _last
)
{
    for (; (_first != _last) && (_seq.m_size < 8); ++_first)
    {
        _seq.m_buf[_seq.m_size++] = *_first;
    }
}

// view_from
//   type: THE BASELINE From.  cvt_seq<int>: no range constructor, no insert.
struct view_from : cvt_seq<int>
{
};

// view_to
//   type: THE BASELINE To.  view_from on every axis the view predicate reads,
// plus a range constructor, so a construction path ALSO exists.
struct view_to : cvt_seq<int>
{
    view_to() = default;

    template<typename It>
    view_to(It _first, It _last)
    {
        cvt_from_range(*this, _first, _last);
    }
};

// lever_elements
//   type: view_to with long elements - compatible with int, not the same.
struct lever_elements : cvt_seq<long>
{
    lever_elements() = default;

    template<typename It>
    lever_elements(It _first, It _last)
    {
        cvt_from_range(*this, _first, _last);
    }
};

// lever_storage
//   type: view_to that declares a static siting, where view_to declares none.
struct lever_storage : view_to
{
    using view_to::view_to;

    static constexpr DStorageDuration::value storage_duration_category =
        DStorageDuration::static_storage;
};

// lever_discipline
//   type: view_to that opts into the sorted invariant, so its discipline is
// strictly stronger than view_from's.
struct lever_discipline : view_to
{
    using view_to::view_to;

    static constexpr bool sorted_invariant = true;
};

// lever_iterability
//   type: view_to iterable through a const object only (const_only mode).
struct lever_iterability
{
    using value_type = int;

    int         m_buf[8] = {};
    std::size_t m_size   = 0;

    lever_iterability() = default;

    template<typename It>
    lever_iterability(It _first, It _last)
    {
        cvt_from_range(*this, _first, _last);
    }

    ~lever_iterability() noexcept {}

    std::size_t size()  const noexcept { return m_size; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + m_size; }
    const int*  data()  const noexcept { return m_buf; }
};

// lever_layout
//   type: view_to with a list_like layout (bidirectional iterators, no
// data()), the same in every other respect.
struct lever_layout
{
    using value_type = int;

    std::list<int> m_items;

    lever_layout() = default;

    template<typename It>
    lever_layout(It _first, It _last)
        : m_items(_first, _last)
    {}

    ~lever_layout() noexcept {}

    std::size_t size() const noexcept { return m_items.size(); }

    std::list<int>::iterator       begin()       noexcept
    {
        return m_items.begin();
    }

    std::list<int>::iterator       end()         noexcept
    {
        return m_items.end();
    }

    std::list<int>::const_iterator begin() const noexcept
    {
        return m_items.begin();
    }

    std::list<int>::const_iterator end()   const noexcept
    {
        return m_items.end();
    }
};

// lever_topology
//   type: a sequence whose elements are themselves containers, so it is
// hierarchical where view_from is flat.
struct lever_topology : cvt_seq<std::vector<int>>
{
    lever_topology() = default;

    template<typename It>
    lever_topology(It _first, It _last)
    {
        for (; (_first != _last) && (m_size < 8); ++_first)
        {
            m_buf[m_size++] = std::vector<int>(1, *_first);
        }
    }
};

// content_from / content_to
//   type: a flat SET (rung `DSet`) and a flat BAG (rung `bag`) of int, each
// with a range constructor.  To is the finer, so nothing is forgotten.
struct content_from : std::set<int>
{
    using std::set<int>::set;
};

struct content_to : std::multiset<int>
{
    using std::multiset<int>::multiset;
};

// lossy_forget / lossy_clamp
//   type: the two lossy Tos.  lossy_forget is a set (the conversion forgets
// order and multiplicity); lossy_clamp is view_to with a capacity() and no
// reserve(), so it is bounded (the conversion clamps).
struct lossy_forget : std::set<int>
{
    using std::set<int>::set;
};

struct lossy_clamp : view_to
{
    using view_to::view_to;

    std::size_t capacity() const noexcept { return 8; }
};

// insert_only
//   type: a list_like sequence with a two-argument insert(first, last) and no
// range constructor: the insert path alone.
struct insert_only : lever_layout
{
    insert_only() = default;

    template<typename It>
    void insert(It _first, It _last)
    {
        m_items.insert(m_items.end(), _first, _last);
    }
};

// no_path
//   type: a list_like sequence of int with neither a range constructor nor an
// insert: compatible elements, no way to build it, and not a view.
struct no_path
{
    using value_type = int;

    std::list<int> m_items;

    ~no_path() noexcept {}

    std::size_t size() const noexcept { return m_items.size(); }

    std::list<int>::const_iterator begin() const noexcept
    {
        return m_items.begin();
    }

    std::list<int>::const_iterator end()   const noexcept
    {
        return m_items.end();
    }
};

// nc_runtime / nc_runtime2 / nc_compile
//   type: non-containers.  nc_runtime and nc_runtime2 are identical and of
// runtime lifetime; nc_compile opts into the compile-time lifetime tag.
struct nc_runtime
{
    int m_value = 0;

    ~nc_runtime() noexcept {}
};

struct nc_runtime2
{
    int m_value = 0;

    ~nc_runtime2() noexcept {}
};

struct nc_compile
{
    using is_constexpr_container = std::true_type;

    int m_value = 0;
};

// fn_type
//   type: a function type, for the robustness section.
using fn_type = int(int);

#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// - I.   Tier vocabulary (enum + conversion_tier_name) -
bool tests_conversion_tier_vocabulary();

// - II.  Construction-path detection -
bool tests_range_constructible();
bool tests_range_insertable();
bool tests_has_constructive_path();

// - III. Per-concern analysis -
bool tests_elements_compatible();
bool tests_conversion_forgets_content();
bool tests_conversion_clamps_capacity();
bool tests_conversion_changes_topology();

// - IV.  Conversion classification -
bool tests_conversion_tier_of_view();
bool tests_conversion_tier_of_constructive();
bool tests_conversion_tier_of_lossy();
bool tests_conversion_tier_of_structural();
bool tests_conversion_tier_of_none();
bool tests_conversion_tier_of_cvref();
bool tests_conversion_predicates();

// - IV.  Aggregate + variable-template companions -
bool tests_conversion_class();
bool tests_conversion_variable_templates();

// - V.   Construction paths, in depth (container_conversion_traits_tests_paths.cpp) -
bool tests_path_range_constructible();
bool tests_path_range_insertable();
bool tests_path_iterable_gate();
bool tests_path_has_constructive_path();

// - VI.  The view predicate, clause by clause (..._view.cpp) -
bool tests_view_baseline();
bool tests_view_conjunct_elements();
bool tests_view_conjunct_content();
bool tests_view_conjunct_topology();
bool tests_view_conjunct_layout();
bool tests_view_conjunct_lifetime();
bool tests_view_conjunct_storage();
bool tests_view_conjunct_iterability();
bool tests_view_conjunct_discipline();
bool tests_view_discipline_asymmetry();
bool tests_view_non_container();

// - VII. The standard library (..._stdlib.cpp) -
bool tests_stdlib_identity();
bool tests_stdlib_sequences();
bool tests_stdlib_associative();
bool tests_stdlib_array();
bool tests_stdlib_array_extent();
bool tests_stdlib_forward_list();
bool tests_stdlib_structural();

// - VIII. Robustness: cv-ref, hostile and incomplete types (..._robustness.cpp) -
bool tests_cvref();
bool tests_hostile_types();
bool tests_incomplete_type();
bool tests_trait_shape();


// - the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// container_conversion_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
container_conversion_traits_spec()
{
    return dt::module_spec{
        "container_conversion_traits",
        "conversion - the carrying of a container of one type to another - is "
        "an operation FOUNDED on...",
        {
            dt::block_spec{
                "tier",
                "Tier",
                {
                    { "tests_conversion_tier_vocabulary",
                      "checks "
                      "ccv_cstr_eq(conversion_tier_name(DConversionTier::view), "
                      "\"view\")",
                      &tests_conversion_tier_vocabulary },
                }
            },
            dt::block_spec{
                "construction_path",
                "Construction path",
                {
                    { "tests_range_constructible",
                      "checks (is_range_constructible<std::vector<int>, "
                      "std::vector<int>>::value)",
                      &tests_range_constructible },
                    { "tests_range_insertable",
                      "checks (is_range_insertable<std::vector<int>, "
                      "std::set<int>>::value)",
                      &tests_range_insertable },
                    { "tests_has_constructive_path",
                      "checks (has_constructive_path<std::vector<int>, "
                      "std::vector<int>>::value)",
                      &tests_has_constructive_path },
                }
            },
            dt::block_spec{
                "concerns",
                "Concerns",
                {
                    { "tests_elements_compatible",
                      "checks (elements_compatible<std::vector<int>, "
                      "std::vector<int>>::value)",
                      &tests_elements_compatible },
                    { "tests_conversion_forgets_content",
                      "checks (conversion_forgets_content<std::vector<int>, "
                      "std::set<int>>::value)",
                      &tests_conversion_forgets_content },
                    { "tests_conversion_clamps_capacity",
                      "checks (conversion_clamps_capacity<std::vector<int>, "
                      "std::array<int, 4>>::",
                      &tests_conversion_clamps_capacity },
                    { "tests_conversion_changes_topology",
                      "checks (conversion_changes_topology<std::vector<int>, "
                      "std::vector<std::vec",
                      &tests_conversion_changes_topology },
                }
            },
            dt::block_spec{
                "classification",
                "Classification",
                {
                    { "tests_conversion_tier_of_view",
                      "checks (conversion_tier_of<std::vector<int>, "
                      "std::vector<int>>::value == D",
                      &tests_conversion_tier_of_view },
                    { "tests_conversion_tier_of_constructive",
                      "checks (conversion_tier_of<std::vector<int>, "
                      "std::list<int>>::value == DCo",
                      &tests_conversion_tier_of_constructive },
                    { "tests_conversion_tier_of_lossy",
                      "checks (conversion_tier_of<std::vector<int>, "
                      "std::set<int>>::value == DCon",
                      &tests_conversion_tier_of_lossy },
                    { "tests_conversion_tier_of_structural",
                      "checks (conversion_tier_of<std::vector<int>, "
                      "std::vector<std::vector<int>>",
                      &tests_conversion_tier_of_structural },
                    { "tests_conversion_tier_of_none",
                      "checks (conversion_tier_of<std::vector<int>, "
                      "std::vector<opaque>>::value =",
                      &tests_conversion_tier_of_none },
                    { "tests_conversion_tier_of_cvref",
                      "checks (conversion_tier_of<const std::vector<int>&, "
                      "std::vector<int>>::val",
                      &tests_conversion_tier_of_cvref },
                    { "tests_conversion_predicates",
                      "checks (is_convertible_between<std::vector<int>, "
                      "std::vector<int>>::value)",
                      &tests_conversion_predicates },
                }
            },
            dt::block_spec{
                "aggregate",
                "Aggregate",
                {
                    { "tests_conversion_class",
                      "checks (DConversionClass<std::vector<int>, "
                      "std::vector<int>>::tier == DCon",
                      &tests_conversion_class },
                    { "tests_conversion_variable_templates",
                      "checks (conversion_tier_of_v<std::vector<int>, "
                      "std::vector<int>> == conver",
                      &tests_conversion_variable_templates },
                }
            },
            dt::block_spec{
                "paths",
                "Construction paths in depth: the range constructor, the "
                "two-argument insert, the iterable gate both share, and their "
                "disjunction, on fixtures and the standard library.",
                {
                    { "tests_path_range_constructible",
                      "is_range_constructible: To(begin, end); std::array has "
                      "none.",
                      &tests_path_range_constructible },
                    { "tests_path_range_insertable",
                      "is_range_insertable: insert(first, last); the sequences "
                      "have none.",
                      &tests_path_range_insertable },
                    { "tests_path_iterable_gate",
                      "Both path traits reject a non-iterable From first.",
                      &tests_path_iterable_gate },
                    { "tests_path_has_constructive_path",
                      "has_constructive_path is exactly their disjunction.",
                      &tests_path_has_constructive_path },
                }
            },
            dt::block_spec{
                "view",
                "The view predicate, clause by clause: a baseline pair, one "
                "lever per clause, the discipline asymmetry, and non-containers.",
                {
                    { "tests_view_baseline",
                      "view_from -> view_to is a view, on all clauses.",
                      &tests_view_baseline },
                    { "tests_view_conjunct_elements",
                      "Different elements: constructive, not a view.",
                      &tests_view_conjunct_elements },
                    { "tests_view_conjunct_content",
                      "A finer content rung: constructive; coarser: lossy.",
                      &tests_view_conjunct_content },
                    { "tests_view_conjunct_topology",
                      "A topology change is structural, whatever else holds.",
                      &tests_view_conjunct_topology },
                    { "tests_view_conjunct_layout",
                      "vector -> list: one layout apart, constructive.",
                      &tests_view_conjunct_layout },
                    { "tests_view_conjunct_lifetime",
                      "Lifetime: reachable alone only between non-iterables.",
                      &tests_view_conjunct_lifetime },
                    { "tests_view_conjunct_storage",
                      "A declared siting against none: constructive.",
                      &tests_view_conjunct_storage },
                    { "tests_view_conjunct_iterability",
                      "Mutable vs const-only iteration: not a view.",
                      &tests_view_conjunct_iterability },
                    { "tests_view_conjunct_discipline",
                      "A weaker From: constructive.",
                      &tests_view_conjunct_discipline },
                    { "tests_view_discipline_asymmetry",
                      "A stronger From: a view; the reverse is not.",
                      &tests_view_discipline_asymmetry },
                    { "tests_view_non_container",
                      "Two different non-containers are no conversion; the "
                      "identity is still a view.",
                      &tests_view_non_container },
                }
            },
            dt::block_spec{
                "stdlib",
                "The standard containers against each other: identity, "
                "sequences, associatives, std::array and its extents, "
                "forward_list, and the flat / hierarchical divide.",
                {
                    { "tests_stdlib_identity",
                      "Every standard container is a view of itself.",
                      &tests_stdlib_identity },
                    { "tests_stdlib_sequences",
                      "vector, list, deque: constructive between them.",
                      &tests_stdlib_sequences },
                    { "tests_stdlib_associative",
                      "set, multiset, map; the two canonical quotients.",
                      &tests_stdlib_associative },
                    { "tests_stdlib_array",
                      "std::array: bounded, and without a range constructor.",
                      &tests_stdlib_array },
                    { "tests_stdlib_array_extent",
                      "Different extents: narrowing lossy, widening none.",
                      &tests_stdlib_array_extent },
                    { "tests_stdlib_forward_list",
                      "Copies into and out of forward_list are constructive.",
                      &tests_stdlib_forward_list },
                    { "tests_stdlib_structural",
                      "vector<vector<int>> against flat containers.",
                      &tests_stdlib_structural },
                }
            },
            dt::block_spec{
                "robustness",
                "Every trait over cv and references, hostile types and an "
                "incomplete type, and each trait's shape.",
                {
                    { "tests_cvref",
                      "Each trait answers the same through cv and refs.",
                      &tests_cvref },
                    { "tests_hostile_types",
                      "Every adversarial shape is reported, not diagnosed.",
                      &tests_hostile_types },
                    { "tests_incomplete_type",
                      "The path traits tolerate an incomplete type.",
                      &tests_incomplete_type },
                    { "tests_trait_shape",
                      "Each trait is a well-formed bool trait.",
                      &tests_trait_shape },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONVERSION_TRAITS_TESTS_
