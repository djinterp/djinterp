// djinterp
#include "test_traits_tests.hpp"

// std
#include <memory>
#include <string>
#include <type_traits>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
tests_fixture_shapes
  empty, incomplete, abstract, final_type - the four structural fixtures.
  Each is in the zoo because it is a known way for a probe to be WRONG rather
  than merely false, and each check below is the property that makes it so.
  Tests the following:
  - `empty` has no members and no size, and every member probe reports false for
    it without diagnosing
  - `incomplete` is DECLARED AND NEVER DEFINED.  A probe must tolerate it, which
    means the probe may not do anything requiring a complete type - and
    completeness is exactly what sizeof requires, so the sizeof probe declining
    is what proves the fixture is genuinely incomplete rather than merely empty
  - `abstract` cannot be materialized, so any probe that constructs a value of
    its subject (rather than taking a reference) breaks on it
  - `final_type` cannot be derived from, which is the fixture for any trait that
    probes by inheriting from its subject
*/
bool
tests_fixture_shapes()
{
    bool ok = true;

    // empty
    D_TT_CHECK(std::is_empty<fixtures::empty>::value);
    D_TT_CHECK(std::is_class<fixtures::empty>::value);
    D_TT_CHECK(!is_detected<p_value_type, fixtures::empty>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::empty>::value);
    D_TT_CHECK(!is_detected<p_front, fixtures::empty>::value);

    // incomplete: no probe may require completeness, and sizeof does
    D_TT_CHECK(!is_detected<p_sizeof_ce, fixtures::incomplete>::value);
    D_TT_CHECK(is_detected<p_sizeof_ce, fixtures::empty>::value);
    D_TT_CHECK(!is_detected<p_value_type, fixtures::incomplete>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::incomplete>::value);
    D_TT_CHECK(!is_detected<p_default_expr, fixtures::incomplete>::value);
    D_TT_CHECK(std::is_class<fixtures::incomplete>::value);

    // abstract: cannot be materialized
    D_TT_CHECK(std::is_abstract<fixtures::abstract>::value);
    D_TT_CHECK(!std::is_constructible<fixtures::abstract>::value);
    D_TT_CHECK(!is_detected<p_default_expr, fixtures::abstract>::value);
    D_TT_CHECK(!is_detected<p_default_ce, fixtures::abstract>::value);

    // ...but a REFERENCE to it is fine, which is the distinction a probe that
    // uses declval<_Type&>() relies on
    D_TT_CHECK(std::is_polymorphic<fixtures::abstract>::value);

    // final: cannot be derived from
    D_TT_CHECK(std::is_final<fixtures::final_type>::value);
    D_TT_CHECK(!std::is_final<fixtures::empty>::value);
    D_TT_CHECK(std::is_empty<fixtures::final_type>::value);

    return ok;
}

/*
tests_fixture_private_members
  private_members - a `value_type` and a `size()` that are both PRIVATE.

  Access checking is part of substitution, so the failure lands in the immediate
  context and a correct probe reports false.  A probe that ignored access control
  would not compile.  This fixture is what proves a probe relies on that rather
  than on luck - and the only way to state the claim is to point a probe at a
  type that HAS the member and still must answer no.

  Tests the following:
  - both probes report false for it
  - the same probes report TRUE for a subject with the same members made public,
    so the false above is about ACCESS and not about absence
*/
bool
tests_fixture_private_members()
{
    bool ok = true;

    // it has them, and they are unreachable
    D_TT_CHECK(!is_detected<p_value_type, fixtures::private_members>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::private_members>::value);

    // ...and the very same probes find them when they are public.  Without this
    // line the checks above would be satisfied by a fixture that simply had no
    // members at all
    D_TT_CHECK(is_detected<p_value_type, sized>::value);
    D_TT_CHECK(is_detected<p_size, sized>::value);

    // the inline spelling agrees
    D_TT_CHECK(!is_valid<fixtures::private_members&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));
    D_TT_CHECK(is_valid<sized&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));

    return ok;
}

