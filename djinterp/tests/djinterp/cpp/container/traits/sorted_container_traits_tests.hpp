/*******************************************************************************
* djinterp [testing]                           sorted_container_traits_tests.hpp
*
* Unit tests for sorted_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the sortedness-axis trait
* suite.  Every test function is declared here and defined in the section .cpp
* that matches the section of the header under test.
*
*   WHAT IS UNDER TEST.  The SORTEDNESS axis asks whether a container's TYPE
* promises its elements come out in comparator order.  It sits just above the
* order axis and is sourced from it: the classification first asks whether the
* container is unordered or ordered, and only then what it guarantees.
*
*   THE READING TO GET RIGHT is that "sorted" and "enumerates in sorted order"
* are DIFFERENT questions here.  sortedness::sorted means an ORDERED container
* whose positions are maintained in comparator order.  A std::set has no
* positions at all, so it is not sorted in that sense - it is monotone, and
* monotone is a state of is_unsorted_container.  Yet it does admit sorted
* enumeration.  A std::set is therefore simultaneously unsorted and sorted-
* enumerable, which reads as a contradiction until the two questions are held
* apart.  tests_predicates_monotone_paradox pins it.
*
*   ONE FIXTURE IS DELIBERATELY ABSENT.  A type with a NON-STATIC data member
* named sorted_invariant cannot be classified at all: it is detected by
* sorted_invariant_helper, whose detection is unevaluated, and then read by the
* specialisation's body, which is evaluated and needs an object.  That is a
* hard error outside the immediate context, so no such fixture can appear here
* without breaking the build.  tests_sorted_invariant_shapes documents the
* boundary and pins every shape either side of it.
*
*   FIXTURE NAMING.  Fixtures are prefixed srt_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.  Standard-library
* carriers with comma-bearing argument lists are typedef'd here for the same
* reason.
*
*
* path:      /tests/djinterp/cpp/container/traits/sorted_container_traits_tests.hpp
*                                           sorted_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Base and order-axis fixtures
      ----------------------------

III.  Comparator fixtures
      -------------------

IV.   Interval-bound fixtures
      -----------------------

V.    Sorted-invariant fixtures
      -------------------------

VI.   Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

VII.  Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_SORTED_CONTAINER_TRAITS_
#define DJINTERP_TESTS_SORTED_CONTAINER_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/sorted_container_traits.hpp"
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

// srt_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = srt_check_failed(...)` on the failing path only.
inline
bool srt_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_SR_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_SR_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::srt_check_failed(#__VA_ARGS__,          \
                                                       __FILE__,              \
                                                       __LINE__);             \
        }                                                                     \
    } while (0)

// D_SR_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_SR_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_SR_CV_REF_AGREE
//   macro: every trait in this header writes clean_t before asking its
// question, so all seven cv- and reference-qualified spellings of a type must
// answer alike - including the volatile ones.  Not usable on void or a
// function type, since `void&` cannot be formed.
#define D_SR_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_SR_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_SR_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_SR_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_SR_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_SR_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_SR_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_SR_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// srt_state_count
//   constant: how many states the sortedness axis carries.  The enum tests
// loop over the whole range rather than sampling it.
static const int srt_state_count = 5;

// srt_state_at
//   function: the _i-th sortedness state, in declaration order.  Out-of-range
// indices clamp to non_container so a loop bug cannot read past the enum.
inline
sortedness srt_state_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return sortedness::non_container;
        case 1:  return sortedness::unordered;
        case 2:  return sortedness::monotone;
        case 3:  return sortedness::order_dependent;
        case 4:  return sortedness::sorted;
        default: return sortedness::non_container;
    }
}

// srt_name_is
//   function: compares a state's spelling against an expected literal.  The
// header returns a const char*, so pointer identity is not the question - the
// characters are.
inline
bool srt_name_is(
    sortedness  _state,
    const char* _expected
)
{
    return ( std::strcmp(sortedness_name(_state), _expected) == 0 );
}


// ===========================================================================
// II.  BASE AND ORDER-AXIS FIXTURES
// ===========================================================================
//   The sortedness cascade dispatches on the order axis first, so each fixture
// below states its order-axis standing before adding a sortedness signal.

// DSrtBare
//   struct: not iterable - the non_container state.
struct DSrtBare
{
    int unrelated;
};

// DSrtKeyedNotIterable
//   struct: keyed but with no traversal, so it is neither ordered nor
// unordered and lands on non_container despite carrying a key.
struct DSrtKeyedNotIterable
{
    using value_type = int;
    using key_type   = int;
};

// DSrtSequence
//   struct: an ordered container with no sortedness signal of any kind - the
// order_dependent state, where sortedness is a property of the instance and
// not of the type.
struct DSrtSequence
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};

// DSrtKeyed
//   struct: an unordered container with no comparator - the unordered state,
// where sortedness does not apply.
struct DSrtKeyed
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// III. COMPARATOR FIXTURES
// ===========================================================================

// DSrtKeyedCompare
//   struct: an unordered container that DOES carry a comparator - the monotone
// state.  It has no positions, yet its enumeration is sorted by construction.
struct DSrtKeyedCompare
{
    using value_type  = int;
    using key_type    = int;
    using key_compare = std::less<int>;

    const int* begin() const;
    const int* end() const;
};

// DSrtSequenceCompare
//   struct: an ORDERED container carrying a comparator.  The comparator branch
// is reached only from the unordered arm, so this stays order_dependent - a
// comparator alone promises nothing about a sequence's positions.
struct DSrtSequenceCompare
{
    using value_type  = int;
    using key_compare = std::less<int>;

    const int* begin() const;
    const int* end() const;
};

// DSrtPrivateCompare
//   struct: key_compare is private, so the alias is inaccessible.  Access
// control participates in the probe, and the type reads as plain unordered.
struct DSrtPrivateCompare
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;

private:
    using key_compare = std::less<int>;
};


// ===========================================================================
// IV.  INTERVAL-BOUND FIXTURES
// ===========================================================================
//   has_interval_bounds_helper names two members through decltype and never
// evaluates either, so it detects anything the name can stand for - a static
// constant, a non-static data member, or a static member function.  Only a
// non-static member FUNCTION escapes it, decltype of one being ill-formed.
// The fixtures below cover each shape.

// DSrtBounds
//   struct: the intended shape - a closed-interval carrier declaring static
// constant bounds, whose arithmetic enumeration is monotone by construction.
struct DSrtBounds
{
    using value_type = int;

    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;

    const int* begin() const;
    const int* end() const;
};

// DSrtBoundsLowerOnly
//   struct: only one of the two bounds, so the conjunction fails and the type
// stays order_dependent.
struct DSrtBoundsLowerOnly
{
    using value_type = int;

    static constexpr int lower_bound = 0;

    const int* begin() const;
    const int* end() const;
};

// DSrtBoundsFunctions
//   struct: the bounds are static member FUNCTIONS.  decltype names them
// happily, so this is detected as an interval carrier though it declares no
// interval - a false positive the helper cannot distinguish.
struct DSrtBoundsFunctions
{
    using value_type = int;

    static
    int lower_bound();
    static
    int upper_bound();

    const int* begin() const;
    const int* end() const;
};

// DSrtBoundsNonstatic
//   struct: the bounds are ordinary instance fields.  decltype of a non-static
// data member is well-formed in an unevaluated context, so this too is
// detected as an interval carrier.
struct DSrtBoundsNonstatic
{
    using value_type = int;

    int lower_bound;
    int upper_bound;

    const int* begin() const;
    const int* end() const;
};

// DSrtBoundsMemberFunctions
//   struct: the bounds are non-static member functions - the shape a std::set
// has.  decltype of one is ill-formed, so this is the one bound-like shape the
// helper correctly rejects.
struct DSrtBoundsMemberFunctions
{
    using value_type = int;

    int lower_bound() const;
    int upper_bound() const;

    const int* begin() const;
    const int* end() const;
};

// DSrtKeyedBounds
//   struct: a KEYED container that also declares interval bounds.  The
// cascade tests the unordered arm first, so the bounds are never consulted and
// the type reads unordered - interval bounds are a statement about positions,
// and this type has none.
struct DSrtKeyedBounds
{
    using value_type = int;
    using key_type   = int;

    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;

    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// V.   SORTED-INVARIANT FIXTURES
// ===========================================================================
//   sorted_invariant_helper differs from the bounds helper in that it READS
// the member rather than merely naming it, so its answer is the member's VALUE
// and not its presence.  See the header comment above for the one shape that
// cannot be represented here at all.

// DSrtInvariantTrue
//   struct: the intended opt-in - an ordered sequence asserting it is
// maintained in sorted order.
struct DSrtInvariantTrue
{
    using value_type = int;

    static constexpr bool sorted_invariant = true;

    const int* begin() const;
    const int* end() const;
};

// DSrtInvariantFalse
//   struct: the opt-OUT.  The member is present, so the specialisation is
// selected, but its value is false - so the type stays order_dependent.  This
// is what makes the helper a value read rather than a presence test.
struct DSrtInvariantFalse
{
    using value_type = int;

    static constexpr bool sorted_invariant = false;

    const int* begin() const;
    const int* end() const;
};

// DSrtInvariantIntOne
//   struct: a non-bool constant, contextually converted.  Any non-zero value
// asserts the invariant.
struct DSrtInvariantIntOne
{
    using value_type = int;

    static constexpr int sorted_invariant = 1;

    const int* begin() const;
    const int* end() const;
};

// DSrtInvariantIntZero
//   struct: the same, zero-valued, so the invariant is denied.
struct DSrtInvariantIntZero
{
    using value_type = int;

    static constexpr int sorted_invariant = 0;

    const int* begin() const;
    const int* end() const;
};

// DSrtInvariantFunction
//   struct: sorted_invariant is a static member FUNCTION. It is no constant,
// so the invariant is ignored. (It used to be accepted, decaying to a
// never-null pointer, so a member given this name by accident claimed
// sortedness.)
struct DSrtInvariantFunction
{
    using value_type = int;

    static
    bool sorted_invariant();

    const int* begin() const;
    const int* end() const;
};

// DSrtInvariantNonStatic
//   struct: sorted_invariant is a NON-STATIC data member. Without an object
// it is no constant, so the invariant is ignored. (It used to select the
// specialisation and then be read without an object: a hard error.)
struct DSrtInvariantNonStatic
{
    using value_type = int;

    bool sorted_invariant;

    const int* begin() const;
    const int* end() const;
};

// DSrtInvariantAlias
//   struct: sorted_invariant is a type alias, not a value.  decltype of a type
// name is ill-formed, so the primary template is chosen and the invariant is
// correctly denied.
struct DSrtInvariantAlias
{
    using value_type       = int;
    using sorted_invariant = int;

    const int* begin() const;
    const int* end() const;
};

// DSrtBothSignals
//   struct: interval bounds AND an asserted invariant.  The two are joined by
// a disjunction, so either suffices and both together are no different.
struct DSrtBothSignals
{
    using value_type = int;

    static constexpr int  lower_bound      = 0;
    static constexpr int  upper_bound      = 9;
    static constexpr bool sorted_invariant = true;

    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// VI.  STANDARD-LIBRARY CARRIERS
// ===========================================================================

using DSrtVector    = std::vector<int>;              // order_dependent
using DSrtList      = std::list<int>;                // order_dependent
using DSrtArray     = std::array<int, 3>;            // order_dependent
using DSrtString    = std::string;                   // order_dependent
using DSrtSet       = std::set<int>;                 // monotone
using DSrtMultiset  = std::multiset<int>;            // monotone
using DSrtMap       = std::map<int, int>;            // monotone
using DSrtUset      = std::unordered_set<int>;       // unordered
using DSrtUmap      = std::unordered_map<int, int>;  // unordered
using DSrtPair      = std::pair<int, int>;           // non_container
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// VII. TEST DECLARATIONS
// ===========================================================================

// I.   Sortedness signals           (..._tests_signals.cpp)
bool tests_has_key_compare_helper();
bool tests_has_interval_bounds_helper();
bool tests_interval_bounds_shapes();
bool tests_sorted_invariant_helper();
bool tests_sorted_invariant_shapes();
bool tests_signals_cv_ref();

// II.  Sortedness classification    (..._tests_classification.cpp)
bool tests_sortedness_enum();
bool tests_sortedness_name();
bool tests_sortedness_of_cascade();
bool tests_sortedness_of_precedence();
bool tests_sortedness_of_std_types();
bool tests_sortedness_out_of_range();
bool tests_sortedness_cv_ref();
bool tests_sortedness_companions();

// III. Classification predicates    (..._tests_predicates.cpp)
bool tests_is_sorted_container();
bool tests_is_unsorted_container();
bool tests_is_unsorted_not_complement();
bool tests_is_monotone_container();
bool tests_admits_sorted_enumeration();
bool tests_predicates_monotone_paradox();
bool tests_predicates_partition();
bool tests_predicates_cv_ref();
bool tests_predicates_companions();

// V.   Aggregate snapshot           (..._tests_class.cpp)
bool tests_sorted_container_class_signals();
bool tests_sorted_container_class_kind();
bool tests_sorted_container_class_predicates();
bool tests_sorted_container_class_agreement();
bool tests_sorted_container_class_cv_ref();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// sorted_container_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
sorted_container_traits_spec()
{
    return dt::module_spec{
        "sorted_container_traits",
        "the SORTEDNESS axis, container-side traits: does a container's TYPE "
        "promise its elements are...",
        {
            dt::block_spec{
                "signals",
                "Sortedness signals",
                {
                    { "tests_has_key_compare_helper",
                      "Verifies the comparator tell - a `key_compare` alias - "
                      "which is what separates a tree-backed associative "
                      "container from a hashed one",
                      &tests_has_key_compare_helper },
                    { "tests_has_interval_bounds_helper",
                      "Verifies the interval tell - a lower_bound AND an "
                      "upper_bound member - on the shapes it is meant for",
                      &tests_has_interval_bounds_helper },
                    { "tests_interval_bounds_shapes",
                      "Verifies exactly WHICH declarations the interval probe "
                      "accepts, since the answer is broader than the name "
                      "suggests",
                      &tests_interval_bounds_shapes },
                    { "tests_sorted_invariant_helper",
                      "Verifies the explicit opt-in - a `sorted_invariant` "
                      "constant - which is the only signal in the header that "
                      "is a VALUE read rather than a presence test",
                      &tests_sorted_invariant_helper },
                    { "tests_sorted_invariant_shapes",
                      "Verifies which declarations the invariant probe "
                      "accepts, and documents the one that cannot be tested "
                      "at all",
                      &tests_sorted_invariant_shapes },
                    { "tests_signals_cv_ref",
                      "Verifies that all three signal probes honour the "
                      "clean_t contract",
                      &tests_signals_cv_ref },
                }
            },
            dt::block_spec{
                "classification",
                "Sortedness classification",
                {
                    { "tests_sortedness_enum",
                      "Verifies the enumeration itself",
                      &tests_sortedness_enum },
                    { "tests_sortedness_name",
                      "Verifies the diagnostic spelling of each state",
                      &tests_sortedness_name },
                    { "tests_sortedness_of_cascade",
                      "Verifies every branch of the classification, on a "
                      "carrier for each of the five states",
                      &tests_sortedness_of_cascade },
                    { "tests_sortedness_of_precedence",
                      "Verifies that the ORDER-axis test comes first, which "
                      "decides which signal is even consulted",
                      &tests_sortedness_of_precedence },
                    { "tests_sortedness_of_std_types",
                      "Verifies the state each standard container receives - "
                      "where the classification meets real interfaces",
                      &tests_sortedness_of_std_types },
                    { "tests_sortedness_out_of_range",
                      "Verifies what the spelling function does with a value "
                      "that names no enumerator",
                      &tests_sortedness_out_of_range },
                    { "tests_sortedness_cv_ref",
                      "Verifies that the classification honours the clean_t "
                      "contract",
                      &tests_sortedness_cv_ref },
                    { "tests_sortedness_companions",
                      "Verifies the `_t` alias and the `_v` companion",
                      &tests_sortedness_companions },
                }
            },
            dt::block_spec{
                "predicates",
                "Classification predicates",
                {
                    { "tests_is_sorted_container",
                      "Verifies the strictest predicate: the type maintains "
                      "its POSITIONS in comparator order",
                      &tests_is_sorted_container },
                    { "tests_is_unsorted_container",
                      "Verifies the predicate naming the three container "
                      "states that are not sorted",
                      &tests_is_unsorted_container },
                    { "tests_is_unsorted_not_complement",
                      "Verifies that is_unsorted_container is NOT the "
                      "negation of is_sorted_container, and that the "
                      "difference is exactly the non_container state",
                      &tests_is_unsorted_not_complement },
                    { "tests_is_monotone_container",
                      "Verifies the predicate naming the state a keyed "
                      "container reaches when it carries a comparator",
                      &tests_is_monotone_container },
                    { "tests_admits_sorted_enumeration",
                      "Verifies the union predicate, which is the one most "
                      "dispatch sites actually want: may this container be "
                      "walked front to back and relied upon to come out in "
                      "comparator order?",
                      &tests_admits_sorted_enumeration },
                    { "tests_predicates_monotone_paradox",
                      "Verifies the reading of this axis most likely to be "
                      "misused",
                      &tests_predicates_monotone_paradox },
                    { "tests_predicates_partition",
                      "Verifies the whole predicate table at once - for each "
                      "of the five states, the exact tuple of four answers it "
                      "produces",
                      &tests_predicates_partition },
                    { "tests_predicates_cv_ref",
                      "Verifies that all four predicates honour the clean_t "
                      "contract",
                      &tests_predicates_cv_ref },
                    { "tests_predicates_companions",
                      "Verifies the `_v` companions of all four predicates",
                      &tests_predicates_companions },
                }
            },
            dt::block_spec{
                "class",
                "Aggregate snapshot",
                {
                    { "tests_sorted_container_class_signals",
                      "Verifies the three raw-signal members, which report "
                      "what the type DECLARED rather than what the axis "
                      "concluded from it",
                      &tests_sorted_container_class_signals },
                    { "tests_sorted_container_class_kind",
                      "Verifies the classification members",
                      &tests_sorted_container_class_kind },
                    { "tests_sorted_container_class_predicates",
                      "Verifies the three derived-predicate members",
                      &tests_sorted_container_class_predicates },
                    { "tests_sorted_container_class_agreement",
                      "Verifies that every member of the aggregate equals the "
                      "standalone trait it reports, so the two spellings can "
                      "never drift apart",
                      &tests_sorted_container_class_agreement },
                    { "tests_sorted_container_class_cv_ref",
                      "Verifies that the aggregate honours the clean_t "
                      "contract",
                      &tests_sorted_container_class_cv_ref },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORTED_CONTAINER_TRAITS_
