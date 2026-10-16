/*******************************************************************************
* djinterp [testing]                        constexpr_container_traits_tests.hpp
*
*   Declarations, fixtures and the check macro for the constexpr_container_traits
* suite.  The section TUs define the tests_* functions declared at the bottom;
* the runner calls them.
*
*   WHAT THIS HEADER IS TESTING.
*   constexpr_container_traits.hpp answers one question - "is this container
* usable in constant evaluation?" - by composing two independent answers under
* the lifetime lattice of meta/lifetime.hpp:
*
*       container_lifetime<T> = meet( size_lifetime<T>, lifetime_of<T> )
*
* where size_lifetime asks "is the SIZE fixed by the type?" from five structural
* signals, and lifetime_of asks "are the CONTENTS constexpr-constructible?".  So
* the suite has three jobs, and they are genuinely different:
*     1. do the five signals fire on what they should, and only on that?
*     2. does the meet reproduce the lattice - all four values, both routes to
*        `none`?
*     3. do the umbrella and the snapshot faithfully restate the two?
*
*   THE PROBES ARE NOT ALL SAFE.
*   Two of them can HARD-ERROR rather than report false, and neither error is
* catchable - both land outside the immediate context of the substitution:
*
*     has_constexpr_size_expression   probes `T{}.size()`.  The temporary `T{}`
*                                     must also be DESTROYED inside the constant
*                                     expression, and a failure inside the
*                                     destructor's BODY is not a substitution
*                                     failure.  std::array<std::string, N> - the
*                                     very type the header's own comment names -
*                                     breaks the build.
*     has_constexpr_container_tag     inherits directly from the opt-in member
*                                     alias.  If that alias is not a class type
*                                     with a ::value, the base-clause is
*                                     ill-formed, and a bad base-clause is never
*                                     SFINAE.
*
*   Fixtures that trigger either are quarantined behind D_CCT_HAZARD_TESTS
* (default 0, see below) so the suite always BUILDS.  Turning the flag on is the
* reproduction.  The hazards TU tests the safe neighbours unconditionally, so
* the boundary itself is under test even when the mines are not.
*
*   COMPLETENESS.
*   The five signals and size_lifetime all tolerate an INCOMPLETE type.  Nothing
* downstream does: lifetime_of routes through is_literal_type, which routes
* through std::is_trivially_destructible, whose mandate is a complete type.  So
* container_lifetime, is_constexpr_container, is_not_constexpr_container and
* constexpr_container_class hard-error on an incomplete type.  The batteries
* below therefore run the signals over D_TEST_HOSTILE_TYPES and the umbrella
* over D_TEST_HOSTILE_TYPES_COMPLETE, and tests_incomplete_type pins exactly
* where the line falls.
*
*   LIBRARY-VERSION SENSITIVITY.
*   `T{}.size()` being a constant expression is a property of the LIBRARY, not
* just the language: C++20 made std::vector and std::string constexpr, so the
* probe now fires on both.  Those expectations are gated on the standard
* feature-test macros rather than assumed, so the suite states the right answer
* on whatever library it is built against.
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests.hpp
*                                      constexpr_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CHECK MACRO
      -----------

II.   HAZARD GATE
      -----------

III.  FIXTURES
      --------
      i.    opt-in tag shapes
      ii.   extent shapes
      iii.  size-expression shapes
      iv.   tuple_size conformers
      v.    constexpr-iteration shapes
      vi.   lifetime_category drivers  (to steer the meet)
      vii.  plain / negative

IV.   TEST DECLARATIONS
      -----------------
*/

#ifndef DJINTERP_TESTING_CONSTEXPR_CONTAINER_TRAITS_TESTS_
#define DJINTERP_TESTING_CONSTEXPR_CONTAINER_TRAITS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../../../inc/djinterp/core/container/traits/constexpr_container_traits.hpp"    // the header under test
#include "djinterp/test/test_traits.hpp"     // the trait-testing toolkit


// D_CCT_HAZARD_TESTS
//   macro: gates the two families of fixture that make the traits HARD-ERROR
// rather than report false.  Zero by default, because a suite that does not
// build reports nothing at all; define it to 1 to reproduce either hazard.
//
//     -DD_CCT_HAZARD_TESTS=1
//
//   The build then fails, which IS the finding.  See tests_hazard_destructor
// and tests_hazard_tag_base for the exact shapes and the reasoning.
#ifndef D_CCT_HAZARD_TESTS
    #define D_CCT_HAZARD_TESTS  0