/*
tests_fixture_ambiguous_members
  ambiguous_members - inherits `value_type` from two bases, so the name is
  ambiguous.  Lookup fails in the immediate context; another substitution failure
  a probe has to survive rather than diagnose.

  Tests the following:
  - the probe reports false for the derived type
  - each base ALONE carries the member, and the probe finds it there - so the
    false above is about AMBIGUITY and not about absence
  - the two bases really do disagree about what value_type is, which is what
    makes the lookup ambiguous rather than merely repeated
*/
bool
tests_fixture_ambiguous_members()
{
    bool ok = true;

    D_TT_CHECK(!is_detected<p_value_type,
                            fixtures::ambiguous_members>::value);

    // each base alone is fine
    D_TT_CHECK(is_detected<p_value_type, fixtures::value_type_a>::value);
    D_TT_CHECK(is_detected<p_value_type, fixtures::value_type_b>::value);

    D_TT_CHECK((std::is_same<detected_t<p_value_type, fixtures::value_type_a>,
                             int>::value));
    D_TT_CHECK((std::is_same<detected_t<p_value_type, fixtures::value_type_b>,
                             char>::value));

    // and the derivation is real
    D_TT_CHECK((std::is_base_of<fixtures::value_type_a,
                                fixtures::ambiguous_members>::value));
    D_TT_CHECK((std::is_base_of<fixtures::value_type_b,
                                fixtures::ambiguous_members>::value));

    return ok;
}

/*
tests_fixture_greedy
  greedy - converts to ANYTHING, via a template conversion operator.

  It satisfies every convertibility-shaped probe and no expression-shaped one,
  which makes it the fixture that SEPARATES the two.  A trait that claims greedy
  models a contract is testing convertibility where it meant to test an
  expression - and that is a mistake nothing else in the zoo can catch, because
  every other fixture answers no to both kinds of probe.

  Tests the following:
  - it converts to a fundamental type, to a class type, and to a type it has
    never heard of
  - a CONVERSION-shaped probe therefore succeeds for it
  - a MEMBER-shaped probe does not - the conversion operator does not conjure a
    size()
  - so the two probes disagree about greedy, and about nothing else in the suite
*/
bool
tests_fixture_greedy()
{
    bool ok = true;

    using greedy = fixtures::greedy;

    // it converts to anything
    D_TT_CHECK((std::is_convertible<greedy, int>::value));
    D_TT_CHECK((std::is_convertible<greedy, bool>::value));
    D_TT_CHECK((std::is_convertible<greedy, std::size_t>::value));
    D_TT_CHECK((std::is_convertible<greedy, fixtures::empty>::value));
    D_TT_CHECK((std::is_convertible<greedy, sized>::value));

    // THE SEPARATION.  a conversion-shaped probe succeeds...
    D_TT_CHECK(is_detected<p_convertible_to_size, greedy>::value);

    // ...and a member-shaped one does not
    D_TT_CHECK(!is_detected<p_size, greedy>::value);
    D_TT_CHECK(!is_detected<p_value_type, greedy>::value);
    D_TT_CHECK(!is_detected<p_front, greedy>::value);

    // the two probes disagree about greedy - and agree about everything else
    D_TT_CHECK(is_detected<p_convertible_to_size, greedy>::value !=
               is_detected<p_size, greedy>::value);
    D_TT_CHECK(is_detected<p_convertible_to_size, plain>::value ==
               is_detected<p_size, plain>::value);

    // ...including `sized`, which has the member and no conversion
    D_TT_CHECK(!is_detected<p_convertible_to_size, sized>::value);
    D_TT_CHECK(is_detected<p_size, sized>::value);

    return ok;
}

