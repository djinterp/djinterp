/******************************************************************************
* djinterp [testing]                                     test_traits_tests.hpp
*
*   Declarations, subjects and check macros for the test_traits suite.  The
* section TUs define the tests_* functions declared at the bottom; the runner
* calls them.
*
*   TESTING A TEST TOOL.
*   test_traits.hpp is the vocabulary every other trait suite in the framework
* asks its questions in.  If it is wrong, every suite built on it is wrong in
* the same direction and none of them says so - a broken cv-ref matrix reports
* "agrees" for a trait that does not, a broken is_bool_trait reports "well
* formed" for a trait that is not, and the bugs those checks exist to find go on
* not being found.  So this suite is the one place in the framework where the
* subjects have to be KNOWN-good and KNOWN-bad by construction rather than by
* the tool's own say-so.
*
*   That is what the subjects below are.  Every trait in section i is a trait
* whose correct answer is derivable from its definition in one line - always
* true, always false, true only for int - and every MIS-SHAPED trait is broken
* in exactly one named way, so that when is_bool_trait rejects it the suite
* knows WHICH clause did the rejecting.  Nothing here is inferred from the tool
* under test.
*
*   THE THREE THINGS THAT CANNOT BE CHECKED AT RUN TIME.
*   Three of the toolkit's guarantees are guarantees about COMPILATION, and a
* passing run says nothing about any of them:
*
*     count_holds does not short-circuit    a trait that hard-errors on a late
*                                           cell must break the BUILD - that is
*                                           the whole reason the header does not
*                                           use std::conjunction
*     D_TEST_STATIC fails the build         by design
*     the probes are SFINAE-clean           an ill-formed probe must report
*                                           false, not diagnose
*
*   Each is demonstrated in the pins TU: the SAFE half unconditionally (that
* std::conjunction really does short-circuit, so the contrast is real), and the
* half that breaks the build behind D_TT_HAZARD_TESTS.  A suite that does not
* compile reports nothing, so the mines are off by default and turning them on
* IS the reproduction.
*
*
* path:      /tests/djinterp/test/test_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CHECK MACROS
II.   HAZARD GATE
III.  SUBJECTS
      i.    well-shaped traits
      ii.   mis-shaped traits          (one named defect each)
      iii.  `_v` companions            (one honest, one stale)
      iv.   cv-ref subjects            (one per first_disagreement branch)
      v.    subject types
      vi.   probes                     (one per declarator in section I)
      vii.  concept subjects           (C++20)
IV.   TEST DECLARATIONS
*/

#ifndef DJINTERP_TESTING_TEST_TRAITS_TESTS_
#define DJINTERP_TESTING_TEST_TRAITS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>
// djinterp
#include "test_traits.hpp"      // the header under test


// D_TT_HAZARD_TESTS
//   macro: gates the four demonstrations whose failure mode is the BUILD, not
// the report.  Zero by default, because a suite that does not compile reports
// nothing at all; define it to 1 to reproduce all four:
//
//     -DD_TT_HAZARD_TESTS=1
//
//   The build then fails - four times, each with a named diagnostic - and that
// is the point.  See tests_build_time_hazards.
#ifndef D_TT_HAZARD_TESTS
    #define D_TT_HAZARD_TESTS   0
#endif  // D_TT_HAZARD_TESTS


NS_DJINTERP
NS_TESTING


// the toolkit under test, by name.  djinterp::testing nests inside djinterp, so
// the detection idiom (is_detected, detected_t, nonesuch, ...) is already
// reachable unqualified; the test:: names are not, and are pulled in here
// rather than with a using-directive so that what this suite is exercising is
// visible at the top of the file.
using ::djinterp::test::count_holds;
using ::djinterp::test::count_holds_v;
using ::djinterp::test::cvref_report;
using ::djinterp::test::holds_for_all;
using ::djinterp::test::holds_for_all_v;
using ::djinterp::test::holds_for_any;
using ::djinterp::test::holds_for_any_v;
using ::djinterp::test::holds_for_none;
using ::djinterp::test::holds_for_none_v;
using ::djinterp::test::is_bool_trait;
using ::djinterp::test::is_bool_trait_v;
using ::djinterp::test::is_nothrow_probe;
using ::djinterp::test::is_nothrow_probe_v;
using ::djinterp::test::is_valid;
using ::djinterp::test::trait_across_cvref;
using ::djinterp::test::trait_ignores_cvref;
using ::djinterp::test::trait_ignores_cvref_v;
using ::djinterp::test::trait_is_well_formed;
using ::djinterp::test::trait_is_well_formed_v;
using ::djinterp::test::yields_lvalue;
using ::djinterp::test::yields_lvalue_v;
using ::djinterp::test::yields_prvalue;
using ::djinterp::test::yields_prvalue_v;
using ::djinterp::test::yields_xvalue;
using ::djinterp::test::yields_xvalue_v;

