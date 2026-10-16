/*******************************************************************************
* djinterp [test]                                             concepts_tests.hpp
*
*   Declarations for the DTest concepts test suite plus the shared helper
* types and assertion macros used by every per-section translation unit.
* Mirrors the layout of type_traits_tests.hpp: one function per semantic
* section of concepts.hpp, all taking a single `test_handler&` parameter,
* dispatched by the runner via reset_counters() / handler.result().
*
*   STRUCTURE:
*     A.  Per-section test-function declarations (one per concepts.hpp
*         semantic section, in concepts.hpp's order).
*     B.  Shared assertion macros (DT_CONCEPT / DT_NOT_CONCEPT) that
*         both static_assert and record_assertion the predicate.
*     C.  Shared helper types - empty / trivial / polymorphic / abstract
*         / final / aggregate classes; an empty / non-empty struct; a
*         non-trivial class with user-provided special members; a class
*         template; a class with all the container-shaped nested types;
*         a minimal allocator; scoped / unscoped enums; callable types
*         (throwing and noexcept); etc.
*
*   ASSERTION PATTERN:
*     Each predicate is checked twice:
*       - at compile time via static_assert (build-time guarantee), and
*       - at run time via record_assertion (report-side mirror).
*     The two are bundled into DT_CONCEPT / DT_NOT_CONCEPT so the call
*     site stays single-line.  The runtime arm is what populates the
*     framework's test report; the compile-time arm is the hard fence.
*
*   NAMING CONVENTION:
*     Every per-section function takes its handler parameter as `_h`,
*     because the DT_CONCEPT / DT_NOT_CONCEPT macros refer to that name
*     unqualified.  Don't rename the parameter in the .cpp implementations
*     or the macros will fail to compile.
*
*   PORTABILITY:
*   C++20 minimum (concepts).  The suite is conditionally compiled out
* when D_ENV_CPP_FEATURE_LANG_CONCEPTS is unset, in which case each
* per-section function becomes a no-op stub that records a single
* `pending` status so the report still has a row per section.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_META_CONCEPTS_TESTS_
#define DJINTERP_TESTS_META_CONCEPTS_TESTS_ 1

// std
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include <djinterp/djinterp.hpp>
#include <djinterp/core/meta/concepts.hpp>
#include <djinterp/test/test_defaults.hpp>
#include <djinterp/test/test_handler.hpp>


// =============================================================================
// A.   ASSERTION MACROS
// =============================================================================

// DT_CONCEPT
//   macro: assert `expr` (a constexpr bool, typically a concept evaluation)
// is true at compile time AND record the assertion on the local `_h`
// test_handler reference for the runtime report.
#define DT_CONCEPT(expr)                                                      \
    do {                                                                      \
        static_assert((expr), "DT_CONCEPT failed: " #expr);                   \
        ::djinterp::test::record_assertion(_h, (expr), #expr);                \
    } while (0)

// DT_NOT_CONCEPT
//   macro: assert `expr` (a constexpr bool, typically a concept evaluation)
// is false at compile time AND record the assertion on the local `_h`
// test_handler reference for the runtime report.
#define DT_NOT_CONCEPT(expr)                                                  \
    do {                                                                      \
        static_assert(!(expr), "DT_NOT_CONCEPT failed: " #expr);              \
        ::djinterp::test::record_assertion(_h, !(expr), "!(" #expr ")");      \
    } while (0)



// =============================================================================
// B.   PER-SECTION TEST FUNCTION DECLARATIONS
// =============================================================================
// Order mirrors concepts.hpp:
//   0 . macros        - D_CONCEPT_HAS_* family
//   I . std_reexports - sections I.1 - I.4 standard library re-exports
//  II . fundamental   - section II.1
//  II . composite     - section II.2
//  II . properties    - section II.3
//  II . tuple         - section II.4
//  II . rules         - section II.5 class-definition rule concepts
//  II . container     - section II.6 container / allocator concepts
//  II . templates     - section II.7 template concepts
//  II . logical       - section II.8 logical concepts
//  II . invocable     - section II.9 invocable concepts
//  II . numeric       - section II.10 size / numeric concepts
//  II . pack          - section II.11 parameter pack concepts

bool concepts_tests_macros(test_handler& _test_handler, test_option _options...);
bool concepts_tests_std_reexports(test_handler& _test_handler, test_option _options...);
bool concepts_tests_fundamental(test_handler& _test_handler, test_option _options...);
bool concepts_tests_composite(test_handler& _test_handler, test_option _options...);
bool concepts_tests_properties(test_handler& _test_handler, test_option _options...);
bool concepts_tests_tuple(test_handler& _test_handler, test_option _options...);
bool concepts_tests_rules(test_handler& _test_handler, test_option _options...);
bool concepts_tests_container(test_handler& _test_handler, test_option _options...);
bool concepts_tests_templates(test_handler& _test_handler, test_option _options...);
bool concepts_tests_logical(test_handler& _test_handler, test_option _options...);
bool concepts_tests_invocable(test_handler& _test_handler, test_option _options...);
bool concepts_tests_numeric(test_handler& _test_handler, test_option _options...);
bool concepts_tests_pack(test_handler& _test_handler, test_option _options...);



// =============================================================================
// C.   SHARED HELPER TYPES
// =============================================================================
// Fixture types used across multiple test sections.  Kept at namespace
// scope inside djinterp::test::fixtures so each .cpp can pick what it
// needs by `using` the names locally.  Comments above each helper say
// which sections consume it.
//
// IMPORTANT: these are *fixtures*, not part of the production surface;
// they live in a sub-namespace specifically so they cannot collide with
// the djinterp:: names they're testing.



// -----------------------------------------------------------------------------
// C.1  Class fixtures (triviality / polymorphism / abstraction / finality)
// -----------------------------------------------------------------------------
// Consumed by: properties, rules, fundamental, composite.

// empty_class
//   helper: a class with no non-static data members - is_empty_v == true.
struct empty_class
{};

// trivial_class
//   helper: a class trivially constructible / copyable / destructible.
// All special members compiler-generated; classified as trivial AND
// standard-layout (i.e. POD).
struct trivial_class
{
    int  a;
    char b;
};

// nontrivial_class
//   helper: a class with at least one user-provided non-trivial special
// member.  Both `is_trivial` and `follows_rule_of_zero` should be false;
// `follows_rule_of_three` / `follows_rule_of_five` should still hold
// because the rule-of-N traits/concepts are surface-level (do the
// operations *exist*?), not deep ("are they trivial?").
class nontrivial_class
{
public:
    nontrivial_class()                                       { }
    nontrivial_class(const nontrivial_class&)                { }
    nontrivial_class(nontrivial_class&&) D_NOEXCEPT          { }
    nontrivial_class& operator=(const nontrivial_class&)
        { return *this; }
    nontrivial_class& operator=(nontrivial_class&&) D_NOEXCEPT
        { return *this; }
    ~nontrivial_class()                                      { }
};

// polymorphic_class
//   helper: declares a virtual function - is_polymorphic_v == true.
// Concrete, so is_abstract_v == false.
struct polymorphic_class
{
    virtual void poke() {}
    virtual ~polymorphic_class() {}
};

// abstract_class
//   helper: has a pure virtual function - is_abstract_v == true.
struct abstract_class
{
    virtual void poke() = 0;
    virtual ~abstract_class() {}
};

// final_class
//   helper: marked `final` - is_final_v == true.
struct final_class final
{};

// aggregate_class
//   helper: an aggregate - no user-provided ctors, no private members,
// no virtuals, no base classes.  is_aggregate_v == true.
struct aggregate_class
{
    int  x;
    char y;
};

// derived_from_polymorphic
//   helper: derives from polymorphic_class.  Used by derived_from and
// the all_derived_from concept.
struct derived_from_polymorphic : polymorphic_class
{};

// derived_concrete
//   helper: derives from polymorphic_class and provides poke().
struct derived_concrete : abstract_class
{
    void poke() override {}
};

// rule_of_zero_class
//   helper: a class with no user-provided special members.  All five
// special members are implicitly defined and trivial - this is the
// canonical Rule-of-Zero shape.
struct rule_of_zero_class
{
    int  m_int;
    char m_char;
};

// rule_of_three_class
//   helper: explicit copy ctor, copy assign, destructor.  Move ops are
// suppressed by the user-provided copy/dtor.  follows_rule_of_three
// should hold; follows_rule_of_five should be false (no move ops in
// this configuration).
class rule_of_three_class
{
public:
    rule_of_three_class()                                       { }
    rule_of_three_class(const rule_of_three_class&)             { }
    rule_of_three_class& operator=(const rule_of_three_class&)
        { return *this; }
    ~rule_of_three_class()                                      { }
};

// rule_of_five_class
//   helper: all five special members user-provided.  follows_rule_of_five
// should hold; follows_rule_of_three should also hold (it's a subset
// of the five).
class rule_of_five_class
{
public:
    rule_of_five_class()                                            { }
    rule_of_five_class(const rule_of_five_class&)                   { }
    rule_of_five_class(rule_of_five_class&&) D_NOEXCEPT             { }
    rule_of_five_class& operator=(const rule_of_five_class&)
        { return *this; }
    rule_of_five_class& operator=(rule_of_five_class&&) D_NOEXCEPT
        { return *this; }
    ~rule_of_five_class()                                           { }
};

// noncopyable_class
//   helper: copy operations explicitly deleted.  Used to probe the
// negative side of follows_rule_of_three / follows_rule_of_five and
// the standard copyable / copy_constructible concepts.
class noncopyable_class
{
public:
    noncopyable_class()                                  = default;
    noncopyable_class(const noncopyable_class&)          = delete;
    noncopyable_class& operator=(const noncopyable_class&)
                                                         = delete;
    noncopyable_class(noncopyable_class&&) D_NOEXCEPT    = default;
    noncopyable_class& operator=(noncopyable_class&&)    D_NOEXCEPT
                                                         = default;
    ~noncopyable_class()                                 = default;
};


// -----------------------------------------------------------------------------
// C.2  Enum fixtures
// -----------------------------------------------------------------------------
// Consumed by: fundamental.

// unscoped_enum_t
//   helper: a classic C-style unscoped enum.  is_enum_v && !is_scoped_enum_v.
enum unscoped_enum_t
{
    UE_A,
    UE_B
};

// scoped_enum_t
//   helper: an enum class (scoped).  is_enum_v && is_scoped_enum_v.
enum class scoped_enum_t
{
    A,
    B
};


// -----------------------------------------------------------------------------
// C.3  Union fixture
// -----------------------------------------------------------------------------
// Consumed by: fundamental.

// union_t
//   helper: a union with two members - is_union_v == true.
union union_t
{
    int   i;
    float f;
};


// -----------------------------------------------------------------------------
// C.4  Pointer-to-member fixtures
// -----------------------------------------------------------------------------
// Consumed by: fundamental.

// pointee_class
//   helper: target class for pointer-to-member tests.  Has one data
// member and one member function to take member pointers of.
struct pointee_class
{
    int  data;
    void member_fn() {}
};


// -----------------------------------------------------------------------------
// C.5  Container-shaped fixtures (nested types, size(), max_size, ...)
// -----------------------------------------------------------------------------
// Consumed by: container, templates.

// sized_container
//   helper: has size_type, value_type, iterator, const_iterator, and a
// size() member returning size_type.  Matches the container concepts in
// section II.6.
struct sized_container
{
    using value_type     = int;
    using size_type      = std::size_t;
    using iterator       = int*;
    using const_iterator = const int*;

    size_type size() const D_NOEXCEPT { return 0; }
};

// bounded_container
//   helper: like sized_container plus a `max_size` static constant of
// type `const size_type`.  Used by has_max_size_c and bounded_c.
struct bounded_container
{
    using value_type     = int;
    using size_type      = std::size_t;
    using iterator       = int*;
    using const_iterator = const int*;

    static const size_type max_size = 16;

    size_type size() const D_NOEXCEPT { return 0; }
};

// no_size_member
//   helper: has size_type but no size() method.  Negative case for
// `sizeable`.
struct no_size_member
{
    using size_type = std::size_t;
};

// no_size_type
//   helper: has size() but no size_type alias.  Negative case for
// `sizeable`.
struct no_size_type
{
    std::size_t size() const D_NOEXCEPT { return 0; }
};


// -----------------------------------------------------------------------------
// C.6  Allocator fixture
// -----------------------------------------------------------------------------
// Consumed by: container.  std::allocator<T> is itself an allocator and
// is also exercised; this fixture is the bare-minimum shape that still
// satisfies `allocator_c`.

// minimal_allocator
//   helper: the minimum surface allocator_c requires - value_type,
// allocate(n), and deallocate(p, n).  Mirrors the allocator concept
// requirements at section II.6.
template<typename _T>
struct minimal_allocator
{
    using value_type = _T;

    minimal_allocator() = default;

    template<typename _U>
    minimal_allocator(const minimal_allocator<_U>&) D_NOEXCEPT {}

    _T*  allocate(std::size_t _n)            { return static_cast<_T*>(::operator new(_n * sizeof(_T))); }
    void deallocate(_T* _p, std::size_t /*_n*/) D_NOEXCEPT { ::operator delete(_p); }

    bool operator==(const minimal_allocator&) const D_NOEXCEPT { return true; }
    bool operator!=(const minimal_allocator&) const D_NOEXCEPT { return false; }
};


