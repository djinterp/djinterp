/******************************************************************************
* djinterp [test]                                     test_container_tests.hpp
*
*   Declarations for the unit-test suite covering test_container.hpp.  Each
* free function exercises one facet of the test_container contract - the
* trait + concept surface that detects whether an arbitrary container may
* HOLD and RUN test_objects - and returns true iff every check inside it
* passed.  Tests are grouped into translation units by the section of
* test_container.hpp they cover:
*
*   - test_container_tests_probes.cpp    -> I.   structural member probes
*   - test_container_tests_element.cpp   -> II.  element protocol
*   - test_container_tests_contract.cpp  -> III. test-container contract
*   - test_container_tests_concepts.cpp  -> IV.  concepts (C++20+)
*
*   The lone shared check helper, test_container_check, reports a failing
* check (with its stringized expression and source location) and forwards the
* boolean.  The D_TC_CHECK macro is the intended call site.
*
*   WHAT IS UNDER TEST IS COMPILE-TIME:
*   test_container.hpp is entirely traits and concepts, so every check is a
* comparison of a compile-time constant (a trait's ::value, a `_v` companion,
* or a concept) against its expected boolean.  The checks are therefore
* compile-time-evaluated and runtime-reported: routing each through
* test_container_check keeps the probes, the contract, and the concepts on the
* one uniform reporting path the six-kind tree and the report/PDF read back.
*
*   FIXTURES.  The container fixtures are hand-rolled minimal types, each
* carrying exactly the subset of the detected surface a given check needs, so
* every structural probe and every rung of the contract ladder is exercised in
* isolation - adding OR withholding a single member flips exactly one result.
* The element fixtures (mini_evaluable / not_evaluable, plus the framework's
* own basic_test and a std::vector of it) drive the element protocol both ways.
* object_container_min deliberately exposes NON-const begin()/end() so the
* documented raw-probe-vs-contract normalization asymmetry can be witnessed.
*
*   NOTE: the entities under test live in djinterp::test (and
* djinterp::test::internal); the tests themselves live, flat, in
* djinterp::testing.  The alias `dt` below binds the former.
*
*
* path:      /tests/djinterp/test/test_container/test_container_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_TEST_CONTAINER_TESTS_
#define DJINTERP_TEST_CONTAINER_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>
// djinterp  -- framework header FIRST, then the header under test (normal mode)
// or the enriched authoring surface (spec mode).
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_container.hpp>   // the contract under test
#endif
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>    // spec mode: module_spec + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt
//   alias: the namespace the entities under test live in.  Declared
// unconditionally (both modes) so the fixtures, the section-file test bodies,
// and the spec provider all name the contract through one spelling.
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// test_container_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_container_check(
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


// =========================================================================
//  shared fixtures -- element types (drive the element protocol both ways)
// =========================================================================

// mini_evaluable
//   fixture: a hand-rolled type satisfying the test object protocol
// (is_test_evaluable) structurally - the six const-lvalue members a runner
// reads - WITHOUT being basic_test.  Proves the element protocol, and thus
// container_element_evaluable's specialization, is structural, not nominal.
struct mini_evaluable
{
    explicit operator bool()   const { return true; }
    int           status()     const { return 0; }
    bool          result()     const { return true; }
    std::int32_t  type_id()    const { return 0; }
    std::uint32_t callable_id()const { return 0; }
    int           metadata()   const { return 0; }
};

// not_evaluable
//   fixture: a plain type satisfying none of the element protocol.  Its
// presence as a value_type makes a container fail the element gate.
struct not_evaluable
{
    int x;
};

// tc_node
//   fixture: a stand-in node type, used as the node_type alias the
// append_child probe names (append_child(node_type*, value_type)).
struct tc_node
{
    dt::basic_test data;
};


// =========================================================================
//  shared fixtures -- container stand-ins (each carries exactly one subset)
// =========================================================================

// object_container_min
//   fixture: the MINIMUM object container - value_type (an evaluable
// element), begin()/end(), size(), empty().  Satisfies
// is_test_object_container; rooted / buildable both fail (no root, no
// append_child).  begin()/end() are NON-const on purpose: a const-qualified
// probe of them fails, which is what makes the raw-probe-vs-contract
// asymmetry observable.
struct object_container_min
{
    using value_type = dt::basic_test;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    std::size_t size()  const { return 0; }
    bool        empty() const { return true; }
};

// rooted_container_min
//   fixture: object_container_min PLUS root().  Satisfies
// is_rooted_test_container; buildable still fails (no append_child).
struct rooted_container_min
{
    using value_type = dt::basic_test;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    std::size_t size()  const { return 0; }
    bool        empty() const { return true; }
    void        root()        {}
};

// buildable_container_min
//   fixture: the full ladder - value_type, node_type, begin()/end(), size(),
// empty(), root(), and append_child(node_type*, value_type).  Mirrors the
// surface test_tree presents; satisfies is_buildable_test_container.
struct buildable_container_min
{
    using value_type = dt::basic_test;
    using node_type  = tc_node;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    std::size_t size()  const { return 0; }
    bool        empty() const { return true; }
    node_type*  root()        { return nullptr; }
    node_type*  append_child(node_type*, value_type) { return nullptr; }
};

// missing_value_type
//   fixture: object container floor MINUS value_type.  has_value_type fails,
// so is_test_object_container fails; also the type for which
// container_element_evaluable's guarded PRIMARY (no value_type) must stay
// well-formed and false.
struct missing_value_type
{
    dt::basic_test* begin()       { return nullptr; }
    dt::basic_test* end()         { return nullptr; }
    std::size_t     size()  const { return 0; }
    bool            empty() const { return true; }
};

// missing_begin_end
//   fixture: object container floor MINUS begin()/end().
struct missing_begin_end
{
    using value_type = dt::basic_test;

    std::size_t size()  const { return 0; }
    bool        empty() const { return true; }
};

// missing_size
//   fixture: object container floor MINUS size().
struct missing_size
{
    using value_type = dt::basic_test;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    bool        empty() const { return true; }
};

// missing_empty
//   fixture: object container floor MINUS empty().
struct missing_empty
{
    using value_type = dt::basic_test;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    std::size_t size()  const { return 0; }
};

// non_evaluable_element
//   fixture: a full object container floor whose value_type is NOT evaluable
// (a plain int).  Isolates the element gate: every structural member is
// present, yet is_test_object_container fails on the element protocol alone,
// and it drives container_element_evaluable's SPECIALIZATION to false.
struct non_evaluable_element
{
    using value_type = int;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    std::size_t size()  const { return 0; }
    bool        empty() const { return true; }
};

// root_only
//   fixture: exposes root() and nothing else relevant.  has_root_method true;
// is_rooted_test_container false (the object-container floor is absent) -
// proving root() alone is insufficient.
struct root_only
{
    void root() {}
};

// clear_only
//   fixture: exposes clear() and nothing else.  has_clear_method true.
// (has_clear_method is wired to no contract predicate, so it is only
// reachable by a direct probe test.)
struct clear_only
{
    void clear() {}
};

// plain_empty
//   fixture: an empty struct - the universal negative.  Every structural
// probe and every contract predicate is false for it.
struct plain_empty
{
};

// tc_vector_of_tests
//   fixture: a std::vector of the framework's basic_test - a real standard
// container whose element IS evaluable.  A positive for
// is_test_object_container (value_type / begin/end / size / empty all present)
// that is nonetheless neither rooted nor buildable (no root / no node_type /
// no append_child), and whose clear() gives has_clear_method a real positive.
typedef std::vector<dt::basic_test> tc_vector_of_tests;

// tc_vector_of_int
//   fixture: a std::vector<int> - a real standard container whose element is
// NOT evaluable.  A negative for is_test_object_container that fails on the
// element gate alone.
typedef std::vector<int> tc_vector_of_int;

// append_wrong_arity
//   fixture: names value_type / node_type and an append_child of the WRONG
// arity (one argument).  has_append_child_method must reject it - the probe
// detects the two-argument (node_type*, value_type) shape specifically.
struct append_wrong_arity
{
    using value_type = dt::basic_test;
    using node_type  = tc_node;

    void append_child(value_type) {}
};

// append_no_node_type
//   fixture: has a two-argument append_child but NO node_type alias.  Because
// the probe names _Type::node_type, substitution fails and
// has_append_child_method is false - a hard error is not permitted here.
struct append_no_node_type
{
    using value_type = dt::basic_test;

    void append_child(tc_node*, value_type) {}
};

// append_no_root
//   fixture: a growable container MINUS root() - full object-container floor
// plus append_child, but no root().  has_append_child_method is true, yet
// is_buildable_test_container is false: buildable is layered on the rooted
// floor, so the missing root() defeats it.
struct append_no_root
{
    using value_type = dt::basic_test;
    using node_type  = tc_node;

    value_type* begin()       { return nullptr; }
    value_type* end()         { return nullptr; }
    std::size_t size()  const { return 0; }
    bool        empty() const { return true; }
    node_type*  append_child(node_type*, value_type) { return nullptr; }
};


#endif  // !DTEST_SPEC_MODE (fixtures + check helper)


// =========================================================================
//  test declarations
// =========================================================================

// I.   structural member probes
bool tests_probe_has_value_type();
bool tests_probe_has_size_accessor();
bool tests_probe_has_empty_method();
bool tests_probe_has_begin_end();
bool tests_probe_has_root_method();
bool tests_probe_has_clear_method();
bool tests_probe_has_append_child_method();
bool tests_probe_value_type_cv_normalized();
bool tests_probe_v_companions();

// II.  element protocol
bool tests_element_evaluable_positive();
bool tests_element_evaluable_negative();
bool tests_element_guard_primary_no_value_type();
bool tests_element_guard_specialization_evaluable();
bool tests_element_guard_specialization_non_evaluable();
bool tests_element_guard_wellformed_without_value_type();

// III. test-container contract
bool tests_contract_object_positive();
bool tests_contract_object_missing_members();
bool tests_contract_object_element_gate();
bool tests_contract_rooted_positive_negative();
bool tests_contract_buildable_positive_negative();
bool tests_contract_ladder_monotonicity();
bool tests_contract_cv_ref_normalization();
bool tests_contract_raw_probe_vs_contract_asymmetry();
bool tests_contract_v_companions();

// IV.  concepts (C++20+)
bool tests_concepts_structural_positive();
bool tests_concepts_structural_negative();
bool tests_concepts_contract_positive();
bool tests_concepts_contract_negative();
bool tests_concepts_mirror_traits();
bool tests_concepts_cv_ref_normalization();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Mirrors this suite as data - each unit test paired with its name and a
//  one-line descriptor - so run_module / run_suite can lower it into the
//  six-kind tree and drive the report.  References only the bool() test
//  declarations above; no fixtures required, which is what lets an aggregate
//  runner include this suite header alongside others in one TU.
// =========================================================================

inline dt::module_spec
container_spec()
{
    return dt::module_spec{
        "test_container",
        "The test_container contract: the structural member probes, the "
        "guarded element protocol, the three-rung read/run/build ladder, "
        "and the C++20 concept mirror.",
        {
            dt::block_spec{
                "probes",
                "Structural member probes: value_type, size, empty, begin/end, "
                "root, clear, append_child (and their _v companions).",
                {
                    { "probe_has_value_type",
                      "has_value_type detects a nested value_type alias",
                      &tests_probe_has_value_type },
                    { "probe_has_size_accessor",
                      "has_size_accessor detects size() on a const lvalue",
                      &tests_probe_has_size_accessor },
                    { "probe_has_empty_method",
                      "has_empty_method detects empty() on a const lvalue",
                      &tests_probe_has_empty_method },
                    { "probe_has_begin_end",
                      "has_begin_end detects begin()/end() on a non-const lvalue",
                      &tests_probe_has_begin_end },
                    { "probe_has_root_method",
                      "has_root_method detects root()",
                      &tests_probe_has_root_method },
                    { "probe_has_clear_method",
                      "has_clear_method detects clear()",
                      &tests_probe_has_clear_method },
                    { "probe_has_append_child_method",
                      "has_append_child_method detects append_child(node_type*, value_type)",
                      &tests_probe_has_append_child_method },
                    { "probe_value_type_cv_normalized",
                      "has_value_type strips cv/ref before inspecting",
                      &tests_probe_value_type_cv_normalized },
                    { "probe_v_companions",
                      "each probe's _v companion agrees with ::value",
                      &tests_probe_v_companions },
                }
            },
            dt::block_spec{
                "element",
                "The guarded element protocol (container_element_evaluable).",
                {
                    { "element_evaluable_positive",
                      "is_test_evaluable holds for elements meeting the protocol",
                      &tests_element_evaluable_positive },
                    { "element_evaluable_negative",
                      "is_test_evaluable rejects elements missing the protocol",
                      &tests_element_evaluable_negative },
                    { "element_guard_primary_no_value_type",
                      "the primary (no value_type) is false",
                      &tests_element_guard_primary_no_value_type },
                    { "element_guard_specialization_evaluable",
                      "the specialization defers to is_test_evaluable (evaluable element)",
                      &tests_element_guard_specialization_evaluable },
                    { "element_guard_specialization_non_evaluable",
                      "the specialization is false for a non-evaluable element",
                      &tests_element_guard_specialization_non_evaluable },
                    { "element_guard_wellformed_without_value_type",
                      "naming the guard on a type without value_type is well-formed",
                      &tests_element_guard_wellformed_without_value_type },
                }
            },
            dt::block_spec{
                "contract",
                "The layered contract: is_test_object_container, "
                "is_rooted_test_container, is_buildable_test_container.",
                {
                    { "contract_object_positive",
                      "is_test_object_container accepts a minimal object container",
                      &tests_contract_object_positive },
                    { "contract_object_missing_members",
                      "withholding any one member defeats is_test_object_container",
                      &tests_contract_object_missing_members },
                    { "contract_object_element_gate",
                      "a non-evaluable element defeats is_test_object_container",
                      &tests_contract_object_element_gate },
                    { "contract_rooted_positive_negative",
                      "is_rooted_test_container adds root() atop the object floor",
                      &tests_contract_rooted_positive_negative },
                    { "contract_buildable_positive_negative",
                      "is_buildable_test_container adds append_child() atop the rooted floor",
                      &tests_contract_buildable_positive_negative },
                    { "contract_ladder_monotonicity",
                      "buildable implies rooted implies object container",
                      &tests_contract_ladder_monotonicity },
                    { "contract_cv_ref_normalization",
                      "the contract predicates strip cv/ref before inspecting",
                      &tests_contract_cv_ref_normalization },
                    { "contract_raw_probe_vs_contract_asymmetry",
                      "raw probes query the type directly; the contract normalizes",
                      &tests_contract_raw_probe_vs_contract_asymmetry },
                    { "contract_v_companions",
                      "each contract predicate's _v companion agrees with ::value",
                      &tests_contract_v_companions },
                }
            },
            dt::block_spec{
                "concepts",
                "The C++20 concept layer mirroring the traits.",
                {
                    { "concepts_structural_positive",
                      "the structural concepts hold for satisfying types",
                      &tests_concepts_structural_positive },
                    { "concepts_structural_negative",
                      "the structural concepts reject non-satisfying types",
                      &tests_concepts_structural_negative },
                    { "concepts_contract_positive",
                      "the contract concepts hold across the ladder",
                      &tests_concepts_contract_positive },
                    { "concepts_contract_negative",
                      "the contract concepts reject non-qualifying types",
                      &tests_concepts_contract_negative },
                    { "concepts_mirror_traits",
                      "each concept agrees with its trait by construction",
                      &tests_concepts_mirror_traits },
                    { "concepts_cv_ref_normalization",
                      "the concepts strip cv/ref before inspecting",
                      &tests_concepts_cv_ref_normalization },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_TC_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through test_container_check, capturing the expression text and source
// location.  Variadic so an expression containing top-level commas (e.g. a
// multi-argument template-id like foo<a, b>::value) needs no defensive
// parentheses.
#define D_TC_CHECK(...)                                                       \
    ::djinterp::testing::test_container_check(                                \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_CONTAINER_TESTS_
