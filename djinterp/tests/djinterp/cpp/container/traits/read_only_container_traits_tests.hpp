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
*   unrestricted handle reads read_write.  The fixtures DRoView and
*   DRoMutableView are that pair, sharing an `underlying` type so the point is
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
* path:      /tests/djinterp/cpp/container/traits/read_only_container_traits_tests.hpp
*                                        read_only_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.09
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
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/read_only_container_traits.hpp"
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

// ro_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = ro_check_failed(...)` on the failing path only.
inline
bool ro_check_failed(
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

// DRoObserver
//   struct: const observers and no mutator - read_only, structurally.
struct DRoObserver
{
    using value_type = int;

    const int& operator[](
        std::size_t _i
    ) const;
    const int* begin() const;
    std::size_t size() const;
};

// DRoBeginOnly
//   struct: const iteration alone, which is the whole read surface an
// associative view needs - read_only, structurally.
struct DRoBeginOnly
{
    using value_type = int;

    const int* begin() const;
};

// DRoConstIndex
//   struct: an operator[] yielding a CONST reference on both overloads.  The
// value is observable and no assignment through the subscript is well-formed,
// so this is the shape a read_only wrapper presents.
struct DRoConstIndex
{
    using value_type = int;

    const int& operator[](
        std::size_t _i
    );
    const int& operator[](
        std::size_t _i
    ) const;
};

// DRoTagged
//   struct: stamped read_only, with a matching structure.
struct DRoTagged
{
    using value_type = int;
    using capability = read_only;

    const int* begin() const;
};

// DRoTaggedOverride
//   struct: structurally read_write - it has both surfaces - but stamped
// read_only.  The tag is honoured first, so this is read_only.  This is the
// carrier a delegating predicate must agree with the baseline about.
struct DRoTaggedOverride
{
    using value_type = int;
    using capability = read_only;

    const int& operator[](
        std::size_t _i
    ) const;
    int& operator[](
        std::size_t _i
    );
    void push_back(
        int _value
    );
    void clear();
};

// DRoTaggedElsewhere
//   struct: stamped write_only while structurally read_only - the mirror of
// DRoTaggedOverride, and the case where deferring to the tag makes the
// predicate answer FALSE for a type that looks read_only.
struct DRoTaggedElsewhere
{
    using value_type = int;
    using capability = write_only;

    const int* begin() const;
};

// DRoSink
//   struct: a mutator plus value-free metadata and no value observer -
// write_only.
struct DRoSink
{
    using value_type = int;

    void push_back(
        int _value
    );
    std::size_t size() const;
    bool empty() const;
};

// DRoBoth
//   struct: both surfaces - read_write.
struct DRoBoth
{
    using value_type = int;

    const int* begin() const;
    void push_back(
        int _value
    );
};

// DRoInert
//   struct: neither surface - not access-classifiable.
struct DRoInert
{
    int unrelated;
};

// DRoMetadataOnly
//   struct: size() and empty() alone.  Metadata is not observation, so this is
// `none` and not read_only - the distinction a predicate keyed on "has no
// mutator" would miss.
struct DRoMetadataOnly
{
    using value_type = int;

    std::size_t size() const;
    bool empty() const;
};


// ===========================================================================
// III. OVERLAY FIXTURES
// ===========================================================================
//   The orthogonality pair.  Both wrap the SAME fully mutable container and
// differ only in what they forward, so any difference in capability is a
// property of the handle and not of the data.

// DRoView
//   struct: a restricted handle over a std::vector - it forwards the const
// surface only.
struct DRoView
{
    using value_type = int;
    using underlying = std::vector<int>;

    const int& operator[](
        std::size_t _i
    ) const;
    const int* begin() const;
    std::size_t size() const;

private:
    underlying m_data;
};

// DRoMutableView
//   struct: an unrestricted handle over the same std::vector - it forwards the
// mutating surface too.
struct DRoMutableView
{
    using value_type = int;
    using underlying = std::vector<int>;

    const int& operator[](
        std::size_t _i
    ) const;
    int& operator[](
        std::size_t _i
    );
    const int* begin() const;
    void push_back(
        int _value
    );
    std::size_t size() const;

private:
    underlying m_data;
};

// DRoViewOfImmutable
//   struct: the same restricted handle over an ARRAY of const elements, which
// cannot be modified through any handle at all.  It reads read_only exactly as
// DRoView does, so the capability says nothing about what lies behind it.
struct DRoViewOfImmutable
{
    using value_type = int;
    using underlying = std::array<const int, 3>;

    const int& operator[](
        std::size_t _i
    ) const;
    const int* begin() const;
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

using DRoVector = std::vector<int>;
using DRoArray  = std::array<int, 3>;
using DRoString = std::string;
using DRoSet    = std::set<int>;
using DRoMap    = std::map<int, int>;
using DRoPair   = std::pair<int, int>;
#endif  // !DTEST_SPEC_MODE  (fixtures)



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


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// read_only_container_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
read_only_container_traits_spec()
{
    return dt::module_spec{
        "read_only_container_traits",
        "is_read_only_container, true iff a handle to the type grants "
        "observation but NOT mutation",
        {
            dt::block_spec{
                "predicate",
                "A. The predicate",
                {
                    { "tests_is_read_only_container",
                      "Verifies that the predicate selects the read_only "
                      "capability and no other",
                      &tests_is_read_only_container },
                    { "tests_is_read_only_structural",
                      "Verifies the structural route - a value observer "
                      "present, every mutator absent",
                      &tests_is_read_only_structural },
                    { "tests_is_read_only_tagged",
                      "Verifies the opt-in route - a stamped `capability` "
                      "tag, which the baseline honours ahead of the structure",
                      &tests_is_read_only_tagged },
                    { "tests_is_read_only_std_types",
                      "Verifies the predicate across the standard library, "
                      "and records why it finds nothing there",
                      &tests_is_read_only_std_types },
                    { "tests_is_read_only_cv_ref",
                      "Verifies that the predicate honours the clean_t "
                      "contract",
                      &tests_is_read_only_cv_ref },
                    { "tests_is_read_only_companions",
                      "Verifies the `_v` companion the trait_detect engine "
                      "emits",
                      &tests_is_read_only_companions },
                }
            },
            dt::block_spec{
                "lattice",
                "B. Delegation, lattice and orthogonality",
                {
                    { "tests_read_only_delegates",
                      "Verifies that the predicate is exactly the baseline's "
                      "read_only verdict, and carries no second opinion",
                      &tests_read_only_delegates },
                    { "tests_read_only_lattice_exclusivity",
                      "Verifies that read_only and read_write name DIFFERENT "
                      "corners, and that no type satisfies both",
                      &tests_read_only_lattice_exclusivity },
                    { "tests_read_only_is_not_complement",
                      "Verifies that read_only is NOT the negation of "
                      "read_write - the mistake the lattice's shape invites",
                      &tests_read_only_is_not_complement },
                    { "tests_read_only_orthogonal_to_mutability",
                      "Verifies the header's ORTHOGONALITY claim: the "
                      "capability describes the HANDLE, not the data behind it",
                      &tests_read_only_orthogonal_to_mutability },
                    { "tests_read_only_tag_agreement",
                      "Verifies that the predicate and the baseline's tag "
                      "re-emission name the same corner - so a type routed by "
                      "access_tag_of and one tested by this predicate cannot "
                      "disagree",
                      &tests_read_only_tag_agreement },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_READ_ONLY_CONTAINER_TRAITS_
