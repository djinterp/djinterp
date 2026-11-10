/*******************************************************************************
* djinterp [testing]                   observational_comparison_traits_tests.hpp
*
* Unit tests for observational_comparison_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the observational
* comparison trait suite.  Every test function is declared here and defined in
* the section .cpp that matches the section of the header under test.
*
*   WHAT IS UNDER TEST.  Not "are these two containers equal" but "through
* which of their shared interface operations could they be told apart".  The
* header maps an inspection surface to the finest content level it decides -
* at(.)+shape resolves str, iteration resolves seq, count resolves bag,
* contains resolves set, size alone resolves size, nothing resolves none - and
* then takes the COARSER of two types' levels as the level at which a pair can
* actually be compared.
*
*   THE CENTRAL CASE the fixtures are built around is the gap between a type's
* native content level and what its interface observationally resolves: a
* std::set natively compares at =set, yet iterating it exposes its canonical
* order, so its interface resolves seq.  tests_observational_level_of_set_gap
* pins that, and the cascade fixtures below isolate every other rung.
*
*   FIXTURE NAMING.  Fixtures are prefixed oct_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.  Standard-library
* carriers with comma-bearing argument lists are typedef'd here for the same
* reason.
*
*
* path:      /tests/djinterp/cpp/container/traits/observational_comparison_traits_tests.hpp
*                                    observational_comparison_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Level-resolving fixtures  (one per rung of the cascade)
      -------------------------------------------------------

III.  Cascade precedence probes (types that light several rungs at once)
      ------------------------------------------------------------------

IV.   Probe shape fixtures      (what counts as "has the operation")
      --------------------------------------------------------------

V.    Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

VI.   Static-size fixtures      (the stage proxy)
      -------------------------------------------

VII.  Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_OBSERVATIONAL_COMPARISON_TRAITS_
#define DJINTERP_TESTS_OBSERVATIONAL_COMPARISON_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <forward_list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/observational_comparison_traits.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// oct_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = oct_check_failed(...)` on the failing path only.
inline
bool oct_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_OC_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_OC_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::oct_check_failed(#__VA_ARGS__,          \
                                                       __FILE__,              \
                                                       __LINE__);             \
        }                                                                     \
    } while (0)

// D_OC_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_OC_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_OC_CV_REF_AGREE
//   macro: every trait in this header writes clean_t before asking its
// question, so all seven cv- and reference-qualified spellings of a type must
// answer alike.  ONE macro suffices for the whole header, which is itself the
// property under test - a sibling trait header phrases some questions on the
// unstripped type and does not have this uniformity.  Not usable on void or a
// function type, since `void&` cannot be formed.
#define D_OC_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_OC_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_OC_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_OC_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_OC_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_OC_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_OC_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_OC_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// D_OC_CV_REF_PAIR_AGREE
//   macro: the same, for the two-argument traits.  The left operand is varied
// through all seven spellings against a fixed right operand, then the right
// through all seven against a fixed left, which is enough to show each side is
// cleaned independently.
#define D_OC_CV_REF_PAIR_AGREE(TRAIT, LEFT, RIGHT, EXPECT)                    \
    do                                                                        \
    {                                                                         \
        D_OC_CHECK(TRAIT<LEFT, RIGHT>::value                == (EXPECT));     \
        D_OC_CHECK(TRAIT<const LEFT, RIGHT>::value          == (EXPECT));     \
        D_OC_CHECK(TRAIT<volatile LEFT, RIGHT>::value       == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT&, RIGHT>::value               == (EXPECT));     \
        D_OC_CHECK(TRAIT<const LEFT&, RIGHT>::value         == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT&&, RIGHT>::value              == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT, const RIGHT>::value          == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT, volatile RIGHT>::value       == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT, RIGHT&>::value               == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT, const RIGHT&>::value         == (EXPECT));     \
        D_OC_CHECK(TRAIT<LEFT, RIGHT&&>::value              == (EXPECT));     \
        D_OC_CHECK(TRAIT<const LEFT&, const RIGHT&>::value  == (EXPECT));     \
    } while (0)

// oct_level_count
//   constant: how many levels the refinement order carries.  The algebra tests
// loop over the whole enum rather than sampling it, so every law is checked on
// every pair (36) or triple (216) rather than on chosen witnesses.
static const int oct_level_count = 6;

// oct_level_at
//   function: the _i-th observational level in refinement order, coarsest
// first.  Out-of-range indices clamp to none so a loop bug cannot silently
// read past the enum.
inline
observational_level oct_level_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return observational_level::none;
        case 1:  return observational_level::size;
        case 2:  return observational_level::set;
        case 3:  return observational_level::bag;
        case 4:  return observational_level::seq;
        case 5:  return observational_level::str;
        default: return observational_level::none;
    }
}

// oct_name_is
//   function: compares a level's spelling against an expected literal.  The
// header returns a const char*, so identity of pointers is not the question -
// the characters are.
inline
bool oct_name_is(
    observational_level _level,
    const char*         _expected
)
{
    return ( std::strcmp(observational_level_name(_level), _expected) == 0 );
}

// oct_shared_is_meet
//   function: true iff a pair's shared level is the meet of the two types'
// own levels.  That is the trait's definition; stating it as a helper lets a
// spread of pairs be checked in one line each, which is what turns a
// definitional identity into a useful regression net.
template<typename _Left,
         typename _Right>
inline
bool oct_shared_is_meet()
{
    return ( shared_observational_level<_Left, _Right>::value
                 == observational_level_coarser(
                        observational_level_of<_Left>::value,
                        observational_level_of<_Right>::value ) );
}

// oct_stage_is_meet
//   function: the same identity for the stage of a comparison, over the
// lifetime lattice rather than the level order.
template<typename _Left,
         typename _Right>
inline
bool oct_stage_is_meet()
{
    return ( observational_stage_of<_Left, _Right>::value
                 == lifetime_meet(
                        observed_access_stage_of<_Left>::value,
                        observed_access_stage_of<_Right>::value ) );
}


// ===========================================================================
// II.  LEVEL-RESOLVING FIXTURES
// ===========================================================================
//   One fixture per rung of the cascade, each carrying the MINIMUM interface
// that reaches its rung, so a regression names the rung at fault.

// DOctBare
//   struct: no inspection surface at all.  The universal negative, and the
// only way to reach none.
struct DOctBare
{
    int unrelated;
};

// DOctSizeOnly
//   struct: size() and nothing else - the coarsest content step, equal-size.
struct DOctSizeOnly
{
    std::size_t size() const;
};

// DOctContainsOnly
//   struct: membership readable, so =set is decidable and nothing finer.
// Deliberately carries no size(), to show set is reached on its own merit
// rather than by falling through from a coarser rung.
struct DOctContainsOnly
{
    using value_type = int;

    bool contains(
        int _x
    ) const;
};

// DOctCountOnly
//   struct: per-class multiplicities readable, so =bag.
struct DOctCountOnly
{
    using value_type = int;

    std::size_t count(
        int _x
    ) const;
};

// DOctIterableOnly
//   struct: a const iteration and nothing else - the ordered frontier is
// readable, so =seq.  No size(), which is what distinguishes this from a
// container that merely reports a count.
struct DOctIterableOnly
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};

// DOctSubscriptOnly
//   struct: positional operator[] on a const lvalue, without iteration.  A
// flat positional read resolves seq.
struct DOctSubscriptOnly
{
    using value_type = int;

    const int& operator[](
        std::size_t _i
    ) const;
};

// DOctAtOnly
//   struct: positional at() without operator[] and without iteration.  The
// other half of the indexed-access disjunction.
struct DOctAtOnly
{
    using value_type = int;

    const int& at(
        std::size_t _i
    ) const;
};

// DOctHierarchicalIndexed
//   struct: positional access INTO A NESTED SHAPE - the only combination that
// reaches str.  The hierarchical tag supplies the shape; the subscript
// supplies the positional read.  Deliberately NOT iterable, so str is shown to
// need neither iteration nor size.
struct DOctHierarchicalIndexed
{
    using value_type          = int;
    using structure_category  = hierarchical;

    const int& operator[](
        std::size_t _i
    ) const;
};


// ===========================================================================
// III. CASCADE PRECEDENCE PROBES
// ===========================================================================
//   Types that satisfy several rungs at once.  The cascade takes the finest,
// so each of these pins one step of the order.

// DOctCountAndContains
//   struct: both the bag and the set interface.  bag is finer, so count wins.
struct DOctCountAndContains
{
    using value_type = int;

    std::size_t count(
        int _x
    ) const;
    bool contains(
        int _x
    ) const;
};

// DOctContainsAndSize
//   struct: both the set and the size interface.  set is finer.
struct DOctContainsAndSize
{
    using value_type = int;

    bool contains(
        int _x
    ) const;
    std::size_t size() const;
};

// DOctIterableAndCount
//   struct: both the seq and the bag interface.  seq is finer, so iteration
// wins - reading the sequence also reads the bag.
struct DOctIterableAndCount
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
    std::size_t count(
        int _x
    ) const;
};

// DOctHierarchicalIterable
//   struct: shape WITHOUT positional access.  str needs both, so this falls to
// seq - the nesting is present but unreachable by index.
struct DOctHierarchicalIterable
{
    using value_type         = int;
    using structure_category = hierarchical;

    const int* begin() const;
    const int* end() const;
};

// DOctHierarchicalCount
//   struct: shape with neither positional access nor iteration.  Falls past
// str and past seq to bag, which shows the shape signal contributes nothing on
// its own.
struct DOctHierarchicalCount
{
    using value_type         = int;
    using structure_category = hierarchical;

    std::size_t count(
        int _x
    ) const;
};

// DOctIndexedFlat
//   struct: positional access with an explicit FLAT tag, so the shape signal
// is denied outright rather than left to the depth heuristic.  Resolves seq.
struct DOctIndexedFlat
{
    using value_type         = int;
    using structure_category = flat;

    const int& operator[](
        std::size_t _i
    ) const;
    const int& at(
        std::size_t _i
    ) const;
};

// DOctFullInterface
//   struct: every operation at once, over a nested shape.  Resolves str, the
// finest rung, and serves as the positive carrier wherever a test needs a type
// that lights every probe.
struct DOctFullInterface
{
    using value_type         = int;
    using structure_category = hierarchical;

    std::size_t size() const;
    const int& operator[](
        std::size_t _i
    ) const;
    const int& at(
        std::size_t _i
    ) const;
    std::size_t count(
        int _x
    ) const;
    bool contains(
        int _x
    ) const;
    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// IV.  PROBE SHAPE FIXTURES
// ===========================================================================
//   The probes ask whether an operation is callable ON A CONST LVALUE, which
// is narrower than "declares a member of that name".  These fixtures pin what
// that admits and what it rejects.

// DOctNonconstSize
//   struct: size() is not const-qualified, so it is not an observation.
struct DOctNonconstSize
{
    std::size_t size();
};

// DOctPrivateSize
//   struct: size() is private.  Access control participates in the probe.
struct DOctPrivateSize
{
private:
    std::size_t size() const;
};

// DOctArgSize
//   struct: size() requires an argument, so the nullary call is ill-formed.
struct DOctArgSize
{
    std::size_t size(
        int _which
    ) const;
};

// DOctDefaultArgSize
//   struct: size() has a defaulted parameter, so the nullary call succeeds.
struct DOctDefaultArgSize
{
    std::size_t size(
        int _which = 0
    ) const;
};

// DOctVoidSize
//   struct: size() returns void.  The probe names the expression's type and
// never constrains it, so void counts as a detection - a reminder that these
// are structural probes, not semantic ones.
struct DOctVoidSize
{
    void size() const;
};

// DOctDataSize
//   struct: size is a data member, not callable.
struct DOctDataSize
{
    std::size_t size;
};

// DOctStaticSizeMember
//   struct: size() is a static member function.  Callable through an object
// expression, so it is a detection.
struct DOctStaticSizeMember
{
    static
    std::size_t size();
};

// DOctNonconstSubscript
//   struct: operator[] is NOT const-qualified - the shape a std::map has,
// because its subscript inserts.  Observation must leave the container
// invariant, so this is correctly not matched.
struct DOctNonconstSubscript
{
    using value_type = int;

    int& operator[](
        std::size_t _i
    );
};

// DOctNonconstAt
//   struct: at() is not const-qualified either.
struct DOctNonconstAt
{
    using value_type = int;

    int& at(
        std::size_t _i
    );
};

// DOctAtWrongArg
//   struct: at() takes something a size_t does not convert to, so the
// positional reading fails.
struct DOctAtWrongArg
{
    using value_type = int;

    const int& at(
        const char* _key
    ) const;
};

// DOctCountWrongArg
//   struct: count() does not accept the element type.
struct DOctCountWrongArg
{
    using value_type = int;

    std::size_t count(
        const char* _key
    ) const;
};

// DOctContainsWrongArg
//   struct: contains() does not accept the element type.
struct DOctContainsWrongArg
{
    using value_type = int;

    bool contains(
        const char* _key
    ) const;
};

// DOctCountNonconst
//   struct: count() is not const-qualified.
struct DOctCountNonconst
{
    using value_type = int;

    std::size_t count(
        int _x
    );
};

// DOctContainsNonconst
//   struct: contains() is not const-qualified.
struct DOctContainsNonconst
{
    using value_type = int;

    bool contains(
        int _x
    );
};

// oct_count_no_value_type
//   struct: a perfectly good count(int), but NO value_type - so
// element_type_of_t resolves void and the probe has nothing to pass.  This is
// the fixture that exercises the void-element path: the header writes
// declval<element_type_of_t<_Type>>() with no guard against a void element,
// and relies on that being a substitution failure rather than a hard error.
struct oct_count_no_value_type
{
    std::size_t count(
        int _x
    ) const;
    bool contains(
        int _x
    ) const;
};


// ===========================================================================
// V.   STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   Typedef'd so their argument lists' commas do not reach the macros, and so
// the level each resolves is named once here rather than restated per test.

using DOctVector      = std::vector<int>;          // seq, runtime
using DOctNested      = std::vector<std::vector<int> >;  // str, runtime
using DOctArray       = std::array<int, 3>;        // seq, compile-observable
using DOctSet         = std::set<int>;             // seq - the central gap
using DOctMultiset    = std::multiset<int>;        // seq
using DOctMapInt     = std::map<int, int>;        // seq; at(size_t) matches
using DOctMapString  = std::map<std::string, int>;      // seq, at does not
using DOctDeque       = std::deque<int>;           // seq
using DOctFlist       = std::forward_list<int>;    // seq, and NO size()
using DOctString      = std::string;               // seq
using DOctPair        = std::pair<int, int>;       // none, yet compile-stage
using DOctRawArray   = int[4];                    // seq, built-in subscript


// ===========================================================================
// VI.  STATIC-SIZE FIXTURES
// ===========================================================================
//   The stage of an access is proxied by a static size.  These two carry one
// without carrying anything else, so the stage axis can be moved independently
// of the level axis - the orthogonality tests_stage_level_orthogonality pins.
//   The std::tuple_size specialisations that give them their static size
// appear just below, outside the djinterp namespaces.

// DOctStaticSizedBare
//   struct: a static size and NO inspection surface - level none, stage both.
struct DOctStaticSizedBare
{
    int first;
    int second;
};

// DOctStaticSizedSize
//   struct: a static size and a size() member - level size, stage both.
struct DOctStaticSizedSize
{
    std::size_t size() const;
};


NS_END  // testing
NS_END  // djinterp


// std::tuple_size specialisations for the two fixtures above.  Specialising a
// standard trait for a program-defined type is permitted, and is how a type
// declares a compile-time size without being a std::array.
namespace std
{
    template<>
    struct tuple_size< ::djinterp::testing::DOctStaticSizedBare>
        : integral_constant<size_t, 2>
    {};

    template<>
    struct tuple_size< ::djinterp::testing::DOctStaticSizedSize>
        : integral_constant<size_t, 4>
    {};
}


NS_DJINTERP
NS_TESTING
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// VII. TEST DECLARATIONS
// ===========================================================================

// I.   Interface-operation probes    (..._tests_probes.cpp)
bool tests_has_size_member();
bool tests_has_subscript_index();
bool tests_has_at_index();
bool tests_has_indexed_access();
bool tests_has_count_member();
bool tests_has_contains_member();
bool tests_probe_method_shapes();
bool tests_probe_const_observation();
bool tests_probe_void_element_safety();
bool tests_probe_cv_ref();
bool tests_probe_companions();

// II.  The observational level       (..._tests_level_algebra.cpp)
bool tests_observational_level_enum();
bool tests_observational_level_name();
bool tests_observational_level_rank();
bool tests_observational_level_coarser();
bool tests_observational_level_finer();
bool tests_observational_level_subsumes();
bool tests_level_algebra_lattice_laws();
bool tests_level_algebra_out_of_range();

// III. The level one type resolves   (..._tests_level_of.cpp)
bool tests_observational_level_of_cascade();
bool tests_observational_level_of_precedence();
bool tests_observational_level_of_std_types();
bool tests_observational_level_of_set_gap();
bool tests_observational_level_of_cv_ref();
bool tests_observational_level_of_companions();

// IV.  The shared-interface principle (..._tests_shared_level.cpp)
bool tests_shared_observational_level();
bool tests_shared_level_is_meet();
bool tests_shared_level_principle();
bool tests_shared_level_cv_ref();
bool tests_shared_level_companions();

// V.   Stage of a comparison         (..._tests_stage.cpp)
bool tests_has_static_size_helper();
bool tests_observed_access_stage_of();
bool tests_observational_stage_of();
bool tests_is_compile_time_comparison();
bool tests_stage_range();
bool tests_stage_level_orthogonality();
bool tests_stage_cv_ref();
bool tests_stage_companions();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// observational_comparison_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
observational_comparison_traits_spec()
{
    return dt::module_spec{
        "observational_comparison_traits",
        "SFINAE structural traits for OBSERVATIONAL comparison - through "
        "which of a container's interface...",
        {
            dt::block_spec{
                "probes",
                "Interface-operation probes",
                {
                    { "tests_has_size_member",
                      "Verifies the probe for a const-callable size() - the "
                      "equal-size interface, and the coarsest content step "
                      "the header recognises",
                      &tests_has_size_member },
                    { "tests_has_subscript_index",
                      "Verifies the probe for a const-callable "
                      "operator[](size_t)",
                      &tests_has_subscript_index },
                    { "tests_has_at_index",
                      "Verifies the probe for a const-callable at(size_t)",
                      &tests_has_at_index },
                    { "tests_has_indexed_access",
                      "Verifies the disjunction of the two positional probes "
                      "- the interface that, over a nested shape, supplies "
                      "str, and over a flat one supplies a positional seq read",
                      &tests_has_indexed_access },
                    { "tests_has_count_member",
                      "Verifies the probe for a const-callable count(x) "
                      "taking an element - the per-class (bag) interface",
                      &tests_has_count_member },
                    { "tests_has_contains_member",
                      "Verifies the probe for a const-callable contains(x) - "
                      "the membership (set) interface",
                      &tests_has_contains_member },
                    { "tests_probe_method_shapes",
                      "Verifies WHAT the probes admit, using size() as the "
                      "representative, since the question they ask is \"does "
                      "this expression compile on a const lvalue\" rather "
                      "than \"is a member of this name declared\"",
                      &tests_probe_method_shapes },
                    { "tests_probe_const_observation",
                      "Verifies the rule that unifies section I: an operation "
                      "is an observation only if it is callable on a CONST "
                      "lvalue",
                      &tests_probe_const_observation },
                    { "tests_probe_void_element_safety",
                      "Verifies that the element-taking probes survive a type "
                      "with NO element type",
                      &tests_probe_void_element_safety },
                    { "tests_probe_cv_ref",
                      "Verifies that every section I probe honours the "
                      "clean_t contract",
                      &tests_probe_cv_ref },
                    { "tests_probe_companions",
                      "Verifies the `_v` companions of the section I probes",
                      &tests_probe_companions },
                }
            },
            dt::block_spec{
                "level_algebra",
                "The observational level",
                {
                    { "tests_observational_level_enum",
                      "Verifies the enumeration itself",
                      &tests_observational_level_enum },
                    { "tests_observational_level_name",
                      "Verifies the diagnostic spelling of each level",
                      &tests_observational_level_name },
                    { "tests_observational_level_rank",
                      "Verifies the refinement rank - the linear order the "
                      "level preorder collapses to",
                      &tests_observational_level_rank },
                    { "tests_observational_level_coarser",
                      "Verifies the meet of the refinement order - the level "
                      "a shared interface resolves",
                      &tests_observational_level_coarser },
                    { "tests_observational_level_finer",
                      "Verifies the join of the refinement order",
                      &tests_observational_level_finer },
                    { "tests_observational_level_subsumes",
                      "Verifies the order predicate - \"_a resolves at least "
                      "as finely as _b\"",
                      &tests_observational_level_subsumes },
                    { "tests_level_algebra_lattice_laws",
                      "Verifies that the meet and the join really do form a "
                      "lattice over the six levels, checked on every pair and "
                      "every triple rather than on witnesses",
                      &tests_level_algebra_lattice_laws },
                    { "tests_level_algebra_out_of_range",
                      "Verifies what the algebra does with a value that names "
                      "no enumerator, and records the two consequences that "
                      "follow",
                      &tests_level_algebra_out_of_range },
                }
            },
            dt::block_spec{
                "level_of",
                "The level one type resolves",
                {
                    { "tests_observational_level_of_cascade",
                      "Verifies that each rung is reachable, on a fixture "
                      "carrying the minimum interface for it",
                      &tests_observational_level_of_cascade },
                    { "tests_observational_level_of_precedence",
                      "Verifies the ORDER of the cascade, on fixtures that "
                      "satisfy two rungs at once",
                      &tests_observational_level_of_precedence },
                    { "tests_observational_level_of_std_types",
                      "Verifies the levels the standard containers resolve, "
                      "which is where the header's claims meet real interfaces",
                      &tests_observational_level_of_std_types },
                    { "tests_observational_level_of_set_gap",
                      "Verifies the gap this header exists to describe: the "
                      "distance between a container's NATIVE content level "
                      "and what its interface OBSERVATIONALLY resolves",
                      &tests_observational_level_of_set_gap },
                    { "tests_observational_level_of_cv_ref",
                      "Verifies that the trait honours the clean_t contract",
                      &tests_observational_level_of_cv_ref },
                    { "tests_observational_level_of_companions",
                      "Verifies the `_t` alias and the `_v` companion",
                      &tests_observational_level_of_companions },
                }
            },
            dt::block_spec{
                "shared_level",
                "The shared-interface principle",
                {
                    { "tests_shared_observational_level",
                      "Verifies the level at which named pairs can be decided",
                      &tests_shared_observational_level },
                    { "tests_shared_level_is_meet",
                      "Verifies that the trait really is the meet of the two "
                      "operands' levels, and inherits the meet's algebra",
                      &tests_shared_level_is_meet },
                    { "tests_shared_level_principle",
                      "Verifies the principle as the header states it: an "
                      "iterable container compared with an opaque one falls "
                      "from seq to whatever else is shared - contains gives "
                      "set, else size, else identity",
                      &tests_shared_level_principle },
                    { "tests_shared_level_cv_ref",
                      "Verifies that both operands are cleaned independently",
                      &tests_shared_level_cv_ref },
                    { "tests_shared_level_companions",
                      "Verifies the `_t` alias and the `_v` companion",
                      &tests_shared_level_companions },
                }
            },
            dt::block_spec{
                "stage",
                "Stage of a comparison",
                {
                    { "tests_has_static_size_helper",
                      "Verifies the internal helper that detects a "
                      "std::tuple_size<T>::value",
                      &tests_has_static_size_helper },
                    { "tests_observed_access_stage_of",
                      "Verifies the stage at which a single type's contents "
                      "are observable",
                      &tests_observed_access_stage_of },
                    { "tests_observational_stage_of",
                      "Verifies the stage a comparison of two types carries - "
                      "the meet of their access stages",
                      &tests_observational_stage_of },
                    { "tests_is_compile_time_comparison",
                      "Verifies the predicate that reads the compile-time bit "
                      "off the pair's stage",
                      &tests_is_compile_time_comparison },
                    { "tests_stage_range",
                      "Verifies which of the four lifetime values these "
                      "traits can actually produce",
                      &tests_stage_range },
                    { "tests_stage_level_orthogonality",
                      "Verifies that the stage axis and the level axis are "
                      "INDEPENDENT",
                      &tests_stage_level_orthogonality },
                    { "tests_stage_cv_ref",
                      "Verifies that the stage traits honour the clean_t "
                      "contract",
                      &tests_stage_cv_ref },
                    { "tests_stage_companions",
                      "Verifies the `_t` aliases and the `_v` companions of "
                      "section V",
                      &tests_stage_companions },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_OBSERVATIONAL_COMPARISON_TRAITS_