/*
tests_fixture_evil
  evil - overloads unary `operator&` and `operator,`, the two operators a
  carelessly written probe routes through by accident.

  A probe that says `&_x` where it meant std::addressof(_x), or that separates
  sub-expressions with a bare comma, silently changes meaning here and NOWHERE
  ELSE.  That is what makes this fixture worth having: the bug it catches is
  invisible on every other type in the language.

  Tests the following:
  - `&evil` yields void*, not evil* - the operator has taken the address away
  - std::addressof yields evil*, which is what a probe should have used
  - a bare comma with an evil left operand is a CALL, not the built-in comma:
    the result type is `evil`, not the type of the right-hand operand
  - `void(...)` around the left operand defeats it - the built-in comma is
    restored.  This is the guard is_valid uses, and the one the header tells
    every probe author to spell in their trailing return type
  - the toolkit's own probes and is_valid are not fooled by any of it
*/
bool
tests_fixture_evil()
{
    bool ok = true;

    using evil = fixtures::evil;

    // operator& has taken the address away
    D_TT_CHECK((std::is_same<decltype(&std::declval<evil&>()),
                             void*>::value));
    D_TT_CHECK((!std::is_same<decltype(&std::declval<evil&>()),
                              evil*>::value));

    // ...and addressof gets it back
    D_TT_CHECK((std::is_same<
                    decltype(std::addressof(std::declval<evil&>())),
                    evil*>::value));

    // operator, hijacks a bare comma: the result is evil, not int
    D_TT_CHECK((std::is_same<decltype((std::declval<evil&>(), 0)),
                             evil>::value));
    D_TT_CHECK((!std::is_same<decltype((std::declval<evil&>(), 0)),
                              int>::value));

    // ...and void(...) restores the built-in comma
    D_TT_CHECK((std::is_same<decltype((void(std::declval<evil&>()), 0)),
                             int>::value));

    // for contrast, an ordinary type is not hijacked at all
    D_TT_CHECK((std::is_same<decltype((std::declval<plain&>(), 0)),
                             int>::value));
    D_TT_CHECK((std::is_same<decltype(&std::declval<plain&>()),
                             plain*>::value));

    // and the toolkit is not fooled
    D_TT_CHECK(!is_detected<p_size, evil>::value);
    D_TT_CHECK(!is_detected<p_value_type, evil>::value);
    D_TT_CHECK(!is_valid<evil&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));

    return ok;
}

/*
tests_fixture_throwing_nothrowing
  throwing and nothrowing - the negative and positive fixtures for every noexcept
  probe.  Same surface, opposite noexcept-ness, so a probe that reported
  well-formedness where it meant non-throwing would pass for one and fail for the
  other.

  THE FINDING.  `throwing` is documented as "default-constructs, copies, assigns
  and destroys, and NONE OF IT is noexcept".  Five of the six are right.  The
  destructor is not:

      ~throwing();

  has no exception specification - and since C++11 a destructor with no exception
  specification is IMPLICITLY noexcept(true).  So

      std::is_nothrow_destructible<fixtures::throwing>::value   ==   TRUE

  The one fixture in the zoo whose entire job is to be the negative case for a
  noexcept probe has a NON-THROWING destructor.  A suite testing a
  `noexcept(destroy)` probe against it would get a false positive from the very
  fixture that was supposed to catch the false positive - and would then ship,
  because everything passed.

  The fix is one token:

      ~throwing() noexcept(false);

  which is what `nothrowing` already does in the other direction (it spells
  D_NOEXCEPT explicitly, redundantly and correctly).  The checks below pin the
  behaviour AS IT STANDS; applying the fix flips exactly one of them, and it is
  labelled.

  Tests the following:
  - the five operations that ARE throwing on `throwing`, and non-throwing on
    `nothrowing`
  - the destructor, which is non-throwing on BOTH - the defect
  - both fixtures have the full surface, so the difference really is
    noexcept-ness and not availability
  - a destructor with no exception specification is implicitly noexcept, stated
    on a local subject so the mechanism is not taken on trust
*/
bool
tests_fixture_throwing_nothrowing()
{
    bool ok = true;

    using t = fixtures::throwing;
    using n = fixtures::nothrowing;

    // both have the full surface
    D_TT_CHECK(std::is_default_constructible<t>::value);
    D_TT_CHECK(std::is_copy_constructible<t>::value);
    D_TT_CHECK(std::is_move_constructible<t>::value);
    D_TT_CHECK(std::is_copy_assignable<t>::value);
    D_TT_CHECK(std::is_move_assignable<t>::value);
    D_TT_CHECK(std::is_destructible<t>::value);

    D_TT_CHECK(std::is_default_constructible<n>::value);
    D_TT_CHECK(std::is_copy_constructible<n>::value);
    D_TT_CHECK(std::is_move_constructible<n>::value);
    D_TT_CHECK(std::is_copy_assignable<n>::value);
    D_TT_CHECK(std::is_move_assignable<n>::value);
    D_TT_CHECK(std::is_destructible<n>::value);

    // the five that behave as documented
    D_TT_CHECK(!std::is_nothrow_default_constructible<t>::value);
    D_TT_CHECK(!std::is_nothrow_copy_constructible<t>::value);
    D_TT_CHECK(!std::is_nothrow_move_constructible<t>::value);
    D_TT_CHECK(!std::is_nothrow_copy_assignable<t>::value);
    D_TT_CHECK(!std::is_nothrow_move_assignable<t>::value);

    D_TT_CHECK(std::is_nothrow_default_constructible<n>::value);
    D_TT_CHECK(std::is_nothrow_copy_constructible<n>::value);
    D_TT_CHECK(std::is_nothrow_move_constructible<n>::value);
    D_TT_CHECK(std::is_nothrow_copy_assignable<n>::value);
    D_TT_CHECK(std::is_nothrow_move_assignable<n>::value);
    D_TT_CHECK(std::is_nothrow_destructible<n>::value);

    // THE DEFECT.  `throwing`'s destructor is noexcept, exactly like
    // `nothrowing`'s - so the two fixtures do NOT differ on this axis at all.
    // Add `noexcept(false)` to ~throwing() and the next line flips
    D_TT_CHECK(std::is_nothrow_destructible<t>::value);
    D_TT_CHECK(std::is_nothrow_destructible<t>::value ==
               std::is_nothrow_destructible<n>::value);

    // ...and the mechanism, on a local subject, so it is not taken on trust: a
    // destructor with NO exception specification is implicitly noexcept(true),
    // and only an explicit noexcept(false) makes it throwing
    struct declared_dtor
    {
        ~declared_dtor();
    };

    struct throwing_dtor
    {
        ~throwing_dtor() noexcept(false);
    };

    D_TT_CHECK(std::is_nothrow_destructible<declared_dtor>::value);
    D_TT_CHECK(!std::is_nothrow_destructible<throwing_dtor>::value);

    return ok;
}

