/*******************************************************************************
* djinterp [testing]                        read_only_container_traits_tests.hpp
*
* Unit tests for read_only_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the read_only corner of
* the access axis.  Every test function is declared here and defined in the
* section .cpp that matches the claim of the header under test.
*
*   WHAT IS UNDER TEST is one predicate.  The header adds NO detection of its
* own: it names the read_only verdict of the baseline classifier in
* read_write_container_traits.hpp and nothing more.  So the suite is not about
* detection - the baseline suite covers that - but about the two claims this
* header makes on top of it:
*
*     DELEGATION.  The predicate is exactly `capability == read_only` and
*   agrees with the baseline everywhere, including on the carriers where the
*   baseline's two signals disagree with one another.  Anything it decided
*   independently would be a second source of truth.
*
*     ORTHOGONALITY.  An access capability is an OVERLAY on a handle, not a
*   property of the data behind it.  A read_only handle to a fully mutable
*   container reads read_only; the same container reached through an
*   unrestricted handle reads read_write.  The fixtures ro_view and
*   ro_mutable_view are that pair, sharing an `underlying` type so the point is
*   made without importing the mutability-grade header.
*
*   A THIRD CLAIM IS IMPLICIT and worth pinning because it is easy to get
* wrong downstream: read_only is one corner of a four-point lattice, so it is
* NOT the negation of read_write.  A write_only sink and an unclassifiable type
* are both non-read_write and both non-read_only.
* tests_read_only_is_not_complement pins it.
*
*   FIXTURE NAMING.  Fixtures are prefixed ro_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.
*
*
* path:      /tests/djinterp/core/container/traits/read_only_container_traits_tests.hpp
*                                        read_only_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Capability-corner fixtures
      --------------------------

III.  Overlay fixtures (the orthogonality pair)
      -----------------------------------------

IV.   Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

V.    Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_READ_ONLY_CONTAINER_TRAITS_
#define DJINTERP_TESTS_READ_ONLY_CONTAINER_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/container/traits/read_only_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// ro_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = ro_check_failed(...)` on the failing path only.
inline bool
ro_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_RO_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds.
#define D_RO_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::ro_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_RO_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_RO_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_RO_CV_REF_AGREE
//   macro: the predicate writes clean_t before asking its question, so all
// seven cv- and reference-qualified spellings of a type must answer alike -
// including the volatile ones.  Not usable on void or a function type, since
// `void&` cannot be formed.
#define D_RO_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_RO_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_RO_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_RO_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_RO_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_RO_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_RO_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_RO_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)


// ===========================================================================
// II.  CAPABILITY-CORNER FIXTURES
// ===========================================================================
//   One carrier per capability, so the predicate can be shown to select
// exactly one of the four rather than merely to fire somewhere.

// ro_observer
//   struct: const observers and no mutator - read_only, structurally.
struct ro_observer
{
    using value_type = int;

    const int&  operator[](std::size_t _i) const;
    const int*  begin() const;
    std::size_t size() const;
};

// ro_begin_only
//   struct: const iteration alone, which is the whole read surface an
// associative view needs - read_only, structurally.
struct ro_begin_only
{
    using value_type = int;

    const int* begin() const;
};

// ro_const_index
//   struct: an operator[] yielding a CONST reference on both overloads.  The
// value is observable and no assignment through the subscript is well-formed,
// so this is the shape a read_only wrapper presents.
struct ro_const_index
{
    using value_type = int;

    const int& operator[](std::size_t _i);
    const int& operator[](std::size_t _i) const;
};

// ro_tagged
//   struct: stamped read_only, with a matching structure.
struct ro_tagged
{
    using value_type = int;
    using capability = read_only;

    const int* begin() const;
};

// ro_tagged_override
//   struct: structurally read_write - it has both surfaces - but stamped
// read_only.  The tag is honoured first, so this is read_only.  This is the
// carrier a delegating predicate must agree with the baseline about.
struct ro_tagged_override
{
    using value_type = int;
    using capability = read_only;

    const int& operator[](std::size_t _i) const;
    int&       operator[](std::size_t _i);
    void       push_back(int _value);
    void       clear();
};

// ro_tagged_elsewhere
//   struct: stamped write_only while structurally read_only - the mirror of
// ro_tagged_override, and the case where deferring to the tag makes the
// predicate answer FALSE for a type that looks read_only.
struct ro_tagged_elsewhere
{
    using value_type = int;
    using capability = write_only;

    const int* begin() const;
};

// ro_sink
//   struct: a mutator plus value-free metadata and no value observer -
// write_only.
struct ro_sink
{
    using value_type = int;

    void        push_back(int _value);
    std::size_t size() const;
    bool        empty() const;
};

// ro_both
//   struct: both surfaces - read_write.
struct ro_both
{
    using value_type = int;

    const int* begin() const;
    void       push_back(int _value);
};

// ro_inert
//   struct: neither surface - not access-classifiable.
struct ro_inert
{
    int unrelated;
};

// ro_metadata_only
//   struct: size() and empty() alone.  Metadata is not observation, so this is
// `none` and not read_only - the distinction a predicate keyed on "has no
// mutator" would miss.
struct ro_metadata_only
{
    using value_type = int;

    std::size_t size() const;
    bool        empty() const;
};


// ===========================================================================
// III. OVERLAY FIXTURES
// ===========================================================================
//   The orthogonality pair.  Both wrap the SAME fully mutable container and
// differ only in what they forward, so any difference in capability is a
// property of the handle and not of the data.

// ro_view
//   struct: a restricted handle over a std::vector - it forwards the const
// surface only.
struct ro_view
{
    using value_type = int;
    using underlying = std::vector<int>;

    const int&  operator[](std::size_t _i) const;
    const int*  begin() const;
    std::size_t size() const;

private:
    underlying m_data;
};

// ro_mutable_view
//   struct: an unrestricted handle over the same std::vector - it forwards the
// mutating surface too.
struct ro_mutable_view
{
    using value_type = int;
    using underlying = std::vector<int>;

    const int&  operator[](std::size_t _i) const;
    int&        operator[](std::size_t _i);
    const int*  begin() const;
    void        push_back(int _value);
    std::size_t size() const;

private:
    underlying m_data;
};

// ro_view_of_immutable
//   struct: the same restricted handle over an ARRAY of const elements, which
// cannot be modified through any handle at all.  It reads read_only exactly as
// ro_view does, so the capability says nothing about what lies behind it.
struct ro_view_of_immutable
{
    using value_type = int;
    using underlying = std::array<const int, 3>;

    const int&  operator[](std::size_t _i) const;
    const int*  begin() const;
    std::size_t size() const;

private:
    underlying m_data;
};


// ===========================================================================
// IV.  STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   None of these is read_only: every standard container exposes both a const
// observer and some mutator, so the whole standard set reads read_write.  The
// read_only corner is reachable only through a restricting handle, which is
// what the fixtures above supply.

using ro_vector = std::vector<int>;
using ro_array  = std::array<int, 3>;
using ro_string = std::string;
using ro_set    = std::set<int>;
using ro_map    = std::map<int, int>;
using ro_pair   = std::pair<int, int>;


// ===========================================================================
// V.   TEST DECLARATIONS
// ===========================================================================

// A. The predicate                (..._tests_predicate.cpp)
bool tests_is_read_only_container();
bool tests_is_read_only_structural();
bool tests_is_read_only_tagged();
bool tests_is_read_only_std_types();
bool tests_is_read_only_cv_ref();
bool tests_is_read_only_companions();

// B. Delegation, lattice and orthogonality
//                                 (..._tests_lattice.cpp)
bool tests_read_only_delegates();
bool tests_read_only_lattice_exclusivity();
bool tests_read_only_is_not_complement();
bool tests_read_only_orthogonal_to_mutability();
bool tests_read_only_tag_agreement();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_READ_ONLY_CONTAINER_TRAITS_
