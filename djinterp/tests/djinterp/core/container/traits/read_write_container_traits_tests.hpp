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
* representable.  rw_metadata_only and rw_sink pin it from both directions.
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
* path:      /tests/djinterp/core/container/traits/read_write_container_traits_tests.hpp
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
#include "../../../../../inc/djinterp/core/container/traits/read_write_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// rw_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = rw_check_failed(...)` on the failing path only.
inline bool
rw_check_failed(
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
inline access_capability
rw_capability_at(
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
inline bool
rw_name_is(
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

// rw_read_index
//   struct: const operator[](size_t) alone.
struct rw_read_index
{
    using value_type = int;

    const int& operator[](std::size_t _i) const;
};

// rw_read_front
//   struct: const front() alone.
struct rw_read_front
{
    using value_type = int;

    const int& front() const;
};

// rw_read_data
//   struct: const data() alone.
struct rw_read_data
{
    using value_type = int;

    const int* data() const;
};

// rw_read_begin
//   struct: const begin() alone - the tell that lets an associative container,
// which has none of the other three, still count as observable.
struct rw_read_begin
{
    using value_type = int;

    const int* begin() const;
};

// rw_read_void_begin
//   struct: begin() returns void.  The probe names the call expression's type
// and never constrains it, so void counts - these are structural probes, not
// semantic ones.
struct rw_read_void_begin
{
    using value_type = int;

    void begin() const;
};

// rw_read_nonconst_only
//   struct: the observers exist but none is const-qualified, so the const-
// instance probe misses every one of them.
struct rw_read_nonconst_only
{
    using value_type = int;

    int& operator[](std::size_t _i);
    int& front();
    int* data();
    int* begin();
};

// rw_read_wrong_index_arg
//   struct: operator[] takes something a size_t will not convert to, so the
// index probe - which supplies a size_t - does not match it.
struct rw_read_wrong_index_arg
{
    using value_type = int;

    const int& operator[](const char* _key) const;
};

// rw_read_private
//   struct: the observer is private.  Access control participates in the
// probe, so this type has no read surface at all.
struct rw_read_private
{
    using value_type = int;

    void clear();

private:
    const int* begin() const;
};

// rw_metadata_only
//   struct: size() and empty() and nothing else.  Value-free metadata is
// deliberately NOT a read surface - it reveals no element value - so this type
// is not observable, and having no mutator either it is not classifiable at
// all.
struct rw_metadata_only
{
    using value_type = int;

    std::size_t size() const;
    bool        empty() const;
};


// ===========================================================================
// III. WRITE-SURFACE FIXTURES
// ===========================================================================
//   A write surface is a mutator reachable on a non-const instance.  Note that
// the three append probes name `typename clean_t<T>::value_type` while the
// clear probe does not - an asymmetry rw_clear_no_value_type and
// rw_push_back_no_value_type pin from either side.

// rw_write_push_back
//   struct: push_back alone.
struct rw_write_push_back
{
    using value_type = int;

    void push_back(int _value);
};

// rw_write_emplace_back
//   struct: emplace_back alone.
struct rw_write_emplace_back
{
    using value_type = int;

    void emplace_back(int _value);
};

// rw_write_push_front
//   struct: push_front alone - the forward_list shape.
struct rw_write_push_front
{
    using value_type = int;

    void push_front(int _value);
};

// rw_write_clear
//   struct: clear() alone.
struct rw_write_clear
{
    using value_type = int;

    void clear();
};

// rw_write_assignable_index
//   struct: a non-const operator[] yielding an assignable lvalue - the
// std::array shape, mutable in place but not growable.
struct rw_write_assignable_index
{
    using value_type = int;

    int&       operator[](std::size_t _i);
    const int& operator[](std::size_t _i) const;
};

// rw_write_const_index
//   struct: BOTH operator[] overloads yield a const reference - the read_only
// handle shape.  Observation succeeds; assignment does not, so the type is
// readable but not writable.
struct rw_write_const_index
{
    using value_type = int;

    const int& operator[](std::size_t _i);
    const int& operator[](std::size_t _i) const;
};

// rw_write_proxy_index
//   struct: operator[] returns a PROXY that is itself assignable - the
// std::vector<bool> shape.  The probe asks whether the assignment expression
// is well-formed, not whether a reference came back, so this is writable.
struct rw_write_proxy_index
{
    using value_type = int;

    struct proxy
    {
        proxy& operator=(int _value);
        operator int() const;
    };

    proxy operator[](std::size_t _i);
    int   operator[](std::size_t _i) const;
};

// rw_push_back_no_value_type
//   struct: a perfectly good push_back, but no value_type alias.  The append
// probe names value_type to build its argument, so it cannot match - and this
// type has no write surface at all.
struct rw_push_back_no_value_type
{
    void push_back(int _value);
};

// rw_clear_no_value_type
//   struct: clear() and no value_type.  The clear probe needs no value_type,
// so this type IS writable - the asymmetry with the append probes.
struct rw_clear_no_value_type
{
    void clear();
};

// rw_write_const_clear
//   struct: clear() is const-qualified.  The probe calls it on a non-const
// lvalue, which a const member function accepts, so a const clear() still
// reads as a mutator.
struct rw_write_const_clear
{
    using value_type = int;

    const int* begin() const;
    void       clear() const;
};

// rw_write_private
//   struct: the mutator is private, so the type is observable only.
struct rw_write_private
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

// rw_tag_read_only / rw_tag_write_only / rw_tag_read_write
//   structs: stamped with the library's own capability tags.
struct rw_tag_read_only
{
    using value_type = int;
    using capability = read_only;

    const int* begin() const;
    void       clear();
};

struct rw_tag_write_only
{
    using value_type = int;
    using capability = write_only;

    const int* begin() const;
    void       clear();
};

struct rw_tag_read_write
{
    using value_type = int;
    using capability = read_write;

    const int* begin() const;
    void       clear();
};

// rw_tag_overrides
//   struct: structurally read_write - it has both surfaces - but stamped
// read_only.  The tag is honoured first, so the structural surface is never
// consulted.  This is the escape hatch the header documents.
struct rw_tag_overrides
{
    using value_type = int;
    using capability = read_only;

    const int& operator[](std::size_t _i) const;
    int&       operator[](std::size_t _i);
    void       push_back(int _value);
    void       clear();
};

// rw_tag_none_bits
//   struct: a tag declaring BOTH bits false, so the type opts out of the axis
// entirely despite having both structural surfaces.  Note this reaches the
// same capability as an unclassifiable type by a different route, which
// has_capability_tag distinguishes.
struct rw_tag_none_bits
{
    struct denied
    {
        static constexpr bool can_read  = false;
        static constexpr bool can_write = false;
    };

    using value_type = int;
    using capability = denied;

    const int* begin() const;
    void       clear();
};

// rw_tag_functions
//   struct: the bits are static member FUNCTIONS.  The probe names them
// happily and the read then decays each to a function pointer, which is never
// null - so BOTH bits read true and the type reports read_write whatever the
// functions would have returned.
//   Classifying this fixture makes GCC emit -Waddress at the point of use, in
// the header rather than here, which is the compiler agreeing with the test.
// To keep that expected diagnostic in one place it is classified ONLY in
// ..._tests_capability_bits.cpp; no other section TU should instantiate a
// trait on it.
struct rw_tag_functions
{
    struct callable
    {
        static bool can_read();
        static bool can_write();
    };

    using value_type = int;
    using capability = callable;

    const int* begin() const;
};

// rw_tag_partial / rw_tag_partial_write
//   structs: the tag carries one bit but not the other.  Both bits are
// required, so neither tag is trusted and the structural surface decides.
//   The pair is deliberately symmetric: a probe requiring only one of the two
// would still reject one of these fixtures, so it takes both to pin the
// conjunction.
struct rw_tag_partial
{
    struct half
    {
        static constexpr bool can_read = true;
    };

    using value_type = int;
    using capability = half;

    const int* begin() const;
    void       clear();
};

struct rw_tag_partial_write
{
    struct other_half
    {
        static constexpr bool can_write = true;
    };

    using value_type = int;
    using capability = other_half;

    const int* begin() const;
    void       clear();
};

// rw_tag_alias_no_bits
//   struct: `capability` names a type carrying no bits at all.
struct rw_tag_alias_no_bits
{
    using value_type = int;
    using capability = int;

    const int* begin() const;
    void       clear();
};

// rw_tag_data_member
//   struct: `capability` is a data member, not a type, so it cannot open a
// nested-name-specifier and the probe is ill-formed.
struct rw_tag_data_member
{
    using value_type = int;

    static constexpr int capability = 0;

    const int* begin() const;
    void       clear();
};

// rw_tag_private
//   struct: the tag is private, so it is invisible and structure decides.
struct rw_tag_private
{
    using value_type = int;

    const int* begin() const;
    void       clear();

private:
    using capability = read_only;
};


// ===========================================================================
// V.   CAPABILITY-CORNER FIXTURES
// ===========================================================================
//   One structurally-classified carrier per capability, for tests that need a
// representative of each rather than a probe of one signal.

// rw_sink
//   struct: an append-only sink - a mutator plus value-free metadata, and no
// value observer.  This is the carrier the metadata exclusion exists to make
// representable.
struct rw_sink
{
    using value_type = int;

    void        push_back(int _value);
    std::size_t size() const;
    bool        empty() const;
};

// rw_observer
//   struct: const observers and no mutator - a read-only view.
struct rw_observer
{
    using value_type = int;

    const int& operator[](std::size_t _i) const;
    const int* begin() const;
    std::size_t size() const;
};

// rw_both
//   struct: both surfaces, structurally - the unrestricted baseline.
struct rw_both
{
    using value_type = int;

    const int* begin() const;
    void       push_back(int _value);
};

// rw_inert
//   struct: neither surface - not access-classifiable.
struct rw_inert
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

using rw_vector      = std::vector<int>;
using rw_bool_vector = std::vector<bool>;
using rw_array       = std::array<int, 3>;
using rw_deque       = std::deque<int>;
using rw_list        = std::list<int>;
using rw_flist       = std::forward_list<int>;
using rw_string      = std::string;
using rw_set         = std::set<int>;
using rw_map         = std::map<int, int>;
using rw_umap        = std::unordered_map<int, int>;
using rw_pair        = std::pair<int, int>;


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