/*
tests_fixture_literal_nonliteral
  literal and nonliteral - the positive and negative fixtures for
  D_TEST_CONSTEXPR_PROBE.
  Tests the following:
  - literal has a constexpr constructor, so an expression that builds one IS a
    constant expression, and the constexpr probe detects it
  - nonliteral's constructor and destructor are NOT constexpr, so no expression
    that builds one ever is - and the probe declines
  - but nonliteral is still CONSTRUCTIBLE: an EXPR probe over the same expression
    succeeds.  The pair is what separates "ill-formed" from "well-formed but not
    constant", which is the caveat the constexpr declarator carries
*/
bool
tests_fixture_literal_nonliteral()
{
    bool ok = true;

    // literal: constexpr-constructible
    D_TT_CHECK(is_detected<p_literal_expr, fixtures::literal>::value);
    D_TT_CHECK(is_detected<p_literal_ce, fixtures::literal>::value);
    D_TT_CHECK(std::is_trivially_destructible<fixtures::literal>::value);

    // ...and really is one, at compile time
    D_CONSTEXPR fixtures::literal lit{7};

    D_TT_CHECK(lit.value == 7);

    // nonliteral: constructible, and never constant
    D_TT_CHECK(std::is_default_constructible<fixtures::nonliteral>::value);
    D_TT_CHECK(is_detected<p_default_expr, fixtures::nonliteral>::value);
    D_TT_CHECK(!is_detected<p_default_ce, fixtures::nonliteral>::value);
    D_TT_CHECK(!std::is_trivially_destructible<fixtures::nonliteral>::value);

    // the two probes agree about literal and disagree about nonliteral, which is
    // exactly the caveat, stated
    D_TT_CHECK(is_detected<p_default_expr, fixtures::empty>::value ==
               is_detected<p_default_ce, fixtures::empty>::value);
    D_TT_CHECK(is_detected<p_default_expr, fixtures::nonliteral>::value !=
               is_detected<p_default_ce, fixtures::nonliteral>::value);

    return ok;
}