namespace fixtures = ::djinterp::test::fixtures;


///////////////////////////////////////////////////////////////////////////////
///                I.   CHECK MACROS                                         ///
///////////////////////////////////////////////////////////////////////////////

// tt_report_failure
//   function: prints one failed check, naming the assertion, its file and its
// line, so a red line in the report identifies the assertion rather than only
// the function that contained it.
D_INLINE void
tt_report_failure(
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

// tt_name_or_null
//   function: renders a first_disagreement() result for printing - the cell
// name, or the literal word "(none)" for the nullptr that means "every cell
// agreed".
D_INLINE const char*
tt_name_or_null(
    const char* _name
)
{
    return (_name != nullptr) ? _name : "(none)";
}

// tt_names_match
//   function: compares two first_disagreement() results, either of which may be
// nullptr.  A plain strcmp would dereference the nullptr that means "agrees",
// which is the single most likely way to write this check wrong.
D_INLINE bool
tt_names_match(
    const char* _expected,
    const char* _actual
)
{
    if ((_expected == nullptr) || (_actual == nullptr))
    {
        return (_expected == _actual);
    }

    return (std::strcmp(_expected, _actual) == 0);
}

// D_TT_CHECK
//   macro: folds one boolean check into the enclosing test's verdict, and
// reports it if it fails.  Expects a local `bool ok` in scope - every tests_*
// function in this suite opens with `bool ok = true;` and closes with
// `return ok;`.  Variadic, so a check containing a template-argument comma
// passes through intact.
#define D_TT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        const bool tt_result = static_cast<bool>(__VA_ARGS__);                \
                                                                              \
        if (!tt_result)                                                       \
        {                                                                     \
            ::djinterp::testing::tt_report_failure(__FILE__,                  \
                                                   __LINE__,                  \
                                                   #__VA_ARGS__);             \
        }                                                                     \
                                                                              \
        ok = (ok && tt_result);                                               \
    } while (false)

// D_TT_FIRST
//   macro: checks that trait_across_cvref<TRAIT, TYPE>().first_disagreement()
// names the cell EXPECTED - a string literal, or nullptr for "every cell
// agrees" - and, on failure, PRINTS the cell it actually named.
//
//   `expected const _Type&, got _Type&` is a bug report.  `false` is a puzzle,
// and the whole reason cvref_report keeps its cells rather than ANDing them
// together is so that the report can say the former.  A suite that then checks
// it with a bare boolean has thrown that away again.
#define D_TT_FIRST(EXPECTED, TRAIT, ...)                                      \
    do                                                                        \
    {                                                                         \
        D_CONSTEXPR ::djinterp::test::cvref_report tt_report =                \
            ::djinterp::test::trait_across_cvref<TRAIT, __VA_ARGS__>();       \
                                                                              \
        const char* const tt_actual = tt_report.first_disagreement();         \
        const bool        tt_match  =                                         \
            ::djinterp::testing::tt_names_match((EXPECTED), tt_actual);       \
                                                                              \
        if (!tt_match)                                                        \
        {                                                                     \
            std::printf("      !! %s:%d\n"                                    \
                        "         first_disagreement of %s<%s>\n"             \
                        "         expected %s, got %s\n",                     \
                        __FILE__,                                             \
                        __LINE__,                                             \
                        #TRAIT,                                               \
                        #__VA_ARGS__,                                         \
                        ::djinterp::testing::tt_name_or_null(EXPECTED),       \
                        ::djinterp::testing::tt_name_or_null(tt_actual));     \
        }                                                                     \
                                                                              \
        ok = (ok && tt_match);                                                \
    } while (false)


///////////////////////////////////////////////////////////////////////////////
///                III. SUBJECTS                                             ///
///////////////////////////////////////////////////////////////////////////////

// i.   well-shaped traits
//////////////////////////////////////////
//   Correct by inspection: each derives from an integral_constant, so each is a
// bool trait by construction, and each one's answer is readable off its own
// definition without consulting anything under test.

// always_true
//   trait: true for every type, including void, function types, arrays and the
// incomplete fixture - the parameter is never touched.  The unit for
// count_holds: `count_holds<always_true, Pack...>` is `sizeof...(Pack)`, which
// is how the hostile lists' cardinalities are pinned.
template<typename>
struct always_true : std::true_type
{
};

// always_false
//   trait: false for every type.  The zero for count_holds.
template<typename>
struct always_false : std::false_type
{
};

// is_int
//   trait: true for `int` and nothing else.  The selective subject - a
// quantifier that cannot tell 1 from 0 or from N is not a quantifier.
template<typename _Type>
struct is_int : std::is_same<_Type, int>
{
};

// is_the_incomplete_fixture
//   trait: true only for fixtures::incomplete.  Used to prove that the
// _COMPLETE hostile lists really are the full lists MINUS that one fixture,
// rather than merely shorter.  std::is_same never requires a complete type, so
// this is askable of the fixture it is about.
template<typename _Type>
struct is_the_incomplete_fixture
    : std::is_same<_Type, fixtures::incomplete>
{
};

// has_member_value_type
//   trait: emitted by trait_detect.hpp, so it is `template<typename, typename =
// void>` - TWO parameters, the second defaulted.  The quantifiers in section
// III take a ONE-parameter template-template argument, and this binds to it
// only because of C++17's P0522.  That is the framework's whole trait style, so
// if the binding ever stops working every suite stops compiling; this subject is
// what says so first.
D_TYPE_TRAIT_HAS_TYPE(has_member_value_type, value_type)

// poison
//   struct: the one type detonates_on_poison refuses to be instantiated on.
struct poison
{
};

// detonates_on_poison
//   trait: true for int, false for everything else - and its instantiation on
// `poison` fires a static_assert.
//
//   This is the subject that makes section III's central claim TESTABLE.
// std::conjunction short-circuits, so
//
//     std::conjunction< detonates_on_poison<int>,      // true  - instantiated
//                       detonates_on_poison<char>,     // false - instantiated
//                       detonates_on_poison<poison> >  // never instantiated
//
// compiles, because conjunction stops at the false and never reaches the third.
// count_holds does not short-circuit, so
//
//     count_holds<detonates_on_poison, int, char, poison>
//
// expands every cell into a template-argument list, instantiates all three, and
// does not compile.  That difference is the whole reason this header does not
// use std::conjunction, and it is the difference between a trait test that finds
// a non-SFINAE-friendly trait and one that hides it.
//
//   The safe half runs unconditionally (tests_no_short_circuit); the half that
// breaks the build is gated (tests_build_time_hazards).
template<typename _Type>
struct detonates_on_poison : std::is_same<_Type, int>
{
    static_assert(!std::is_same<_Type, poison>::value,
                  "count_holds instantiated this cell -- which is exactly the "
                  "point: it does not short-circuit");
};


// ii.  mis-shaped traits
//////////////////////////////////////////
//   is_bool_trait has a void_t sink naming five members and then three
// predicates over them.  Eight ways to fail, and a subject that fails two at
// once proves nothing about either.  Each trait below is broken in exactly ONE
// named way, so a false from is_bool_trait is attributable.

// value_only
//   trait: a `static constexpr bool value` and nothing else - no
// integral_constant base, so no value_type, no ::type, no conversion to bool,
// no call operator.  The SINK fails at its first name.
//
//   Not hypothetical: this is the exact shape of is_bounded_container and
// is_unbounded_container in bounded_container_traits.hpp, and finding it is
// what is_bool_trait was written for.
template<typename>
struct value_only
{
    static constexpr bool value = true;
};

// shape_mimic
//   trait: carries every member the contract names - value_type, type, value, a
// conversion to bool, a call operator - and derives from NOTHING.  The sink
// passes; the is_base_of PREDICATE fails.
//
//   The subtlest of the eight, and the one a hand-rolled shape check written
// with void_t alone would miss entirely.
template<typename>
struct shape_mimic
{
    using value_type = bool;
    using type       = ::djinterp::bool_constant<true>;

    static constexpr bool value = true;

    D_CONSTEXPR operator bool() const D_NOEXCEPT
    {
        return true;
    }

    D_CONSTEXPR bool operator()() const D_NOEXCEPT
    {
        return true;
    }
};

// private_base
//   trait: derives from std::true_type PRIVATELY.  std::is_base_of does not
// check accessibility and reports true, so the predicate alone would pass - but
// the inherited conversion to bool is unreachable, so the SINK fails on its
// static_cast.  The two halves of is_bool_trait catch different things, and
// this subject is the one that proves the sink is doing work the predicates
// cannot.
template<typename>
struct private_base : private std::true_type
{
};

// true_base_a / true_base_b
//   struct: two distinct types, each a std::true_type.  Halves of
// ambiguous_base.
struct true_base_a : std::true_type
{
};

struct true_base_b : std::true_type
{
};

// ambiguous_base
//   trait: derives from bool_constant<true> twice, by two routes.  is_base_of
// reports true (it does not care about ambiguity), but the inherited conversion
// operator is ambiguous - so, again, the SINK is what rejects it.
template<typename>
struct ambiguous_base : true_base_a,
                        true_base_b
{
};

// lying_type
//   trait: `::value` is true and `::type` is std::false_type.  Every member is
// present and the derivation is right; the type/value AGREEMENT predicate is
// what fails.
//
//   A trait whose ::type disagrees with its ::value breaks exactly the code that
// tag-dispatches on the former while branching on the latter - which is to say,
// the code that never gets tested, because both spellings "work".
template<typename>
struct lying_type : std::true_type
{
    using type = std::false_type;
};

// nonstatic_value
//   trait: shadows the inherited static `value` with a NON-static data member.
// `_Trait::value` is then an id-expression naming a non-static member outside a
// member function - ill-formed - so the sink's integral_constant<bool, value>
// cell fails.
template<typename>
struct nonstatic_value : std::true_type
{
    bool value = true;
};

// int_valued
//   trait: derives from integral_constant<int, 1>.  `value` is 1, and 1 fits in
// a bool, so the sink's `integral_constant<bool, _Trait::value>` cell FORMS -
// and then the value_type predicate rejects it, because value_type is int.
//
//   This is the shape a type gets by opting in with the wrong integral_constant,
// and it is a shape that silently works everywhere `::value` is read in a
// boolean context and breaks nowhere until something reads value_type.
template<typename>
struct int_valued : std::integral_constant<int, 1>
{
};

// wide_valued
//   trait: derives from integral_constant<int, 3>.  Now `value` is 3, which does
// NOT fit in a bool, so `integral_constant<bool, 3>` is a narrowing conversion in
// a converted constant expression - ill-formed - and the SINK rejects it before
// any predicate is reached.
//
//   The pair with int_valued is deliberate: same defect, two different rejection
// paths, and only a subject on each path proves both are live.
template<typename>
struct wide_valued : std::integral_constant<int, 3>
{
};

// half_bad
//   trait: a well-formed bool trait for every type EXCEPT char, for which the
// explicit specialization degrades to a bare `value`.  trait_is_well_formed must
// therefore be true for <int> and false for <int, char> - it is a per-TYPE
// property, not a property of the template, and a shape check that instantiated
// the trait once and generalized would get this wrong.
template<typename>
struct half_bad : std::true_type
{
};

template<>
struct half_bad<char>
{
    static constexpr bool value = true;
};


// iii. `_v` companions
//////////////////////////////////////////
//   D_TEST_TRAIT_V_AGREES exists to catch a `_v` that has drifted from its
// trait.  A test that only ever feeds it agreeing pairs proves nothing; the
// stale one below is what proves the macro can fail.

// honest
//   trait: with a `_v` that agrees.
template<typename>
struct honest : std::true_type
{
};

template<typename _Type>
constexpr bool honest_v = honest<_Type>::value;

// stale
//   trait: with a `_v` that is INVERTED - the shape a `_v` takes when its trait
// is re-based and the shorthand is not updated with it.  Every call site that
// prefers the shorthand silently gets the opposite answer, and nothing but
// D_TEST_TRAIT_V_AGREES says so.
template<typename>
struct stale : std::true_type
{
};

template<typename _Type>
constexpr bool stale_v = !stale<_Type>::value;


// iv.  cv-ref subjects
//////////////////////////////////////////
//   cvref_report::first_disagreement() is a seven-way ternary chain with an
// eighth fall-through.  Eight branches, and the only way to reach the Nth is a
// trait that agrees on the first N-1 cells and disagrees on the Nth - so there
// is one subject per branch below, each disagreeing on exactly one cell of the
// eight, in the chain's own order.
//
//   All are asked about `int`.

// cvref_blind
//   trait: agrees on every cell (it strips before deciding).  The nullptr
// branch - the one that means "this trait is cv-ref agnostic".
template<typename _Type>
struct cvref_blind : std::is_same<clean_t<_Type>, int>
{
};

// hates_const
//   trait: disagrees first at `const _Type`.
template<typename _Type>
struct hates_const : ::djinterp::bool_constant<!std::is_const<_Type>::value>
{
};

// hates_volatile
//   trait: agrees on const, disagrees first at `volatile _Type`.
template<typename _Type>
struct hates_volatile
    : ::djinterp::bool_constant<!std::is_volatile<_Type>::value>
{
};

// hates_cv
//   trait: agrees on const and on volatile SEPARATELY, and disagrees only when
// both are present - `const volatile _Type`.
template<typename _Type>
struct hates_cv
    : ::djinterp::bool_constant<
        !( std::is_const<_Type>::value && std::is_volatile<_Type>::value )>
{
};

// hates_lvalue_ref
//   trait: disagrees first at `_Type&`.
template<typename _Type>
struct hates_lvalue_ref
    : ::djinterp::bool_constant<!std::is_lvalue_reference<_Type>::value>
{
};

// hates_const_lvalue_ref
//   trait: tolerates `_Type&` and disagrees only at `const _Type&` - the form a
// by-const-reference parameter hands a trait, and the cell the header's own
// comment calls the one most often forgotten.
template<typename _Type>
struct hates_const_lvalue_ref
    : ::djinterp::bool_constant<
        !( std::is_lvalue_reference<_Type>::value &&
           std::is_const<
               typename std::remove_reference<_Type>::type>::value )>
{
};

// hates_rvalue_ref
//   trait: disagrees first at `_Type&&`.
template<typename _Type>
struct hates_rvalue_ref
    : ::djinterp::bool_constant<!std::is_rvalue_reference<_Type>::value>
{
};

// hates_const_rvalue_ref
//   trait: disagrees only at the last cell, `const _Type&&` - the fall-through
// of the chain, and the one a report that stopped one cell short would miss.
template<typename _Type>
struct hates_const_rvalue_ref
    : ::djinterp::bool_constant<
        !( std::is_rvalue_reference<_Type>::value &&
           std::is_const<
               typename std::remove_reference<_Type>::type>::value )>
{
};


// v.   subject types
//////////////////////////////////////////

// sized
//   struct: the positive subject for every probe declarator at once - a nested
// type, a prvalue accessor, an lvalue accessor, an xvalue accessor, a binary
// call, and a non-throwing swap.  Members are declared and not defined; a probe
// only ever names them in an unevaluated operand.
struct sized
{
    using value_type = int;

    std::size_t size() const;     // prvalue
    int&        front();          // lvalue
    int&&       take();           // xvalue
    int         at(int _index) const;

    void swap(sized& _other) D_NOEXCEPT;
};

// plain
//   struct: the negative subject.  Nothing at all, so every probe above is
// ill-formed for it - and must therefore report FALSE rather than diagnose.
struct plain
{
};

// throwing_swap
//   struct: has the swap, and it is not noexcept.  The middle row of the
// noexcept probe's three: not "ill-formed", not "non-throwing", but "valid and
// throwing" - a row an is_nothrow_probe test that only used `sized` and `plain`
// would never reach.
struct throwing_swap
{
    void swap(throwing_swap& _other);
};

// runtime_valued
//   struct: constructible from an int, and NOT constexpr.  The subject that
// separates D_TEST_CONSTEXPR_PROBE's two failure modes, which the header's own
// comment warns it conflates: `runtime_valued(7).value` is well-formed - an
// EXPR probe detects it - and is not a constant expression, so a CONSTEXPR probe
// does not.  Detected-by-one-and-not-the-other is exactly "compiles, but is not
// constexpr", and nothing else in the fixture zoo says it.
struct runtime_valued
{
    runtime_valued(int _value);

    int value;
};

// probe_holder
//   struct: a probe declared at CLASS scope.  The declarators emit alias
// templates, and the header claims they may be declared "at namespace or class
// scope"; this is the claim, exercised.  A member alias template is still a
// valid template-template argument, so is_detected reads it exactly as it reads
// a namespace-scope probe.
struct probe_holder
{
    D_TEST_TYPE_PROBE(nested_value_type, typename _Type::value_type)
};


// vi.  probes
//////////////////////////////////////////
//   One per declarator, plus the awkward ones.  Namespace scope, so every
// section TU shares them.

// p_value_type
//   probe: D_TEST_TYPE_PROBE - a nested type.
D_TEST_TYPE_PROBE(p_value_type, typename _Type::value_type)

// p_common
//   probe: D_TEST_TYPE_PROBE_2, whose body contains a TOP-LEVEL COMMA.  The
// preprocessor does not treat `<>` as protecting a comma - only parentheses do -
// so `std::common_type_t<_Type, _Other>` reaches the macro as TWO arguments and
// is only reassembled because the declarators are variadic.  A non-variadic
// spelling of these macros would reject this probe, which is the commonest shape
// a binary type probe takes.
D_TEST_TYPE_PROBE_2(p_common, std::common_type_t<_Type, _Other>)

// p_size
//   probe: D_TEST_EXPR_PROBE yielding a PRVALUE.
D_TEST_EXPR_PROBE(p_size, std::declval<const _Type&>().size())

// p_front
//   probe: D_TEST_EXPR_PROBE yielding an LVALUE.
D_TEST_EXPR_PROBE(p_front, std::declval<_Type&>().front())

// p_take
//   probe: D_TEST_EXPR_PROBE yielding an XVALUE.
D_TEST_EXPR_PROBE(p_take, std::declval<_Type&>().take())

// p_at
//   probe: D_TEST_EXPR_PROBE_2 - the binary form, over `_Type` and `_Other`.
D_TEST_EXPR_PROBE_2(p_at, std::declval<const _Type&>().at(
                              std::declval<_Other>()))

// p_swap_noexcept
//   probe: D_TEST_NOEXCEPT_PROBE - well-formed iff the swap is, and its ::value
// reports whether that swap can throw.
D_TEST_NOEXCEPT_PROBE(p_swap_noexcept,
                      std::declval<_Type&>().swap(std::declval<_Type&>()))

// p_default_expr / p_default_ce
//   probes: the SAME expression under an EXPR probe and a CONSTEXPR probe.  The
// pair is the only way to tell "ill-formed" from "well-formed but not constant",
// which the constexpr declarator conflates by design.
D_TEST_EXPR_PROBE(p_default_expr, _Type())
D_TEST_CONSTEXPR_PROBE(p_default_ce, _Type())

// p_literal_expr / p_literal_ce
//   probes: the same pair over an int-taking constructor, for fixtures::literal
// and for runtime_valued.
D_TEST_EXPR_PROBE(p_literal_expr, _Type(7).value)
D_TEST_CONSTEXPR_PROBE(p_literal_ce, _Type(7).value)

// p_sizeof_ce
//   probe: D_TEST_CONSTEXPR_PROBE over sizeof - a constant expression for every
// complete type, INCLUDING a non-literal one, and ill-formed for void.  The
// subject that shows the constexpr declarator is about the EXPRESSION, not about
// the type's literalness.
D_TEST_CONSTEXPR_PROBE(p_sizeof_ce, sizeof(_Type))

// p_convertible_to_size
//   probe: an expression that succeeds by CONVERSION rather than by member
// lookup.  fixtures::greedy satisfies it and satisfies no member-shaped probe -
// which is the separation that fixture exists to make.
D_TEST_EXPR_PROBE(p_convertible_to_size,
                  static_cast<std::size_t>(std::declval<const _Type&>()))


// vii. concept subjects  (C++20)
//////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// value_typed_c
//   concept: the face of has_member_value_type, forwarding to it.  Agrees with
// the trait by construction, which is what D_TEST_TRAIT_CONCEPT_AGREE should
// confirm.
template<typename _Type>
concept value_typed_c = has_member_value_type<_Type>::value;

// value_typed_drifted_c
//   concept: the same face, TIGHTENED - it has grown a clause the trait has not.
// This is what "the concept and the trait agree by construction" looks like the
// day after somebody hardens one of them, and D_TEST_TRAIT_CONCEPT_AGREE must
// report false for it.
template<typename _Type>
concept value_typed_drifted_c =
    has_member_value_type<_Type>::value && (!std::is_class_v<_Type>);

// sized_refined_c
//   concept: written IN TERMS OF value_typed_c.  The refinement ladder built the
// right way - the atomic constraints of the weaker concept literally appear in
// the stronger one, so the compiler can see the subsumption.
template<typename _Type>
concept sized_refined_c = value_typed_c<_Type> &&
                          requires(const _Type& _subject)
                          {
                              _subject.size();
                          };

// sized_restated_c
//   concept: the same requirements, COPY-PASTED rather than composed.  It
// implies value_typed_c - every type satisfying it satisfies that - and it does
// not SUBSUME it, because the two `requires` expressions are distinct atomic
// constraints as far as the compiler is concerned.  Every implication test
// passes; every pair of overloads constrained on the two is ambiguous.
//
//   This is the ladder-shaped bug D_TEST_DECLARE_SUBSUMES exists to find, and
// nothing else in the language finds it until the ambiguous call site appears.
template<typename _Type>
concept sized_restated_c = requires
                           {
                               typename _Type::value_type;
                           } &&
                           requires(const _Type& _subject)
                           {
                               _subject.size();
                           };

// unrelated_c
//   concept: shares nothing with value_typed_c.  Neither subsumes the other, and
// a type satisfying only this one satisfies CONCEPT_MORE without satisfying
// CONCEPT_LESS - the row that proves the emitted trait's `CONCEPT_LESS<_Type> &&`
// guard is doing work.
template<typename _Type>
concept unrelated_c = requires(const _Type& _subject)
                      {
                          _subject.size();
                      };

// refines_correctly
//   trait: does sized_refined_c subsume value_typed_c?  It should.
D_TEST_DECLARE_SUBSUMES(refines_correctly, sized_refined_c, value_typed_c)

// merely_implies
//   trait: does sized_restated_c subsume value_typed_c?  It must NOT - it only
// implies.
D_TEST_DECLARE_SUBSUMES(merely_implies, sized_restated_c, value_typed_c)

// subsumes_nothing
//   trait: two unrelated concepts.  Neither subsumes the other.
D_TEST_DECLARE_SUBSUMES(subsumes_nothing, unrelated_c, value_typed_c)

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                IV.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// I.    probe declarators
bool tests_type_probe();
bool tests_type_probe_2();
bool tests_expr_probe();
bool tests_expr_probe_2();
bool tests_noexcept_probe();
bool tests_constexpr_probe();
bool tests_probe_scope();

// II.   reading a probe
bool tests_is_nothrow_probe();
bool tests_yields_lvalue();
bool tests_yields_xvalue();
bool tests_yields_prvalue();
bool tests_is_valid();
bool tests_is_valid_evil();
bool tests_reading_value_companions();

// III.  type-set quantifiers
bool tests_count_holds();
bool tests_count_holds_empty_pack();
bool tests_holds_for_all();
bool tests_holds_for_any();
bool tests_holds_for_none();
bool tests_quantifier_binding();
bool tests_no_short_circuit();
bool tests_quantifier_value_companions();

// IV.   trait shape
bool tests_is_bool_trait_positive();
bool tests_is_bool_trait_sink();
bool tests_is_bool_trait_predicates();
bool tests_is_bool_trait_non_traits();
bool tests_trait_is_well_formed();
bool tests_trait_v_agrees();
bool tests_shape_self_application();
bool tests_shape_value_companions();

// V.    cv-ref agreement
bool tests_cvref_cells();
bool tests_cvref_report_accessors();
bool tests_cvref_report_first_disagreement();
bool tests_trait_across_cvref();
bool tests_trait_ignores_cvref();
bool tests_cvref_value_companion();

// VI.   fixtures
bool tests_fixture_shapes();
bool tests_fixture_private_members();
bool tests_fixture_ambiguous_members();
bool tests_fixture_greedy();
bool tests_fixture_evil();
bool tests_fixture_throwing_nothrowing();
bool tests_fixture_literal_nonliteral();
bool tests_fixture_enums();
bool tests_fixture_nonclass_types();
bool tests_hostile_list_cardinalities();
bool tests_hostile_list_complete();

// VII.  build-time pins
bool tests_static_pin();
bool tests_build_time_hazards();

// VIII. concept layer  (C++20)
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

bool tests_trait_concept_agree();
bool tests_declare_subsumes_refines();
bool tests_declare_subsumes_implies_only();
bool tests_declare_subsumes_partial();
bool tests_declare_subsumes_shape();

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_TEST_TRAITS_TESTS_
