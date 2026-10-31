/*******************************************************************************
* djinterp [testing]                       read_write_container_traits_tests.hpp
*
* Unit tests for read_write_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the access-axis baseline
* suite.  Every test function is declared here and defined in the section .cpp
* that matches the section of the header under test.
*
*   WHAT IS UNDER TEST.  The ACCESS axis asks what a handle to a type is
* PERMITTED to do - observe, mutate, both, or neither - and reports it as one
* of four capabilities.  Two signals decide, in priority order: an explicit
* `capability` tag carrying can_read and can_write, else the structural surface
* found by SFINAE.
*
*   THE DESIGN POINT THE FIXTURES ARE BUILT AROUND is that value-free metadata
* is deliberately NOT a read surface.  size() and empty() reveal no element
* value, so a type exposing only those plus an append surface reads write_only
* rather than read_write - which is exactly what makes an append-only sink
* representable.  DRwMetadataOnly and DRwSink pin it from both directions.
*
*   ONE FIXTURE IS DELIBERATELY ABSENT.  A capability tag whose can_read /
* can_write are NON-STATIC data members cannot be classified at all: the tag is
* detected by has_capability_bits_helper, whose probe is unevaluated, and then
* read by access_bits_impl's tagged specialisation, which is evaluated and
* needs an object.  That is a hard error outside the immediate context, so no
* such fixture can appear here without breaking the build.
* tests_capability_tag_shapes documents the boundary and pins every shape
* either side of it.
*
*   FIXTURE NAMING.  Fixtures are prefixed rw_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.  Standard-library
* carriers with comma-bearing argument lists are typedef'd here for the same
* reason.
*
*
* path:      /tests/djinterp/cpp/container/traits/read_write_container_traits_tests.hpp
*                                       read_write_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Read-surface fixtures
      ---------------------

III.  Write-surface fixtures
      ----------------------

IV.   Capability-tag fixtures
      -----------------------

V.    Capability-corner fixtures
      --------------------------

VI.   Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

VII.  Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_READ_WRITE_CONTAINER_TRAITS_
#define DJINTERP_TESTS_READ_WRITE_CONTAINER_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/container/traits/read_write_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// rw_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = rw_check_failed(...)` on the failing path only.
inline
bool rw_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_RW_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_RW_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::rw_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_RW_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_RW_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_RW_CV_REF_AGREE
//   macro: every trait in this header writes clean_t before asking its
// question, so all seven cv- and reference-qualified spellings of a type must
// answer alike - including the volatile ones.  Not usable on void or a
// function type, since `void&` cannot be formed.
#define D_RW_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_RW_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_RW_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_RW_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_RW_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_RW_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_RW_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_RW_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// rw_capability_count
//   constant: how many capabilities the axis carries.  The enum tests loop
// over the whole range rather than sampling it.
static const int rw_capability_count = 4;

// rw_capability_at
//   function: the _i-th access_capability, in declaration order.  Out-of-range
// indices clamp to none so a loop bug cannot read past the enum.
inline
access_capability rw_capability_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return access_capability::none;
        case 1:  return access_capability::read_only;
        case 2:  return access_capability::write_only;
        case 3:  return access_capability::read_write;
        default: return access_capability::none;
    }
}

// rw_name_is
//   function: compares a capability's spelling against an expected literal.
// The header returns a const char*, so pointer identity is not the question -
// the characters are.
inline
bool rw_name_is(
    access_capability _capability,
    const char*       _expected
)
{
    return ( std::strcmp(access_capability_name(_capability), _expected)
                 == 0 );
}


// ===========================================================================
// II.  READ-SURFACE FIXTURES
// ===========================================================================
//   A read surface is a VALUE observer reachable on a const instance.  Each
// fixture below isolates one of the four probes, or one way of missing them.

// DRwReadIndex
//   struct: const operator[](size_t) alone.
struct DRwReadIndex
{
    using value_type = int;

    const int& operator[](
        std::size_t _i
    ) const;
};

// DRwReadFront
//   struct: const front() alone.
struct DRwReadFront
{
    using value_type = int;

    const int& front() const;
};

// DRwReadData
//   struct: const data() alone.
struct DRwReadData
{
    using value_type = int;

    const int* data() const;
};

// DRwReadBegin
//   struct: const begin() alone - the tell that lets an associative container,
// which has none of the other three, still count as observable.
struct DRwReadBegin
{
    using value_type = int;

    const int* begin() const;
};

// DRwReadVoidBegin
//   struct: begin() returns void.  The probe names the call expression's type
// and never constrains it, so void counts - these are structural probes, not
// semantic ones.
struct DRwReadVoidBegin
{
    using value_type = int;

    void begin() const;
};

// DRwReadNonconstOnly
//   struct: the observers exist but none is const-qualified, so the const-
// instance probe misses every one of them.
struct DRwReadNonconstOnly
{
    using value_type = int;

    int& operator[](
        std::size_t _i
    );
    int& front();
    int* data();
    int* begin();
};

// DRwReadWrongIndexArg
//   struct: operator[] takes something a size_t will not convert to, so the
// index probe - which supplies a size_t - does not match it.
struct DRwReadWrongIndexArg
{
    using value_type = int;

    const int& operator[](
        const char* _key
    ) const;
};

// DRwReadPrivate
//   struct: the observer is private.  Access control participates in the
// probe, so this type has no read surface at all.
struct DRwReadPrivate
{
    using value_type = int;

    void clear();

private:
    const int* begin() const;
};

// DRwMetadataOnly
//   struct: size() and empty() and nothing else.  Value-free metadata is
// deliberately NOT a read surface - it reveals no element value - so this type
// is not observable, and having no mutator either it is not classifiable at
// all.
struct DRwMetadataOnly
{
    using value_type = int;

    std::size_t size() const;
    bool empty() const;
};


// ===========================================================================
// III. WRITE-SURFACE FIXTURES
// ===========================================================================
//   A write surface is a mutator reachable on a non-const instance.  Note that
// the three append probes name `typename clean_t<T>::value_type` while the
// clear probe does not - an asymmetry rw_clear_no_value_type and
// rw_push_back_no_value_type pin from either side.

// DRwWritePushBack
//   struct: push_back alone.
struct DRwWritePushBack
{
    using value_type = int;

    void push_back(
        int _value
    );
};

// DRwWriteEmplaceBack
//   struct: emplace_back alone.
struct DRwWriteEmplaceBack
{
    using value_type = int;

    void emplace_back(
        int _value
    );
};

// DRwWritePushFront
//   struct: push_front alone - the forward_list shape.
struct DRwWritePushFront
{
    using value_type = int;

    void push_front(
        int _value
    );
};

// DRwWriteClear
//   struct: clear() alone.
struct DRwWriteClear
{
    using value_type = int;

    void clear();
};

// DRwWriteAssignableIndex
//   struct: a non-const operator[] yielding an assignable lvalue - the
// std::array shape, mutable in place but not growable.
struct DRwWriteAssignableIndex
{
    using value_type = int;

    int& operator[](
        std::size_t _i
    );
    const int& operator[](
        std::size_t _i
    ) const;
};

// DRwWriteConstIndex
//   struct: BOTH operator[] overloads yield a const reference - the read_only
// handle shape.  Observation succeeds; assignment does not, so the type is
// readable but not writable.
struct DRwWriteConstIndex
{
    using value_type = int;

    const int& operator[](
        std::size_t _i
    );
    const int& operator[](
        std::size_t _i
    ) const;
};

// DRwWriteProxyIndex
//   struct: operator[] returns a PROXY that is itself assignable - the
// std::vector<bool> shape.  The probe asks whether the assignment expression
// is well-formed, not whether a reference came back, so this is writable.
struct DRwWriteProxyIndex
{
    using value_type = int;

    struct proxy
    {
        proxy& operator=(
            int _value
        );
        operator int() const;
    };

    proxy operator[](
        std::size_t _i
    );
    int operator[](
        std::size_t _i
    ) const;
};

// rw_push_back_no_value_type
//   struct: a perfectly good push_back, but no value_type alias.  The append
// probe names value_type to build its argument, so it cannot match - and this
// type has no write surface at all.
struct rw_push_back_no_value_type
{
    void push_back(
        int _value
    );
};

// rw_clear_no_value_type
//   struct: clear() and no value_type.  The clear probe needs no value_type,
// so this type IS writable - the asymmetry with the append probes.
struct rw_clear_no_value_type
{
    void clear();
};

// DRwWriteConstClear
//   struct: clear() is const-qualified.  The probe calls it on a non-const
// lvalue, which a const member function accepts, so a const clear() still
// reads as a mutator.
struct DRwWriteConstClear
{
    using value_type = int;

    const int* begin() const;
    void clear() const;
};

// DRwWritePrivate
//   struct: the mutator is private, so the type is observable only.
struct DRwWritePrivate
{
    using value_type = int;

    const int* begin() const;

private:
    void clear();
};


// ===========================================================================
// IV.  CAPABILITY-TAG FIXTURES
// ===========================================================================
//   The opt-in signal.  Only a `capability` naming a TYPE that carries both
// bits is trusted; anything else falls back to the structural surface.  Each
// fixture below is structurally read_write, so whatever it reports other than
// read_write is the tag speaking.

// DRwTagReadOnly / DRwTagWriteOnly / DRwTagReadWrite
//   structs: stamped with the library's own capability tags.
struct DRwTagReadOnly
{
    using value_type = int;
    using capability = read_only;

    const int* begin() const;
    void clear();
};

struct DRwTagWriteOnly
{
    using value_type = int;
    using capability = write_only;

    const int* begin() const;
    void clear();
};

struct DRwTagReadWrite
{
    using value_type = int;
    using capability = read_write;

    const int* begin() const;
    void clear();
};

// DRwTagOverrides
//   struct: structurally read_write - it has both surfaces - but stamped
// read_only.  The tag is honoured first, so the structural surface is never
// consulted.  This is the escape hatch the header documents.
struct DRwTagOverrides
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

// DRwTagNoneBits
//   struct: a tag declaring BOTH bits false, so the type opts out of the axis
// entirely despite having both structural surfaces.  Note this reaches the
// same capability as an unclassifiable type by a different route, which
// has_capability_tag distinguishes.
struct DRwTagNoneBits
{
    struct denied
    {
        static constexpr bool can_read  = false;
        static constexpr bool can_write = false;
    };

    using value_type = int;
    using capability = denied;

    const int* begin() const;
    void clear();
};

// DRwTagFunctions
//   struct: the bits are static member FUNCTIONS.  The probe names them
// happily and the read then decays each to a function pointer, which is never
// null - so BOTH bits read true and the type reports read_write whatever the
// functions would have returned.
//   Classifying this fixture makes GCC emit -Waddress at the point of use, in
// the header rather than here, which is the compiler agreeing with the test.
// To keep that expected diagnostic in one place it is classified ONLY in
// ..._tests_capability_bits.cpp; no other section TU should instantiate a
// trait on it.
struct DRwTagFunctions
{
    struct callable
    {
        static
        bool can_read();
        static
        bool can_write();
    };

    using value_type = int;
    using capability = callable;

    const int* begin() const;
};

// DRwTagPartial / DRwTagPartialWrite
//   structs: the tag carries one bit but not the other.  Both bits are
// required, so neither tag is trusted and the structural surface decides.
//   The pair is deliberately symmetric: a probe requiring only one of the two
// would still reject one of these fixtures, so it takes both to pin the
// conjunction.
struct DRwTagPartial
{
    struct half
    {
        static constexpr bool can_read = true;
    };

    using value_type = int;
    using capability = half;

    const int* begin() const;
    void clear();
};

struct DRwTagPartialWrite
{
    struct DOtherHalf
    {
        static constexpr bool can_write = true;
    };

    using value_type = int;
    using capability = DOtherHalf;

    const int* begin() const;
    void clear();
};

// DRwTagAliasNoBits
//   struct: `capability` names a type carrying no bits at all.
struct DRwTagAliasNoBits
{
    using value_type = int;
    using capability = int;

    const int* begin() const;
    void clear();
};

// DRwTagDataMember
//   struct: `capability` is a data member, not a type, so it cannot open a
// nested-name-specifier and the probe is ill-formed.
struct DRwTagDataMember
{
    using value_type = int;

    static constexpr int capability = 0;

    const int* begin() const;
    void clear();
};

// DRwTagPrivate
//   struct: the tag is private, so it is invisible and structure decides.
struct DRwTagPrivate
{
    using value_type = int;

    const int* begin() const;
    void clear();

private:
    using capability = read_only;
};


// ===========================================================================
// V.   CAPABILITY-CORNER FIXTURES
// ===========================================================================
//   One structurally-classified carrier per capability, for tests that need a
// representative of each rather than a probe of one signal.

// DRwSink
//   struct: an append-only sink - a mutator plus value-free metadata, and no
// value observer.  This is the carrier the metadata exclusion exists to make
// representable.
struct DRwSink
{
    using value_type = int;

    void push_back(
        int _value
    );
    std::size_t size() const;
    bool empty() const;
};

// DRwObserver
//   struct: const observers and no mutator - a read-only view.
struct DRwObserver
{
    using value_type = int;

    const int& operator[](
        std::size_t _i
    ) const;
    const int* begin() const;
    std::size_t size() const;
};

// DRwBoth
//   struct: both surfaces, structurally - the unrestricted baseline.
struct DRwBoth
{
    using value_type = int;

    const int* begin() const;
    void push_back(
        int _value
    );
};

// DRwInert
//   struct: neither surface - not access-classifiable.
struct DRwInert
{
    int unrelated;
};


// ===========================================================================
// VI.  STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   Every one of these reads read_write, so the standard library alone
// exercises exactly one of the four capabilities; the other three need the
// fixtures above.  They are still worth pinning, since HOW each reaches
// read_write differs - an associative through const iteration, a fixed array
// through assignable indexing.

using DRwVector      = std::vector<int>;
using DRwBoolVector = std::vector<bool>;
using DRwArray       = std::array<int, 3>;
using DRwDeque       = std::deque<int>;
using DRwList        = std::list<int>;
using DRwFlist       = std::forward_list<int>;
using DRwString      = std::string;
using DRwSet         = std::set<int>;
using DRwMap         = std::map<int, int>;
using DRwUmap        = std::unordered_map<int, int>;
using DRwPair        = std::pair<int, int>;


// ===========================================================================
// VII. TEST DECLARATIONS
// ===========================================================================

// I-a. The read surface           (..._tests_read_surface.cpp)
bool tests_has_const_index_helper();
bool tests_has_const_front_helper();
bool tests_has_const_data_helper();
bool tests_has_const_begin_helper();
bool tests_has_read_surface();
bool tests_read_surface_excludes_metadata();
bool tests_read_surface_std_types();

// I-b. The write surface          (..._tests_write_surface.cpp)
bool tests_has_push_back_helper();
bool tests_has_emplace_back_helper();
bool tests_has_push_front_helper();
bool tests_has_clear_helper();
bool tests_has_assignable_index_helper();
bool tests_has_write_surface();
bool tests_write_surface_value_type_asymmetry();
bool tests_write_surface_std_types();

// I-c. The capability tag and bits resolution
//                                 (..._tests_capability_bits.cpp)
bool tests_has_capability_bits_helper();
bool tests_capability_tag_shapes();
bool tests_access_bits_structural();
bool tests_access_bits_tagged();
bool tests_access_bits_precedence();
bool tests_capability_bits_cv_ref();

// II.  Capability classification  (..._tests_classification.cpp)
bool tests_access_capability_enum();
bool tests_access_capability_name();
bool tests_access_capability_name_total();
bool tests_access_capability_from_bits();
bool tests_access_capability_of();
bool tests_access_capability_of_members();
bool tests_access_capability_cv_ref();
bool tests_access_capability_companions();

// III. Predicate and tag re-emission
//                                 (..._tests_predicate_and_tag.cpp)
bool tests_is_read_write_container();
bool tests_is_read_write_std_types();
bool tests_access_tag_of();
bool tests_access_tag_of_none_is_void();
bool tests_access_tag_round_trip();
bool tests_predicate_cv_ref();
bool tests_predicate_companions();

// IV.  Aggregate snapshot         (..._tests_class.cpp)
bool tests_access_container_class_bits();
bool tests_access_container_class_kind();
bool tests_access_container_class_predicates();
bool tests_access_container_class_agreement();
bool tests_access_container_class_cv_ref();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_READ_WRITE_CONTAINER_TRAITS_