/*
tests_fixture_enums
  plain_enum and scoped_enum.
  Tests the following:
  - both are enumerations
  - the UNSCOPED one converts implicitly to its underlying type, so it slips
    through an integral-shaped probe that was meant to reject it
  - the SCOPED one does not, so it slips through nothing

  That difference is the whole reason both are in the zoo: a trait that means
  "is this an integer" and is written as "is this convertible to an integer"
  accepts the first and rejects the second, and only having both in the battery
  makes the mistake visible.
*/
bool
tests_fixture_enums()
{
    bool ok = true;

    D_TT_CHECK(std::is_enum<fixtures::plain_enum>::value);
    D_TT_CHECK(std::is_enum<fixtures::scoped_enum>::value);

    // THE DIFFERENCE
    D_TT_CHECK((std::is_convertible<fixtures::plain_enum, int>::value));
    D_TT_CHECK((!std::is_convertible<fixtures::scoped_enum, int>::value));

    // neither is an integral type, whatever it converts to - which is what a
    // trait meaning "is this an integer" should have asked
    D_TT_CHECK(!std::is_integral<fixtures::plain_enum>::value);
    D_TT_CHECK(!std::is_integral<fixtures::scoped_enum>::value);

    // and neither has members
    D_TT_CHECK(!is_detected<p_value_type, fixtures::plain_enum>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::scoped_enum>::value);

    return ok;
}

/*
tests_fixture_nonclass_types
  The twelve non-class aliases.  Each is named so a suite can drop it into a pack
  without having to remember which of `int[]`, `int(&)(int)` and `int literal::*`
  needs parentheses - and each is exactly the type it claims.
  Tests the following:
  - every alias names the type its name says it names
  - the shapes that turn a careless `_Type::value_type` into a hard error - void,
    functions, arrays, references, member pointers - are all present
*/
bool
tests_fixture_nonclass_types()
{
    bool ok = true;

    D_TT_CHECK((std::is_same<fixtures::void_type, void>::value));
    D_TT_CHECK((std::is_same<fixtures::const_void_type, const void>::value));
    D_TT_CHECK(std::is_void<fixtures::void_type>::value);
    D_TT_CHECK(std::is_void<fixtures::const_void_type>::value);

    D_TT_CHECK((std::is_same<fixtures::function_type, int(int)>::value));
    D_TT_CHECK(std::is_function<fixtures::function_type>::value);
    D_TT_CHECK((std::is_same<fixtures::function_ptr_type,
                             int (*)(int)>::value));
    D_TT_CHECK(std::is_pointer<fixtures::function_ptr_type>::value);
    D_TT_CHECK((std::is_same<fixtures::function_ref_type,
                             int (&)(int)>::value));
    D_TT_CHECK(std::is_lvalue_reference<fixtures::function_ref_type>::value);

    D_TT_CHECK(std::is_array<fixtures::array_type>::value);
    D_TT_CHECK((std::extent<fixtures::array_type>::value == 3));
    D_TT_CHECK(std::is_array<fixtures::unbounded_array_type>::value);
    D_TT_CHECK((std::extent<fixtures::unbounded_array_type>::value == 0));

    D_TT_CHECK((std::is_same<fixtures::lvalue_ref_type, int&>::value));
    D_TT_CHECK((std::is_same<fixtures::rvalue_ref_type, int&&>::value));
    D_TT_CHECK(std::is_lvalue_reference<fixtures::lvalue_ref_type>::value);
    D_TT_CHECK(std::is_rvalue_reference<fixtures::rvalue_ref_type>::value);

    D_TT_CHECK(std::is_member_object_pointer<
                   fixtures::member_object_ptr_type>::value);
    D_TT_CHECK(std::is_member_function_pointer<
                   fixtures::member_fn_ptr_type>::value);

    D_TT_CHECK((std::is_same<fixtures::nullptr_type, std::nullptr_t>::value));
    D_TT_CHECK(std::is_null_pointer<fixtures::nullptr_type>::value);

    // ...and every one of them survives a member probe
    D_TT_CHECK((holds_for_none<has_member_value_type,
                               D_TEST_HOSTILE_NONCLASS_TYPES>::value));

    return ok;
}