#endif  // D_CCT_HAZARD_TESTS


NS_DJINTERP
NS_TESTING


// the trait-testing toolkit, by name.  The traits under test need no
// qualification: djinterp::testing nests inside djinterp, so `is_constexpr_
// container`, `lifetime`, `lifetime_name` and friends are found by ordinary
// unqualified lookup.
using ::djinterp::test::count_holds;
using ::djinterp::test::holds_for_all;
using ::djinterp::test::holds_for_any;
using ::djinterp::test::holds_for_none;
using ::djinterp::test::cvref_report;
using ::djinterp::test::trait_across_cvref;
using ::djinterp::test::trait_ignores_cvref;
using ::djinterp::test::is_bool_trait;
using ::djinterp::test::trait_is_well_formed;


///////////////////////////////////////////////////////////////////////////////
///                I.   CHECK MACRO                                          ///
///////////////////////////////////////////////////////////////////////////////

// cct_report_failure
//   function: prints one failed check, naming the assertion, its file and its
// line, so a red line in the report identifies the assertion rather than only
// the function that contained it.
D_INLINE void
cct_report_failure(
    const char* _file,
    int         _line,
    const char* _expression
)
{
    std::printf("      !! %s:%d\n         %s\n",
                _file,
                _line,
                _expression);

    return;
}

