/*******************************************************************************
* djinterp [testing]                          runtime_container_traits_tests.hpp
*
* Unit tests for runtime_container_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the runtime end of the
* Lifetime axis.  Every test function is declared here and defined in the
* section .cpp that matches the section of the header under test.
*
*   WHAT IS UNDER TEST.  The Lifetime axis asks WHEN a container's value can
* be settled - in a constant expression, at run time, or either.  This header
* names the runtime end of it: a type is a runtime container when it looks
* like a container AND its shared container_lifetime verdict is the runtime
* stage EXCLUSIVELY.
*
*   TWO CONJUNCTS, AND BOTH MATTER.  The shape half and the lifetime half fail
* independently, so the fixtures supply carriers that satisfy one without the
* other: rt_no_size has the lifetime but not the shape, rt_constexpr_size has
* the shape but not the lifetime.
*
*   is_runtime_only IS EXACT EQUALITY with lifetime::runtime, not a test of the
* runtime bit - so a verdict of `both`, `compile_time` or `none` all read
* false.  The last two are reachable only through the opt-in lifetime_category
* member, which rt_category_* supply; without them a suite would never see a
* container_lifetime outside {runtime, both} and the exactness would go
* unexercised.
*
*   THE SECOND UMBRELLA IS NOT WHAT ITS NAME SUGGESTS.
* requires_runtime_storage is documented as the "stronger signal", but it omits
* the container-shape conjunct entirely and so does NOT imply
* is_runtime_container.  The fixtures populate all four quadrants of the two
* predicates; tests_requires_runtime_storage_is_independent pins it.
*
*   FIXTURE NAMING.  Fixtures are prefixed rt_ and are free of commas, so they
* pass through the cv/ref macro without parenthesising.  Standard-library
* carriers with comma-bearing argument lists are typedef'd here for the same
* reason.
*
*
* path:      /tests/djinterp/core/container/traits/runtime_container_traits_tests.hpp
*                                          runtime_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Shape fixtures
      --------------

III.  Storage-signal fixtures
      -----------------------

IV.   Lifetime-verdict fixtures
      -------------------------

V.    Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

VI.   Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_RUNTIME_CONTAINER_TRAITS_
#define DJINTERP_TESTS_RUNTIME_CONTAINER_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/container/traits/runtime_container_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// rt_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = rt_check_failed(...)` on the failing path only.
inline bool
rt_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_RT_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_RT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::rt_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_RT_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_RT_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_RT_CV_REF_AGREE
//   macro: every probe in this header writes clean_t before asking its
// question, so all seven cv- and reference-qualified spellings of a type must
// answer alike - including the volatile ones.  Not usable on void or a
// function type, since `void&` cannot be formed.
#define D_RT_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_RT_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_RT_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_RT_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_RT_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_RT_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_RT_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_RT_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// rt_lifetime_count
//   constant: how many lifetimes the axis carries.  The verdict tests loop
// over the whole range rather than sampling it.
static const int rt_lifetime_count = 4;

// rt_lifetime_at
//   function: the _i-th lifetime, in increasing bit order.  Out-of-range
// indices clamp to none so a loop bug cannot read past the enum.
inline lifetime
rt_lifetime_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return lifetime::none;
        case 1:  return lifetime::compile_time;
        case 2:  return lifetime::runtime;
        case 3:  return lifetime::both;
        default: return lifetime::none;
    }
}

// rt_life_is
//   function: compares a lifetime's spelling against an expected literal.
inline bool
rt_life_is(
    lifetime    _life,
    const char* _expected
)
{
    return ( std::strcmp(lifetime_name(_life), _expected) == 0 );
}

// rt_has_value_type / rt_has_nested_type
//   traits: detect the two nested names std::integral_constant supplies.  The
// suite uses them to show that the macro-generated signals in this header are
// standard-conforming metafunctions while the hand-written umbrellas above
// them are not - see tests_classification_metafunction_shape.
template<typename _Type, typename = void>
struct rt_has_value_type : std::false_type
{};

template<typename _Type>
struct rt_has_value_type<_Type,
                         decltype((void)sizeof(typename _Type::value_type))>
    : std::true_type
{};

template<typename _Type, typename = void>
struct rt_has_nested_type : std::false_type
{};

template<typename _Type>
struct rt_has_nested_type<_Type,
                          decltype((void)sizeof(typename _Type::type))>
    : std::true_type
{};


// ===========================================================================
// II.  SHAPE FIXTURES
// ===========================================================================
//   The "looks like a container" half.  The probe asks for size() on a CONST
// instance, so const-ness and access control both participate.

// rt_shaped
//   struct: a plain runtime container shape - a const size() and a non-trivial
// destructor, so it is not a literal type.  The baseline positive carrier.
struct rt_shaped
{
    ~rt_shaped() {}

    std::size_t size() const;
};

// rt_no_size
//   struct: no size() at all, so no container shape - though its contents
// give it a runtime lifetime all the same.  The carrier that separates the two
// conjuncts of is_runtime_container.
struct rt_no_size
{
    ~rt_no_size() {}

    int unrelated;
};

// rt_size_nonconst
//   struct: size() exists but is not const-qualified, so the const-instance
// probe misses it.
struct rt_size_nonconst
{
    ~rt_size_nonconst() {}

    std::size_t size();
};

// rt_size_private
//   struct: size() is private, so it is invisible to the probe.
struct rt_size_private
{
    ~rt_size_private() {}

private:
    std::size_t size() const;
};

// rt_size_void
//   struct: size() returns void.  The probe names the call expression's type
// and never constrains it, so this counts - a structural probe, not a semantic
// one.
struct rt_size_void
{
    ~rt_size_void() {}

    void size() const;
};


// ===========================================================================
// III. STORAGE-SIGNAL FIXTURES
// ===========================================================================
//   The two signals that actively REQUIRE runtime storage.  Neither consults
// the container shape, which is what makes requires_runtime_storage
// independent of is_runtime_container rather than stronger than it.

// rt_allocator
//   struct: an allocator_type alias on a container shape.
struct rt_allocator
{
    ~rt_allocator() {}

    using allocator_type = int;

    std::size_t size() const;
};

// rt_reserve
//   struct: a reserve(size_type) on a container shape.
struct rt_reserve
{
    ~rt_reserve() {}

    std::size_t size() const;
    void        reserve(std::size_t _n);
};

// rt_allocator_no_shape
//   struct: an allocator_type alias and NO size().  requires_runtime_storage
// holds while is_runtime_container does not - the quadrant that shows the
// second umbrella does not imply the first.
struct rt_allocator_no_shape
{
    using allocator_type = int;
};

// rt_reserve_no_shape
//   struct: a reserve() and no size(), the same quadrant by the other signal.
struct rt_reserve_no_shape
{
    void reserve(std::size_t _n);
};

// rt_allocator_private
//   struct: the alias is private, so it is invisible.
struct rt_allocator_private
{
    ~rt_allocator_private() {}

    std::size_t size() const;

private:
    using allocator_type = int;
};

// rt_allocator_value
//   struct: `allocator_type` names a static data member rather than a type,
// so the alias probe - which asks for a nested TYPE - does not match it.
struct rt_allocator_value
{
    ~rt_allocator_value() {}

    static constexpr int allocator_type = 0;

    std::size_t size() const;
};

// rt_reserve_wrong_arg
//   struct: reserve() takes something a size_t will not convert to, so the
// probe - which supplies a size_t - does not match it.
struct rt_reserve_wrong_arg
{
    ~rt_reserve_wrong_arg() {}

    std::size_t size() const;
    void        reserve(const char* _tag);
};

// rt_reserve_const
//   struct: reserve() is const-qualified.  The probe calls it on a non-const
// lvalue, which a const member function accepts, so this still counts.
struct rt_reserve_const
{
    ~rt_reserve_const() {}

    std::size_t size() const;
    void        reserve(std::size_t _n) const;
};


// ===========================================================================
// IV.  LIFETIME-VERDICT FIXTURES
// ===========================================================================
//   Carriers for each of the four container_lifetime verdicts.  Two of them
// are reachable only through the opt-in lifetime_category member: the shared
// verdict is the lattice MEET of the size lifetime and the contents lifetime,
// and without an opt-in neither operand ever clears the runtime bit.

// rt_literal_plain
//   struct: a LITERAL type whose size() carries no constexpr signal.  Being
// literal is not enough - the meet takes the size lifetime's runtime-only
// reading - so this is a runtime container.  Worth pinning because the
// opposite is the natural guess.
struct rt_literal_plain
{
    std::size_t size() const { return 3; }
};

// rt_constexpr_size
//   struct: a literal type whose size() IS constexpr, so both operands of the
// meet carry the compile-time bit and the verdict is `both`.  Container-shaped
// but NOT runtime-only - the carrier that separates the second conjunct.
struct rt_constexpr_size
{
    constexpr std::size_t size() const { return 3; }
};

// rt_category_compile_time
//   struct: opts in to a compile_time contents lifetime while its size()
// carries no constexpr signal.  The meet of compile_time and runtime is
// `none`, so this is a container-shaped type whose lifetime is the lattice
// BOTTOM - and is_runtime_only, being exact equality, reads false.
struct rt_category_compile_time
{
    static constexpr lifetime lifetime_category = lifetime::compile_time;

    std::size_t size() const;
};

// rt_category_compile_time_sized
//   struct: the same opt-in with a constexpr size(), so the meet is
// compile_time exactly - the fourth verdict, and the other one that is false
// for a reason other than carrying the runtime bit.
struct rt_category_compile_time_sized
{
    static constexpr lifetime lifetime_category = lifetime::compile_time;

    constexpr std::size_t size() const { return 2; }
};

// rt_category_none
//   struct: opts in to the bottom lifetime directly.
struct rt_category_none
{
    static constexpr lifetime lifetime_category = lifetime::none;

    std::size_t size() const;
};

// rt_category_runtime
//   struct: opts in to a runtime contents lifetime explicitly, reaching the
// same verdict a plain non-literal type reaches structurally.
struct rt_category_runtime
{
    static constexpr lifetime lifetime_category = lifetime::runtime;

    std::size_t size() const;
};


// ===========================================================================
// V.   STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   std::array is the one standard container that is NOT runtime-only: its
// tuple_size gives the size a compile-time reading and its elements are
// literal, so the meet keeps the compile-time bit.  Every other standard
// container reads runtime.

using rt_vector = std::vector<int>;
using rt_array  = std::array<int, 3>;
using rt_string = std::string;
using rt_deque  = std::deque<int>;
using rt_list   = std::list<int>;
using rt_set    = std::set<int>;
using rt_map    = std::map<int, int>;
using rt_pair   = std::pair<int, int>;


// ===========================================================================
// VI.  TEST DECLARATIONS
// ===========================================================================

// I.   Structural signals          (..._tests_signals.cpp)
bool tests_has_size_accessor_signal();
bool tests_has_allocator_alias();
bool tests_has_reserve_method_signal();
bool tests_signals_are_independent();
bool tests_signals_std_types();
bool tests_signals_cv_ref();
bool tests_signals_companions();

// II.  Classification umbrella     (..._tests_classification.cpp)
bool tests_is_runtime_container();
bool tests_is_runtime_container_needs_shape();
bool tests_is_runtime_container_needs_runtime_only();
bool tests_runtime_only_is_exact_equality();
bool tests_requires_runtime_storage();
bool tests_requires_runtime_storage_is_independent();
bool tests_classification_std_types();
bool tests_classification_cv_ref();
bool tests_classification_companions();

// III. Aggregate snapshot          (..._tests_class.cpp)
bool tests_runtime_container_class_signals();
bool tests_runtime_container_class_verdicts();
bool tests_runtime_container_class_life();
bool tests_runtime_container_class_agreement();
bool tests_runtime_container_class_cv_ref();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_RUNTIME_CONTAINER_TRAITS_