/*
tests_hostile_list_cardinalities
  The five list macros, and what is IN them.

  A macro that silently loses a fixture makes every battery in the framework
  weaker without failing anything - the suites keep passing, over a smaller zoo,
  and nobody notices.  count_holds over always_true is the one thing that can
  say so: it counts the pack, and the pack is the list.

  Tests the following:
  - D_TEST_HOSTILE_CLASS_TYPES has 14 members
  - D_TEST_HOSTILE_NONCLASS_TYPES has 12
  - D_TEST_HOSTILE_TYPES is exactly the two of them, 26
  - D_TEST_HOSTILE_CLASS_TYPES_COMPLETE has 13 - the class list minus one
  - D_TEST_HOSTILE_TYPES_COMPLETE has 25
  - each list drops straight into a pack position, which is the only reason they
    are macros and not type lists
*/
bool
tests_hostile_list_cardinalities()
{
    bool ok = true;

    D_TT_CHECK((count_holds<always_true,
                            D_TEST_HOSTILE_CLASS_TYPES>::value == 14));
    D_TT_CHECK((count_holds<always_true,
                            D_TEST_HOSTILE_NONCLASS_TYPES>::value == 12));
    D_TT_CHECK((count_holds<always_true,
                            D_TEST_HOSTILE_TYPES>::value == 26));
    D_TT_CHECK((count_holds<always_true,
                            D_TEST_HOSTILE_CLASS_TYPES_COMPLETE>::value == 13));
    D_TT_CHECK((count_holds<always_true,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value == 25));

    // the full list really is the two halves
    D_TT_CHECK((count_holds<always_true, D_TEST_HOSTILE_TYPES>::value ==
                (count_holds<always_true,
                             D_TEST_HOSTILE_CLASS_TYPES>::value +
                 count_holds<always_true,
                             D_TEST_HOSTILE_NONCLASS_TYPES>::value)));

    D_TT_CHECK((count_holds<always_true,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value ==
                (count_holds<always_true,
                             D_TEST_HOSTILE_CLASS_TYPES_COMPLETE>::value +
                 count_holds<always_true,
                             D_TEST_HOSTILE_NONCLASS_TYPES>::value)));

    // and the lists are usable through the quantifiers, which is the point
    D_TT_CHECK((holds_for_all<always_true, D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((holds_for_none<always_false, D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((holds_for_any<is_int, int, D_TEST_HOSTILE_TYPES>::value));

    return ok;
}

/*
tests_hostile_list_complete
  The _COMPLETE lists are the full lists MINUS the incomplete fixture - and
  nothing else.

  They exist because some traits legitimately REQUIRE a complete type and cannot
  be run against `incomplete` at all: not "report false", but "fail the build".
  The std library mandates completeness for most of the <type_traits> property
  traits, so any probe routing through one inherits the requirement - which is
  exactly what happened to constexpr_container_traits.hpp, whose is_literal_type
  goes through std::is_trivially_destructible.

  Tests the following:
  - the incomplete fixture is IN the full lists and NOT in the _COMPLETE ones -
    checked by a trait that recognizes it by name, so the difference is
    attributable rather than merely arithmetic
  - the two lists otherwise agree: everything else present in one is present in
    the other
  - a completeness-requiring standard trait really does run over the _COMPLETE
    list.  That it does NOT run over the full one is the other half, and it is a
    build failure; see tests_build_time_hazards
*/
bool
tests_hostile_list_complete()
{
    bool ok = true;

    // the incomplete fixture is in the full lists...
    D_TT_CHECK((count_holds<is_the_incomplete_fixture,
                            D_TEST_HOSTILE_CLASS_TYPES>::value == 1));
    D_TT_CHECK((count_holds<is_the_incomplete_fixture,
                            D_TEST_HOSTILE_TYPES>::value == 1));

    // ...and NOT in the _COMPLETE ones
    D_TT_CHECK((count_holds<is_the_incomplete_fixture,
                            D_TEST_HOSTILE_CLASS_TYPES_COMPLETE>::value == 0));
    D_TT_CHECK((count_holds<is_the_incomplete_fixture,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value == 0));

    // it is the ONLY difference: one fewer type, and that type is this one
    D_TT_CHECK((count_holds<always_true, D_TEST_HOSTILE_TYPES>::value -
                count_holds<always_true,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value) == 1);

    // and a completeness-requiring trait genuinely runs over the shorter list.
    // std::is_trivially_destructible MANDATES a complete type, cv void, or an
    // array of unknown bound - and every member of the _COMPLETE list is one of
    // those.  Over the FULL list it does not compile at all
    D_TT_CHECK((count_holds<std::is_trivially_destructible,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value > 0));
    D_TT_CHECK((count_holds<std::is_trivially_destructible,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value < 25));

    // the class fixtures it can still be asked about
    D_TT_CHECK(std::is_trivially_destructible<fixtures::empty>::value);
    D_TT_CHECK(!std::is_trivially_destructible<fixtures::throwing>::value);
    D_TT_CHECK(!std::is_trivially_destructible<fixtures::nonliteral>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