// D_CCT_CHECK
//   macro: folds one boolean check into the enclosing test's verdict, and
// reports it if it fails.  Expects a local `bool ok` in scope - every tests_*
// function in this suite opens with `bool ok = true;` and closes with
// `return ok;`.  Variadic, so a check containing a template-argument comma
// passes through intact.
#define D_CCT_CHECK(...)                                                      \
    do                                                                        \
    {                                                                         \
        const bool cct_result = static_cast<bool>(__VA_ARGS__);               \
                                                                              \
        if (!cct_result)                                                      \
        {                                                                     \
            ::djinterp::testing::cct_report_failure(__FILE__,                 \
                                                    __LINE__,                 \
                                                    #__VA_ARGS__);            \
        }                                                                     \
                                                                              \
        ok = (ok && cct_result);                                              \
    } while (false)

// D_CCT_LIFE
//   macro: a lifetime equality check that, on failure, PRINTS the lifetime it
// actually got.  `expected both, got runtime` is a bug report; `false` is a
// puzzle.  The lattice has four values and the interesting failures are the
// ones that land on the wrong one, not the ones that land on nothing.
#define D_CCT_LIFE(EXPECTED, ...)                                             \
    do                                                                        \
    {                                                                         \
        D_CONSTEXPR ::djinterp::lifetime cct_actual = (__VA_ARGS__);          \
                                                                              \
        if (cct_actual != (EXPECTED))                                         \
        {                                                                     \
            std::printf("      !! %s:%d\n"                                    \
                        "         %s\n"                                       \
                        "         expected %s, got %s\n",                     \
                        __FILE__,                                             \
                        __LINE__,                                             \
                        #__VA_ARGS__,                                         \
                        ::djinterp::lifetime_name(EXPECTED),                  \
                        ::djinterp::lifetime_name(cct_actual));               \
        }                                                                     \
                                                                              \
        ok = ( ok && (cct_actual == (EXPECTED)) );                            \
    } while (false)


///////////////////////////////////////////////////////////////////////////////
///                III. FIXTURES                                             ///
///////////////////////////////////////////////////////////////////////////////

// i.   opt-in tag shapes
//////////////////////////////////////////
//   has_constexpr_container_tag detects `typename clean_t<_Type>::
// is_constexpr_container` and then INHERITS FROM IT.  The inheritance is the
// whole mechanism - it is how present-and-false_type comes out false - and it
// is also the hazard: whatever the member names becomes a base class.

// tag_true
//   struct: the intended opt-in - an explicit claim of static-lifetime
// capability.
struct tag_true
{
    using is_constexpr_container = std::true_type;
};

// tag_false
//   struct: the explicit opt-OUT.  The trait reports false for it, which is
// the documented behaviour - but see tag_false_with_extent for what that
// actually buys.
struct tag_false
{
    using is_constexpr_container = std::false_type;
};

// true_ish
//   struct: a user bool_constant, for tag_derived below.
struct true_ish : std::true_type
{
};

// tag_derived
//   struct: opts in with a type DERIVED from std::true_type rather than
// true_type itself.  The trait inherits it, so ::value is found through the
// base and the opt-in still works.
struct tag_derived
{
    using is_constexpr_container = true_ish;
};

// tag_private
//   struct: the opt-in alias is private.  Access checking is part of
// substitution, so the probe declines rather than diagnosing.
struct tag_private
{
private:
    using is_constexpr_container = std::true_type;
};

// tag_true_nonliteral
//   struct: claims static-lifetime capability while holding a std::string.  It
// is not constexpr-constructible in any meaningful sense, and the tag is
// believed anyway: container_lifetime short-circuits to `both` without ever
// consulting lifetime_of.  The tag is an UNCHECKED assertion, and this is the
// fixture that says so.
struct tag_true_nonliteral
{
    using is_constexpr_container = std::true_type;

    std::string payload;
};

// tag_false_with_extent
//   struct: opts OUT explicitly, and carries a static extent.  The opt-out is
// simply one false disjunct among five: the extent promotes the size anyway,
// and the type comes out constexpr.  An explicit denial denies nothing.
struct tag_false_with_extent
{
    using is_constexpr_container = std::false_type;

    static constexpr std::size_t extent = 4;
};


#if D_CCT_HAZARD_TESTS

// tag_not_a_bool_constant
//   struct: opts in with something that is not a class type.  The trait's
// success specialization inherits from it - `struct ... : int` - and a
// base-clause is never in the immediate context, so this is a HARD ERROR, not
// a false.  Quarantined; see tests_hazard_tag_base.
struct tag_not_a_bool_constant
{
    using is_constexpr_container = int;
};

// tag_valueless_base
//   struct: opts in with a class type that carries no ::value.  The trait
// inherits it happily and then fails to find `value` - again a hard error, and
// again outside any immediate context.
struct tag_valueless_base
{
    struct nope
    {
    };

    using is_constexpr_container = nope;
};

#endif  // D_CCT_HAZARD_TESTS


// ii.  extent shapes
//////////////////////////////////////////
//   has_constexpr_extent probes `decltype(clean_t<_Type>::extent)` - the same
// expression bounded_container_traits.hpp probes under the name
// has_fixed_extent_signal, and it inherits the same over-detection.

// extent_static
//   struct: the intended shape - a static compile-time extent.
struct extent_static
{
    static constexpr std::size_t extent = 8;
};

// extent_zero
//   struct: an extent of ZERO.  A capacity of nothing is still a compile-time
// capacity, so the signal must fire on a falsy value.
struct extent_zero
{
    static constexpr std::size_t extent = 0;
};

// extent_data_member
//   struct: a NON-static data member named `extent`.  Naming a non-static
// member in an unevaluated operand is well-formed, so the signal fires - an
// ordinary field is enough to make a type "compile-time sized".
struct extent_data_member
{
    std::size_t extent;
};

// extent_static_function
//   struct: a static member FUNCTION named `extent`.  decltype yields its
// function type, so the signal fires here too.
struct extent_static_function
{
    static std::size_t extent();
};

// extent_member_function
//   struct: a non-static member function.  Naming it without an object is
// ill-formed, so the signal correctly declines.
struct extent_member_function
{
    std::size_t extent();
};

// extent_nested_type
//   struct: a nested TYPE named `extent`.  decltype takes an expression, not a
// type - correctly declined.
struct extent_nested_type
{
    using extent = std::size_t;
};

// extent_private
//   struct: a private static extent - correctly declined.
struct extent_private
{
private:
    static constexpr std::size_t extent = 8;
};

// extent_enumerator
//   struct: an ENUMERATOR named `extent`.  A compile-time constant by another
// name, and the signal fires - probably the right answer, but not one the
// trait's comment mentions.
struct extent_enumerator
{
    enum
    {
        extent = 8
    };
};

// extent_nonliteral
//   struct: a static extent AND a std::string.  The SIZE is compile-time; the
// CONTENTS are not.  The meet is what decides, and it decides runtime - this is
// the fixture that proves the two halves of the composition are really
// independent.
struct extent_nonliteral
{
    static constexpr std::size_t extent = 4;

    std::string payload;
};


// iii. size-expression shapes
//////////////////////////////////////////
//   has_constexpr_size_expression probes `std::integral_constant<std::size_t,
// clean_t<_Type>{}.size()>`.  Three things must hold at once: T is
// default-constructible, T{} is a constant expression (construction AND
// destruction), and size() is constexpr and yields a value convertible to
// size_t as a non-type template argument.  Each fixture below isolates one.

// size_constexpr
//   struct: the intended shape - default-constructible, literal, constexpr
// size().
struct size_constexpr
{
    constexpr std::size_t
    size() const
    {
        return 4;
    }
};

// size_constexpr_nonconst
//   struct: a NON-const constexpr size().  The probe calls it on the prvalue
// `T{}`, not on a const lvalue, so a non-const member is callable and the
// signal fires.  A deliberate widening relative to the const-lvalue probes
// elsewhere in the framework - and one worth pinning, since it is invisible
// until someone writes a mutating size().
struct size_constexpr_nonconst
{
    constexpr std::size_t
    size()
    {
        return 4;
    }
};

// size_runtime
//   struct: size() is const but not constexpr, so the template argument is not
// a constant expression - correctly declined.
struct size_runtime
{
    std::size_t
    size() const
    {
        return 4;
    }
};

// size_no_default_ctor
//   struct: constexpr size(), and a DELETED default constructor.  One would
// expect the probe to decline, and on C++20 it does.  On C++17 it does not -
// and the reason is worth the paragraph.
//
//   `T{}` is LIST-initialization, not default-construction.  For an AGGREGATE
// it performs aggregate initialization, which never consults a constructor at
// all.  And in C++17 a class with a DELETED default constructor is still an
// aggregate: the aggregate rule of the day excluded classes with user-PROVIDED
// constructors, and `= delete` is user-DECLARED but not user-provided.  So
//
//     std::is_default_constructible<size_no_default_ctor>   ==  false
//     has_constexpr_size_expression<size_no_default_ctor>   ==  TRUE   (C++17)
//
//   C++20's P1008R1 ("Prohibit aggregates with user-declared constructors")
// closed that hole; on C++20 the type is not an aggregate, `T{}` calls the
// deleted constructor, and the probe declines.  The trait therefore gives a
// DIFFERENT ANSWER on C++17 and C++20 for the same type - and the header
// advertises a C++11 baseline.
//
//   is_constexpr_container survives it either way, because lifetime_of routes
// through is_trivially_default_constructible, which is false for the deleted
// case on every standard, and the meet clears the compile-time bit.  The right
// verdict from a wrong intermediate, for the third time (see the vector/string
// and destructor findings).  The common cause is not three bugs but one: `T{}`
// inside a probe does not mean what it looks like it means.
struct size_no_default_ctor
{
    size_no_default_ctor() = delete;

    constexpr std::size_t
    size() const
    {
        return 4;
    }
};

// size_explicit_ctor
//   struct: constexpr size(), and a user-PROVIDED constructor taking an
// argument.  Not an aggregate on any standard, and genuinely not
// default-constructible, so the probe declines everywhere.  This is the fixture
// that carries "no T{} means no signal" unconditionally, now that
// size_no_default_ctor turns out not to.
struct size_explicit_ctor
{
    constexpr
    size_explicit_ctor(int)
    {
    }

    constexpr std::size_t
    size() const
    {
        return 4;
    }
};

// size_returns_int
//   struct: size() returns int.  4 converts to size_t in a converted constant
// expression, so the signal fires - the return TYPE is not pinned, only the
// value's convertibility.
struct size_returns_int
{
    constexpr int
    size() const
    {
        return 4;
    }
};

// size_returns_negative
//   struct: size() returns -1.  Converting -1 to size_t in a converted
// constant expression is a NARROWING, which is ill-formed - so the signal
// declines.  A container that reports a negative size is rejected on a
// technicality rather than on principle, but it is rejected.
struct size_returns_negative
{
    constexpr int
    size() const
    {
        return -1;
    }
};

// size_returns_pointer
//   struct: size() returns a pointer, which is not convertible to size_t as a
// template argument - correctly declined.
struct size_returns_pointer
{
    constexpr const char*
    size() const
    {
        return "";
    }
};

// size_user_destructor
//   struct: constexpr size(), but a user-provided destructor that is NOT
// marked constexpr.  The temporary `T{}` must be destroyed inside the constant
// expression, and a non-constexpr destructor cannot be - correctly declined.
struct size_user_destructor
{
    constexpr std::size_t
    size() const
    {
        return 4;
    }

    ~size_user_destructor()
    {
    }
};

// size_constexpr_destructor
//   struct: the same, with the destructor marked constexpr - and now the
// signal fires.  The pair pins the destructor as a real part of the probe, not
// an incidental one, which is what makes the hazard in tests_hazard_destructor
// possible at all.
//
//   C++20 ONLY.  Constexpr destructors are P0784R7; before C++20 a class with
// ANY user-provided destructor is non-literal, so the positive half of this
// pair cannot be written at all and the negative half (size_user_destructor)
// carries the point on its own.  The dependent checks are gated to match.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

struct size_constexpr_destructor
{
    constexpr std::size_t
    size() const
    {
        return 4;
    }

    constexpr ~size_constexpr_destructor()
    {
    }
};

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

// size_virtual
//   struct: a virtual member makes the type non-literal, so `T{}` is not a
// constant expression - correctly declined.
struct size_virtual
{
    virtual ~size_virtual() = default;

    std::size_t
    size() const
    {
        return 4;
    }
};

// size_with_string_member
//   struct: constexpr size(), and a std::string member.  On a C++20 library,
// std::string's default constructor and destructor are BOTH constexpr and a
// short string allocates nothing, so `T{}` IS a constant expression and the
// signal fires - for a type nobody would call compile-time-sized.  The final
// verdict survives only because lifetime_of rescues it through the meet.
struct size_with_string_member
{
    std::string payload;

    constexpr std::size_t
    size() const
    {
        return 4;
    }
};


// iv.  tuple_size conformers
//////////////////////////////////////////

// tuple_conformer
//   struct: a fixed-size aggregate that says so the canonical way, by
// specializing std::tuple_size (below).  It has NO tag, NO extent, NO size(),
// NO constexpr iteration - the tuple_size disjunct alone promotes it.  And
// tuple_size is the one signal constexpr_container_class does not report, so
// this is the fixture whose snapshot reads "no signals" while its verdict reads
// "constexpr".
struct tuple_conformer
{
};

// tuple_conformer_nonliteral
//   struct: the same opt-in, with non-literal contents.  Compile-time SIZE,
// runtime CONTENTS - the meet says runtime.
struct tuple_conformer_nonliteral
{
    std::string payload;
};


// v.   constexpr-iteration shapes
//////////////////////////////////////////
//   has_constexpr_iteration (constexpr_iterator_traits.hpp) is the fifth
// disjunct.  It wants a nested `constexpr_iterator` alias OR a
// constexpr_begin()/constexpr_end() PAIR - either half alone is not enough.

// iter_alias
//   struct: opts in with the nested alias.
struct iter_alias
{
    using constexpr_iterator = const int*;
};

// iter_pair
//   struct: opts in with the accessor pair.
struct iter_pair
{
    constexpr const int*
    constexpr_begin() const
    {
        return nullptr;
    }

    constexpr const int*
    constexpr_end() const
    {
        return nullptr;
    }
};

// iter_begin_only
//   struct: half the pair.  has_constexpr_begin_method fires; the umbrella
// does not.
struct iter_begin_only
{
    constexpr const int*
    constexpr_begin() const
    {
        return nullptr;
    }
};

// iter_end_only
//   struct: the other half.
struct iter_end_only
{
    constexpr const int*
    constexpr_end() const
    {
        return nullptr;
    }
};


// vi.  lifetime_category drivers
//////////////////////////////////////////
//   container_lifetime is the MEET of two lattice values, and the only way to
// exercise the meet table is to steer both operands independently.  These four
// pin lifetime_of via the opt-in `lifetime_category` member (lifetime.hpp's
// highest-priority signal) while carrying an extent, which pins size_lifetime
// to `both`.  meet(both, X) = X, so each drives container_lifetime to a
// different corner of the lattice.

// life_both_sized
//   struct: extent + lifetime_category = both  ->  meet(both, both) = both.
struct life_both_sized
{
    static constexpr std::size_t extent           = 4;
    static constexpr lifetime    lifetime_category = lifetime::both;
};

// life_compile_sized
//   struct: extent + lifetime_category = compile_time
//           ->  meet(both, compile_time) = compile_time.
// A container that exists ONLY at compile time - constexpr-capable, and not
// usable at runtime.
struct life_compile_sized
{
    static constexpr std::size_t extent           = 4;
    static constexpr lifetime    lifetime_category = lifetime::compile_time;
};

// life_runtime_sized
//   struct: extent + lifetime_category = runtime
//           ->  meet(both, runtime) = runtime.
struct life_runtime_sized
{
    static constexpr std::size_t extent           = 4;
    static constexpr lifetime    lifetime_category = lifetime::runtime;
};

// life_none_sized
//   struct: extent + lifetime_category = none  ->  meet(both, none) = none.
// The bottom of the lattice, reached by an explicit declaration.
struct life_none_sized
{
    static constexpr std::size_t extent           = 4;
    static constexpr lifetime    lifetime_category = lifetime::none;
};

// life_compile_unsized
//   struct: lifetime_category = compile_time and NO size signal at all.
//           ->  meet(runtime, compile_time) = NONE.
// The second, and much less obvious, route to the bottom: contents that exist
// only at compile time, in a container whose size does not.  The two stages have
// no overlap, so the composite has none - which is exactly what the lattice is
// for, and exactly what a boolean "is it constexpr?" could never have said.
struct life_compile_unsized
{
    static constexpr lifetime lifetime_category = lifetime::compile_time;
};


// vii. plain / negative
//////////////////////////////////////////

// no_signal
//   struct: fires nothing.  Note the verdict: its CONTENTS are literal (an
// empty struct is trivially constructible and destructible, so lifetime_of is
// `both`), but its SIZE is not compile-time-expressible, so the meet is
// runtime and it is NOT a constexpr container.  Being constexpr-capable and
// being a constexpr CONTAINER are different claims, and this is the fixture
// that separates them.
struct no_signal
{
};

// no_signal_nonliteral
//   struct: fires nothing and holds a std::string.  Runtime on both halves.
struct no_signal_nonliteral
{
    std::string payload;
};


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                III.iv  TUPLE_SIZE SPECIALIZATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   The std::tuple_size primary template is declared and never defined, which
// is exactly what makes has_tuple_size SFINAE-clean for std::vector.  A
// conforming fixed-size aggregate opts in by specializing it.

namespace std
{

    // tuple_size<djinterp::testing::tuple_conformer>
    //   trait: opts the conformer into the fixed-size-aggregate protocol.
    template<>
    struct tuple_size< ::djinterp::testing::tuple_conformer>
        : integral_constant<size_t, 5>
    {
    };

    // tuple_size<djinterp::testing::tuple_conformer_nonliteral>
    //   trait: the same opt-in for the non-literal conformer.
    template<>
    struct tuple_size< ::djinterp::testing::tuple_conformer_nonliteral>
        : integral_constant<size_t, 5>
    {
    };

}  // namespace std


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                IV.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// I.   compile-time-expressible-size signals
bool tests_tag_basic();
bool tests_tag_access();
bool tests_extent_basic();
bool tests_extent_shapes();
bool tests_size_expression_basic();
bool tests_size_expression_constness();
bool tests_size_expression_return_value();
bool tests_size_expression_constructibility();
bool tests_size_expression_destructor();
bool tests_tuple_size();
bool tests_constexpr_iteration();
bool tests_signal_value_companions();

// II.  lifetime of a container
bool tests_size_lifetime_disjuncts();
bool tests_size_lifetime_negative();
bool tests_size_lifetime_is_binary();
bool tests_size_lifetime_opt_out_ignored();
bool tests_container_lifetime_meet();
bool tests_container_lifetime_lattice();
bool tests_container_lifetime_tag_bypass();
bool tests_container_lifetime_type();
bool tests_container_lifetime_value_companion();

// III. classification umbrella
bool tests_is_constexpr_positive();
bool tests_is_constexpr_negative();
bool tests_is_constexpr_is_the_compile_time_bit();
bool tests_is_not_constexpr();
bool tests_classification_exhaustive();
bool tests_classification_value_companions();

// IV.  aggregate snapshot
bool tests_class_members();
bool tests_class_agrees_with_traits();
bool tests_class_tuple_size_gap();
bool tests_class_is_constexpr_and_empty();

// V.   robustness  (cross-cutting)
bool tests_cvref_signals();
bool tests_cvref_verdicts();
bool tests_hostile_types_rejected();
bool tests_hostile_types_survived();
bool tests_incomplete_type();
bool tests_trait_shape();

// VI.  standard library
bool tests_stdlib_array();
bool tests_stdlib_vector_string();
bool tests_stdlib_node_containers();
bool tests_stdlib_tuple_pair();

// VII. hazards  (the hard-error boundary)
bool tests_hazard_destructor();
bool tests_hazard_tag_base();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_CONSTEXPR_CONTAINER_TRAITS_TESTS_
