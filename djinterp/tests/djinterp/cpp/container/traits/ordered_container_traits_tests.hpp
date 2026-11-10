/*******************************************************************************
* djinterp [testing]                          ordered_container_traits_tests.hpp
*
* Unit tests for ordered_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the order-axis trait
* suite.  Every test function is declared here and defined in the section .cpp
* that matches the section of the header under test.
*
*   WHAT IS UNDER TEST.  The ORDER axis asks whether a container's iteration
* order is a meaningful, stable property of the container's identity - whether
* position CARRIES meaning - and not whether the elements happen to come out in
* some sequence.  The header answers with two complementary predicates, a
* three-way classification, and, for code that must dispatch on physical shape,
* a storage-layout classification.
*
*   THE READING TO GET RIGHT, and the one the fixtures are built around, is
* that "unordered" here means PERMUTATION-INVARIANT, not "not sorted".  A
* std::map is is_unordered_container, because its identity is its key->value
* mapping and no rearrangement of it is a different map - even though
* enumerating it yields a perfectly stable, sorted sequence.  The everyday
* sense of "ordered" is the SORTEDNESS axis, which lives in the sibling header
* and classifies that same std::map as monotone.  tests_order_axis_keyed_gap
* pins the distinction.
*
*   FIXTURE NAMING.  Fixtures are prefixed ord_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.  Standard-library
* carriers with comma-bearing argument lists are typedef'd here for the same
* reason.
*
*
* path:      /tests/djinterp/cpp/container/traits/ordered_container_traits_tests.hpp
*                                          ordered_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Order-axis fixtures
      -------------------

III.  Layout fixtures
      ---------------

IV.   Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

V.    Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_ORDERED_CONTAINER_TRAITS_
#define DJINTERP_TESTS_ORDERED_CONTAINER_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <forward_list>
#include <iterator>
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
#include "../../../../../inc/djinterp/cpp/container/traits/ordered_container_traits.hpp"
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

// ord_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = ord_check_failed(...)` on the failing path only.
inline
bool ord_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_OR_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_OR_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::ord_check_failed(#__VA_ARGS__,          \
                                                       __FILE__,              \
                                                       __LINE__);             \
        }                                                                     \
    } while (0)

// D_OR_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_OR_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_OR_CV_REF_AGREE
//   macro: every trait in this header writes clean_t before asking its
// question, so all seven cv- and reference-qualified spellings of a type must
// answer alike - including the volatile ones, which a trait phrased on the
// unstripped type would lose.  Not usable on void or a function type, since
// `void&` cannot be formed.
#define D_OR_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_OR_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_OR_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_OR_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_OR_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_OR_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_OR_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_OR_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// ord_kind_count / ord_layout_count
//   constants: how many states each classification carries.  The enum tests
// loop over the whole range rather than sampling it.
static const int ord_kind_count   = 3;
static const int ord_layout_count = 6;

// ord_kind_at
//   function: the _i-th ordering_kind, in declaration order.  Out-of-range
// indices clamp to non_container so a loop bug cannot read past the enum.
inline
ordering_kind ord_kind_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return ordering_kind::non_container;
        case 1:  return ordering_kind::unordered;
        case 2:  return ordering_kind::ordered;
        default: return ordering_kind::non_container;
    }
}

// ord_layout_at
//   function: the _i-th sequential_layout, in declaration order.
inline
sequential_layout ord_layout_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return sequential_layout::none;
        case 1:  return sequential_layout::array_like;
        case 2:  return sequential_layout::deque_like;
        case 3:  return sequential_layout::list_like;
        case 4:  return sequential_layout::forward_list_like;
        case 5:  return sequential_layout::string_like;
        default: return sequential_layout::none;
    }
}

// ord_kind_name_is / ord_layout_name_is
//   functions: compare a state's spelling against an expected literal.  The
// header returns a const char*, so pointer identity is not the question - the
// characters are.
inline
bool ord_kind_name_is(
    ordering_kind _kind,
    const char*   _expected
)
{
    return ( std::strcmp(ordering_kind_name(_kind), _expected) == 0 );
}

inline
bool ord_layout_name_is(
    sequential_layout _layout,
    const char*       _expected
)
{
    return ( std::strcmp(sequential_layout_name(_layout), _expected) == 0 );
}


// ===========================================================================
// II.  ORDER-AXIS FIXTURES
// ===========================================================================

// DOrdBare
//   struct: neither iterable nor keyed - the universal negative.
struct DOrdBare
{
    int unrelated;
};

// DOrdBeginOnly
//   struct: begin() without end(), so it is not iterable and lands on the
// non_container state despite looking container-shaped.
struct DOrdBeginOnly
{
    using value_type = int;

    const int* begin() const;
};

// DOrdSequence
//   struct: iterable, no key_type - the minimal positional sequence.
struct DOrdSequence
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};

// DOrdKeyed
//   struct: iterable AND keyed - permutation-invariant, so unordered.
struct DOrdKeyed
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
};

// DOrdKeyedVoid
//   struct: key_type is an alias to void.  The helper detects the ALIAS, not
// any property of what it names, so this is keyed all the same.
struct DOrdKeyedVoid
{
    using value_type = int;
    using key_type   = void;

    const int* begin() const;
    const int* end() const;
};

// DOrdKeyedNotIterable
//   struct: keyed but with no traversal, so it is neither ordered NOR
// unordered - both predicates gate on iterability first.
struct DOrdKeyedNotIterable
{
    using value_type = int;
    using key_type   = int;
};

// DOrdPrivateKey
//   struct: key_type is private, so the alias is inaccessible and the type
// reads as an ordinary sequence.  Access control participates in the probe.
struct DOrdPrivateKey
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;

private:
    using key_type = int;
};


// ===========================================================================
// III. LAYOUT FIXTURES
// ===========================================================================

// DOrdInputIterator
//   struct: a minimal input iterator, for the fixture below.
struct DOrdInputIterator
{
    using iterator_category = std::input_iterator_tag;
    using value_type        = int;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const int*;
    using reference         = const int&;

    int operator*() const;
    DOrdInputIterator& operator++();
    bool operator!=(
        const DOrdInputIterator& _other
    ) const;
};

// DOrdInputOnly
//   struct: an ordered container whose traversal is weaker than forward.  It
// reaches the LAST branch of the layout cascade, which is the only route to a
// layout of none for a type that is genuinely ordered.
struct DOrdInputOnly
{
    using value_type = int;

    DOrdInputIterator begin() const;
    DOrdInputIterator end() const;
};

// DOrdTextForward
//   struct: a text buffer whose traversal is only forward.  c_str() is tested
// before any category, so this is string_like regardless.
struct DOrdTextForward
{
    using value_type = char;

    const char* c_str() const;
    const char* begin() const;
    const char* end() const;
};

// DOrdTextContiguous
//   struct: a text buffer that is ALSO contiguous.  Both branches could fire;
// c_str() comes first, so string_like wins over array_like.
struct DOrdTextContiguous
{
    using value_type = char;

    const char* c_str() const;
    const char* data() const;
    const char* begin() const;
    const char* end() const;
};

// DOrdTextKeyed
//   struct: a text buffer that is keyed, so not ordered.  The layout gate
// rejects it before c_str() is ever consulted.
struct DOrdTextKeyed
{
    using value_type = char;
    using key_type   = int;

    const char* c_str() const;
    const char* begin() const;
    const char* end() const;
};

// DOrdTextVoid
//   struct: c_str() returns void.  The helper names the expression's type and
// never constrains it, so void counts as a detection - a reminder that these
// are structural probes, not semantic ones.
struct DOrdTextVoid
{
    using value_type = char;

    void c_str() const;
    const char* begin() const;
    const char* end() const;
};

// DOrdTextNonconst
//   struct: c_str() is not const-qualified, so the const-lvalue probe misses
// it and the type falls through to its iterator category.
struct DOrdTextNonconst
{
    using value_type = char;

    const char* c_str();
    const char* data() const;
    const char* begin() const;
    const char* end() const;
};

// DOrdTextPrivate
//   struct: c_str() is private.  Access control participates here too.
struct DOrdTextPrivate
{
    using value_type = char;

    const char* data() const;
    const char* begin() const;
    const char* end() const;

private:
    const char* c_str() const;
};

// DOrdTextArg
//   struct: c_str() requires an argument, so the nullary call is ill-formed.
struct DOrdTextArg
{
    using value_type = char;

    const char* c_str(
        int _which
    ) const;
    const char* begin() const;
    const char* end() const;
};

// DOrdRandomNoData
//   struct: random-access traversal with NO data() accessor.  Contiguity is
// inferred from data(), so a type that is contiguous in fact but does not say
// so reads as deque_like - the same reading std::vector<bool> gets.
struct DOrdRandomNoData
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// IV.  STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   Typedef'd so their argument lists' commas do not reach the macros, and so
// the classification each carries is named once here rather than per test.

using DOrdVector      = std::vector<int>;          // ordered, array_like
using DOrdBoolVector = std::vector<bool>;         // ordered, deque_like (!)
using DOrdNested      = std::vector<std::vector<int> >;  // ordered, array_like
using DOrdArray       = std::array<int, 3>;        // ordered, array_like
using DOrdDeque       = std::deque<int>;           // ordered, deque_like
using DOrdList        = std::list<int>;            // ordered, list_like
using DOrdFlist       = std::forward_list<int>;    // ordered, forward_list_like
using DOrdString      = std::string;               // ordered, string_like
using DOrdSet         = std::set<int>;             // unordered - the gap
using DOrdMultiset    = std::multiset<int>;        // unordered
using DOrdMap         = std::map<int, int>;        // unordered
using DOrdUset        = std::unordered_set<int>;   // unordered
using DOrdUmap        = std::unordered_map<int, int>;    // unordered
using DOrdPair        = std::pair<int, int>;       // non_container
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// V.   TEST DECLARATIONS
// ===========================================================================

// I.   Order-axis predicates        (..._tests_order_axis.cpp)
bool tests_has_key_type_helper();
bool tests_is_ordered_container();
bool tests_is_unordered_container();
bool tests_order_axis_exclusivity();
bool tests_order_axis_keyed_gap();
bool tests_order_axis_cv_ref();
bool tests_order_axis_companions();

// II.  The ordering kind            (..._tests_ordering_kind.cpp)
bool tests_ordering_kind_enum();
bool tests_ordering_kind_name();
bool tests_ordering_kind_of();
bool tests_ordering_kind_of_agreement();
bool tests_ordering_kind_out_of_range();
bool tests_ordering_kind_cv_ref();
bool tests_ordering_kind_companions();

// III. The sequential layout        (..._tests_layout.cpp)
bool tests_has_c_str_helper();
bool tests_sequential_layout_enum();
bool tests_sequential_layout_name();
bool tests_sequential_layout_of_std_types();
bool tests_sequential_layout_of_precedence();
bool tests_sequential_layout_of_gate();
bool tests_sequential_layout_of_fallthrough();
bool tests_sequential_layout_out_of_range();
bool tests_sequential_layout_cv_ref();
bool tests_sequential_layout_companions();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// ordered_container_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
ordered_container_traits_spec()
{
    return dt::module_spec{
        "ordered_container_traits",
        "the ORDER axis, container-side traits: is a container's iteration "
        "order a meaningful, stable...",
        {
            dt::block_spec{
                "order_axis",
                "Order-axis predicates",
                {
                    { "tests_has_key_type_helper",
                      "Verifies the associative tell - a `key_type` alias - "
                      "which is the single signal the whole order axis turns "
                      "on",
                      &tests_has_key_type_helper },
                    { "tests_is_ordered_container",
                      "Verifies the positional-identity predicate: iterable "
                      "AND unkeyed",
                      &tests_is_ordered_container },
                    { "tests_is_unordered_container",
                      "Verifies the permutation-invariance predicate: "
                      "iterable AND keyed",
                      &tests_is_unordered_container },
                    { "tests_order_axis_exclusivity",
                      "Verifies how the two predicates relate",
                      &tests_order_axis_exclusivity },
                    { "tests_order_axis_keyed_gap",
                      "Verifies the reading this axis is easiest to get wrong",
                      &tests_order_axis_keyed_gap },
                    { "tests_order_axis_cv_ref",
                      "Verifies that both predicates honour the clean_t "
                      "contract",
                      &tests_order_axis_cv_ref },
                    { "tests_order_axis_companions",
                      "Verifies the `_v` companions of the two predicates",
                      &tests_order_axis_companions },
                }
            },
            dt::block_spec{
                "ordering_kind",
                "The ordering kind",
                {
                    { "tests_ordering_kind_enum",
                      "Verifies the enumeration itself",
                      &tests_ordering_kind_enum },
                    { "tests_ordering_kind_name",
                      "Verifies the diagnostic spelling of each state",
                      &tests_ordering_kind_name },
                    { "tests_ordering_kind_of",
                      "Verifies the classification itself, on a carrier for "
                      "each state",
                      &tests_ordering_kind_of },
                    { "tests_ordering_kind_of_agreement",
                      "Verifies that the classification and the two "
                      "predicates of section I are one statement in two "
                      "forms, so neither can drift from the other",
                      &tests_ordering_kind_of_agreement },
                    { "tests_ordering_kind_out_of_range",
                      "Verifies what the spelling function does with a value "
                      "that names no enumerator",
                      &tests_ordering_kind_out_of_range },
                    { "tests_ordering_kind_cv_ref",
                      "Verifies that the classification honours the clean_t "
                      "contract",
                      &tests_ordering_kind_cv_ref },
                    { "tests_ordering_kind_companions",
                      "Verifies the `_t` alias and the `_v` companion",
                      &tests_ordering_kind_companions },
                }
            },
            dt::block_spec{
                "layout",
                "The sequential layout",
                {
                    { "tests_has_c_str_helper",
                      "Verifies the text-buffer tell - a const-callable "
                      "c_str() - which is the only signal in this header that "
                      "is a member EXPRESSION rather than a nested alias",
                      &tests_has_c_str_helper },
                    { "tests_sequential_layout_enum",
                      "Verifies the enumeration itself",
                      &tests_sequential_layout_enum },
                    { "tests_sequential_layout_name",
                      "Verifies the diagnostic spelling of each layout",
                      &tests_sequential_layout_name },
                    { "tests_sequential_layout_of_std_types",
                      "Verifies the layout each standard container receives - "
                      "where the classification meets real interfaces",
                      &tests_sequential_layout_of_std_types },
                    { "tests_sequential_layout_of_precedence",
                      "Verifies the ORDER of the cascade",
                      &tests_sequential_layout_of_precedence },
                    { "tests_sequential_layout_of_gate",
                      "Verifies that only an ORDERED container has a layout",
                      &tests_sequential_layout_of_gate },
                    { "tests_sequential_layout_of_fallthrough",
                      "Verifies the LAST branch of the cascade, which is "
                      "otherwise unreachable by any standard container",
                      &tests_sequential_layout_of_fallthrough },
                    { "tests_sequential_layout_out_of_range",
                      "Verifies what the spelling function does with a value "
                      "that names no enumerator",
                      &tests_sequential_layout_out_of_range },
                    { "tests_sequential_layout_cv_ref",
                      "Verifies that the classification honours the clean_t "
                      "contract",
                      &tests_sequential_layout_cv_ref },
                    { "tests_sequential_layout_companions",
                      "Verifies the `_t` alias and the `_v` companion",
                      &tests_sequential_layout_companions },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_ORDERED_CONTAINER_TRAITS_
