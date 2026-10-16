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
* path:      /tests/djinterp/core/container/traits/ordered_container_traits_tests.hpp
*                                          ordered_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
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
#include "../../../../../inc/djinterp/core/container/traits/ordered_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// ord_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = ord_check_failed(...)` on the failing path only.
inline bool
ord_check_failed(
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
inline ordering_kind
ord_kind_at(
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
inline sequential_layout
ord_layout_at(
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
inline bool
ord_kind_name_is(
    ordering_kind _kind,
    const char*   _expected
)
{
    return ( std::strcmp(ordering_kind_name(_kind), _expected) == 0 );
}

inline bool
ord_layout_name_is(
    sequential_layout _layout,
    const char*       _expected
)
{
    return ( std::strcmp(sequential_layout_name(_layout), _expected) == 0 );
}


// ===========================================================================
// II.  ORDER-AXIS FIXTURES
// ===========================================================================

// ord_bare
//   struct: neither iterable nor keyed - the universal negative.
struct ord_bare
{
    int unrelated;
};

// ord_begin_only
//   struct: begin() without end(), so it is not iterable and lands on the
// non_container state despite looking container-shaped.
struct ord_begin_only
{
    using value_type = int;

    const int* begin() const;
};

// ord_sequence
//   struct: iterable, no key_type - the minimal positional sequence.
struct ord_sequence
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};

// ord_keyed
//   struct: iterable AND keyed - permutation-invariant, so unordered.
struct ord_keyed
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
};

// ord_keyed_void
//   struct: key_type is an alias to void.  The helper detects the ALIAS, not
// any property of what it names, so this is keyed all the same.
struct ord_keyed_void
{
    using value_type = int;
    using key_type   = void;

    const int* begin() const;
    const int* end() const;
};

// ord_keyed_not_iterable
//   struct: keyed but with no traversal, so it is neither ordered NOR
// unordered - both predicates gate on iterability first.
struct ord_keyed_not_iterable
{
    using value_type = int;
    using key_type   = int;
};

// ord_private_key
//   struct: key_type is private, so the alias is inaccessible and the type
// reads as an ordinary sequence.  Access control participates in the probe.
struct ord_private_key
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

// ord_input_iterator
//   struct: a minimal input iterator, for the fixture below.
struct ord_input_iterator
{
    using iterator_category = std::input_iterator_tag;
    using value_type        = int;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const int*;
    using reference         = const int&;

    int  operator*() const;
    ord_input_iterator& operator++();
    bool operator!=(const ord_input_iterator& _other) const;
};

// ord_input_only
//   struct: an ordered container whose traversal is weaker than forward.  It
// reaches the LAST branch of the layout cascade, which is the only route to a
// layout of none for a type that is genuinely ordered.
struct ord_input_only
{
    using value_type = int;

    ord_input_iterator begin() const;
    ord_input_iterator end() const;
};

// ord_text_forward
//   struct: a text buffer whose traversal is only forward.  c_str() is tested
// before any category, so this is string_like regardless.
struct ord_text_forward
{
    using value_type = char;

    const char* c_str() const;
    const char* begin() const;
    const char* end() const;
};

// ord_text_contiguous
//   struct: a text buffer that is ALSO contiguous.  Both branches could fire;
// c_str() comes first, so string_like wins over array_like.
struct ord_text_contiguous
{
    using value_type = char;

    const char* c_str() const;
    const char* data() const;
    const char* begin() const;
    const char* end() const;
};

// ord_text_keyed
//   struct: a text buffer that is keyed, so not ordered.  The layout gate
// rejects it before c_str() is ever consulted.
struct ord_text_keyed
{
    using value_type = char;
    using key_type   = int;

    const char* c_str() const;
    const char* begin() const;
    const char* end() const;
};

// ord_text_void
//   struct: c_str() returns void.  The helper names the expression's type and
// never constrains it, so void counts as a detection - a reminder that these
// are structural probes, not semantic ones.
struct ord_text_void
{
    using value_type = char;

    void        c_str() const;
    const char* begin() const;
    const char* end() const;
};

// ord_text_nonconst
//   struct: c_str() is not const-qualified, so the const-lvalue probe misses
// it and the type falls through to its iterator category.
struct ord_text_nonconst
{
    using value_type = char;

    const char* c_str();
    const char* data() const;
    const char* begin() const;
    const char* end() const;
};

// ord_text_private
//   struct: c_str() is private.  Access control participates here too.
struct ord_text_private
{
    using value_type = char;

    const char* data() const;
    const char* begin() const;
    const char* end() const;

private:
    const char* c_str() const;
};

// ord_text_arg
//   struct: c_str() requires an argument, so the nullary call is ill-formed.
struct ord_text_arg
{
    using value_type = char;

    const char* c_str(int _which) const;
    const char* begin() const;
    const char* end() const;
};

// ord_random_no_data
//   struct: random-access traversal with NO data() accessor.  Contiguity is
// inferred from data(), so a type that is contiguous in fact but does not say
// so reads as deque_like - the same reading std::vector<bool> gets.
struct ord_random_no_data
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

using ord_vector      = std::vector<int>;          // ordered, array_like
using ord_bool_vector = std::vector<bool>;         // ordered, deque_like (!)
using ord_nested      = std::vector<std::vector<int> >;  // ordered, array_like
using ord_array       = std::array<int, 3>;        // ordered, array_like
using ord_deque       = std::deque<int>;           // ordered, deque_like
using ord_list        = std::list<int>;            // ordered, list_like
using ord_flist       = std::forward_list<int>;    // ordered, forward_list_like
using ord_string      = std::string;               // ordered, string_like
using ord_set         = std::set<int>;             // unordered - the gap
using ord_multiset    = std::multiset<int>;        // unordered
using ord_map         = std::map<int, int>;        // unordered
using ord_uset        = std::unordered_set<int>;   // unordered
using ord_umap        = std::unordered_map<int, int>;    // unordered
using ord_pair        = std::pair<int, int>;       // non_container


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_ORDERED_CONTAINER_TRAITS_
