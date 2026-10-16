/*******************************************************************************
* djinterp [testing]                       write_only_container_traits_tests.hpp
*
* Unit tests for write_only_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the write_only corner of
* the access axis.  Every test function is declared here and defined in the
* section .cpp that matches the claim of the header under test.
*
*   WHAT IS UNDER TEST is one predicate.  The header adds NO detection of its
* own: it names the write_only verdict of the baseline classifier in
* read_write_container_traits.hpp and nothing more.  So the suite is not about
* detection - the baseline suite covers that - but about the claims this header
* makes on top of it:
*
*     DELEGATION.  The predicate is exactly `capability == write_only` and
*   agrees with the baseline everywhere, including on the carriers where the
*   baseline's two signals disagree with one another.
*
*     THE METADATA EXCLUSION.  This is the claim that distinguishes this corner
*   from its read_only mirror, and the reason the corner is reachable at all.
*   size() and empty() reveal no ELEMENT VALUE and so are not observers; a sink
*   exposing only those plus an append surface therefore reads write_only
*   rather than read_write.  Were metadata counted, essentially every realistic
*   sink would classify read_write and this predicate would be true of almost
*   nothing.  wo_sink and wo_sink_bare pin it from either side.
*
*     ORTHOGONALITY.  An access capability is an OVERLAY on a handle, not a
*   property of the data behind it.  wo_sink_view and wo_full_view wrap the
*   same std::vector and differ only in what they forward.
*
*   A FOURTH CLAIM IS IMPLICIT and easy to get wrong downstream: write_only is
* one corner of a four-point lattice, so it is NOT the negation of read_only,
* nor of read_write.  An unclassifiable type is none of the three.
*
*   FIXTURE NAMING.  Fixtures are prefixed wo_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.
*
*
* path:      /tests/djinterp/core/container/traits/write_only_container_traits_tests.hpp
*                                       write_only_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Capability-corner fixtures
      --------------------------

III.  Metadata fixtures (the exclusion that makes the corner reachable)
      -----------------------------------------------------------------

IV.   Overlay fixtures (the orthogonality pair)
      -----------------------------------------

V.    Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

VI.   Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_WRITE_ONLY_CONTAINER_TRAITS_
#define DJINTERP_TESTS_WRITE_ONLY_CONTAINER_TRAITS_ 1

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
#include "../../../../../inc/djinterp/core/container/traits/write_only_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// wo_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = wo_check_failed(...)` on the failing path only.
inline bool
wo_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_WO_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds.
#define D_WO_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::wo_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_WO_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_WO_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_WO_CV_REF_AGREE
//   macro: the predicate writes clean_t before asking its question, so all
// seven cv- and reference-qualified spellings of a type must answer alike -
// including the volatile ones.  Not usable on void or a function type, since
// `void&` cannot be formed.
#define D_WO_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_WO_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_WO_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_WO_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_WO_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_WO_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_WO_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_WO_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)


// ===========================================================================
// II.  CAPABILITY-CORNER FIXTURES
// ===========================================================================
//   One carrier per capability, so the predicate can be shown to select
// exactly one of the four rather than merely to fire somewhere.  Each mutator
// shape gets its own carrier, since the baseline's write surface is a
// disjunction of five probes and this predicate inherits all of them.

// wo_sink
//   struct: the canonical append-only sink - a mutator plus value-free
// metadata, and no value observer.  The carrier the metadata exclusion exists
// to make representable.
struct wo_sink
{
    using value_type = int;

    void        push_back(int _value);
    std::size_t size() const;
    bool        empty() const;
};

// wo_sink_bare
//   struct: the same sink with the metadata removed.  It classifies
// identically, which is what shows the metadata was never contributing to the
// verdict.
struct wo_sink_bare
{
    using value_type = int;

    void push_back(int _value);
};

// wo_emplace_sink / wo_push_front_sink / wo_clear_sink
//   structs: sinks reached through each of the other mutator shapes, so the
// predicate is shown to inherit the whole write surface rather than one probe.
struct wo_emplace_sink
{
    using value_type = int;

    void emplace_back(int _value);
};

struct wo_push_front_sink
{
    using value_type = int;

    void push_front(int _value);
};

struct wo_clear_sink
{
    using value_type = int;

    void clear();
};

// wo_nonconst_observer
//   struct: its accessors exist but none is const-qualified, so no value is
// observable on a const instance - while its non-const operator[] yields an
// assignable lvalue.  The type therefore reads WRITE_ONLY, not readable, which
// is the least obvious way to reach this corner.
struct wo_nonconst_observer
{
    using value_type = int;

    int& operator[](std::size_t _i);
    int& front();
};

// wo_tagged
//   struct: stamped write_only, with a matching structure.
struct wo_tagged
{
    using value_type = int;
    using capability = write_only;

    void push_back(int _value);
};

// wo_tagged_override
//   struct: structurally read_write - it has both surfaces - but stamped
// write_only.  The tag is honoured first, so this is write_only.  The carrier
// a delegating predicate must agree with the baseline about.
struct wo_tagged_override
{
    using value_type = int;
    using capability = write_only;

    const int& operator[](std::size_t _i) const;
    int&       operator[](std::size_t _i);
    void       push_back(int _value);
    void       clear();
};

// wo_tagged_elsewhere
//   struct: stamped read_only while structurally write_only - the mirror of
// wo_tagged_override, and the case where deferring to the tag makes the
// predicate answer FALSE for a type that looks like a sink.
struct wo_tagged_elsewhere
{
    using value_type = int;
    using capability = read_only;

    void push_back(int _value);
};

// wo_observer
//   struct: const observers and no mutator - read_only.
struct wo_observer
{
    using value_type = int;

    const int&  operator[](std::size_t _i) const;
    const int*  begin() const;
    std::size_t size() const;
};

// wo_both
//   struct: both surfaces - read_write.
struct wo_both
{
    using value_type = int;

    const int* begin() const;
    void       push_back(int _value);
};

// wo_inert
//   struct: neither surface - not access-classifiable.
struct wo_inert
{
    int unrelated;
};


// ===========================================================================
// III. METADATA FIXTURES
// ===========================================================================
//   The exclusion this corner depends on.  Value-free metadata reveals no
// element value and so is not a read surface; these carriers isolate metadata
// from every other signal.

// wo_metadata_only
//   struct: size() and empty() and nothing else.  Not observable, and having
// no mutator either, so it is `none` rather than write_only - metadata alone
// is not enough in EITHER direction.
struct wo_metadata_only
{
    using value_type = int;

    std::size_t size() const;
    bool        empty() const;
};

// wo_sink_rich_metadata
//   struct: a sink carrying every value-free accessor a container might offer.
// None of them observes an element, so the verdict is unchanged - which is
// what shows the exclusion is about the KIND of accessor and not about how
// many there are.
struct wo_sink_rich_metadata
{
    using value_type = int;

    void        push_back(int _value);
    std::size_t size() const;
    std::size_t capacity() const;
    std::size_t max_size() const;
    bool        empty() const;
};

// wo_sink_with_observer
//   struct: the same sink with ONE value observer added.  That single accessor
// moves it off this corner to read_write, so the exclusion is load-bearing.
struct wo_sink_with_observer
{
    using value_type = int;

    void        push_back(int _value);
    std::size_t size() const;
    const int&  front() const;
};


// ===========================================================================
// IV.  OVERLAY FIXTURES
// ===========================================================================
//   The orthogonality pair.  Both wrap the SAME fully mutable container and
// differ only in what they forward, so any difference in capability is a
// property of the handle and not of the data.

// wo_sink_view
//   struct: a restricting handle over a std::vector - it forwards the append
// surface and the metadata, but no value observer.
struct wo_sink_view
{
    using value_type = int;
    using underlying = std::vector<int>;

    void        push_back(int _value);
    std::size_t size() const;
    bool        empty() const;

private:
    underlying m_data;
};

// wo_full_view
//   struct: an unrestricted handle over the same std::vector - it forwards the
// const surface too.
struct wo_full_view
{
    using value_type = int;
    using underlying = std::vector<int>;

    const int&  operator[](std::size_t _i) const;
    void        push_back(int _value);
    std::size_t size() const;

private:
    underlying m_data;
};


// ===========================================================================
// V.   STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   None of these is write_only: every standard container exposes a const
// observer alongside its mutators, so the whole standard set reads read_write.
// The write_only corner is reachable only through a restricting handle, which
// is what the fixtures above supply.

using wo_vector = std::vector<int>;
using wo_array  = std::array<int, 3>;
using wo_string = std::string;
using wo_set    = std::set<int>;
using wo_map    = std::map<int, int>;
using wo_pair   = std::pair<int, int>;


// ===========================================================================
// VI.  TEST DECLARATIONS
// ===========================================================================

// A. The predicate                (..._tests_predicate.cpp)
bool tests_is_write_only_container();
bool tests_is_write_only_structural();
bool tests_is_write_only_tagged();
bool tests_is_write_only_std_types();
bool tests_is_write_only_cv_ref();
bool tests_is_write_only_companions();

// B. Delegation, metadata, lattice and orthogonality
//                                 (..._tests_lattice.cpp)
bool tests_write_only_delegates();
bool tests_write_only_metadata_exclusion();
bool tests_write_only_lattice_exclusivity();
bool tests_write_only_is_not_complement();
bool tests_write_only_orthogonal_to_mutability();
bool tests_write_only_tag_agreement();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_WRITE_ONLY_CONTAINER_TRAITS_