// -----------------------------------------------------------------------------
// C.7  Template fixtures
// -----------------------------------------------------------------------------
// Consumed by: templates.

// empty_template
//   helper: a class template that takes zero (defaulted) args.  Its
// instantiation `empty_template<>` should match is_template_c.
template<typename...>
struct empty_template
{};

// args_template
//   helper: a class template with one or more args - its instantiation
// `args_template<int>` should match is_template_with_args_c but NOT
// is_template_c.
template<typename...>
struct args_template
{};

// nested_template_holder
//   helper: a class with a nested template alias named `type`.  Matches
// has_nested_template_type_c.
struct nested_template_holder
{
    template<typename _U>
    using type = _U*;
};

// template_with_base_value_type
//   helper: a class template whose value_type is itself a base of the
// template instantiation.  Matches template_parameter_base_of.  The
// declaration order matters: `value_type` must be the public base and
// the alias must name it before any forward-derivation tricks.
struct base_for_value_type
{};

struct template_with_base_value_type : base_for_value_type
{
    using value_type = base_for_value_type;
};


// -----------------------------------------------------------------------------
// C.8  Tuple fixtures
// -----------------------------------------------------------------------------
// Consumed by: tuple, pack.

// homogeneous_tuple_t
//   helper: a tuple where every element has the same type.
using homogeneous_tuple_t = std::tuple<int, int, int>;

// heterogeneous_tuple_t
//   helper: a tuple with mixed element types.
using heterogeneous_tuple_t = std::tuple<int, double, char>;

// empty_tuple_t
//   helper: a zero-element tuple.
using empty_tuple_t = std::tuple<>;

// single_element_tuple_t
//   helper: a one-element tuple.
using single_element_tuple_t = std::tuple<int>;


// -----------------------------------------------------------------------------
// C.9  Callable fixtures (invocable concepts)
// -----------------------------------------------------------------------------
// Consumed by: invocable, std_reexports.

// returns_int
//   helper: a callable taking int and returning int, NOT noexcept.
struct returns_int
{
    int operator()(int _x) const { return _x + 1; }
};

// returns_int_noexcept
//   helper: a callable taking int and returning int, declared noexcept.
struct returns_int_noexcept
{
    int operator()(int _x) const D_NOEXCEPT { return _x + 1; }
};

// returns_void
//   helper: a callable taking nothing and returning void.
struct returns_void
{
    void operator()() const { }
};


// -----------------------------------------------------------------------------
// C.10  Method-detection fixtures
// -----------------------------------------------------------------------------
// Consumed by: macros, container.

// method_haver
//   helper: a class with `foo()`, `bar(int)`, `baz() -> int`,
// `qux(int) -> int`, a nested type `nested`, and a static member
// `kStatic`.  Exercises every D_CONCEPT_HAS_* macro positively.
struct method_haver
{
    using nested = int;
    static const int kStatic = 42;

    void foo()                {}
    void bar(int /*arg*/)     {}
    int  baz()                { return 0; }
    int  qux(int _arg)        { return _arg + 1; }
};

// method_lacker
//   helper: an empty class.  Exercises every D_CONCEPT_HAS_* macro
// negatively.  Note: also lacks the nested-template alias used by the
// templates section, so it doubles as a negative fixture there.
struct method_lacker
{};


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_META_CONCEPTS_TESTS_
