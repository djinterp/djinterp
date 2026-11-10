/*******************************************************************************
* djinterp [test]                                    structural_traits_tests.hpp
*
*   Unit-test declarations for core/functional/structural_traits.hpp -- the
* structural detection traits behind the parse/scanner/pattern layers and the
* functional dataflow roles (producer / consumer / view).  One declaration group
* per section of the module under test:
*
*     structural_traits_tests_members.cpp   -- I.   has_match_result_type,
*                                                    has_find_method
*     structural_traits_tests_arity.cpp     -- II.  the arity trilogy
*                                                    (nullary / unary / binary)
*     structural_traits_tests_optional.cpp  -- III. DProducesOptionalLike,
*                                                    is_unfold_step
*     structural_traits_tests_aliases.cpp   -- IV.  the _v shorthands (C++14+)
*     structural_traits_tests_concepts.cpp  -- V.   BinaryCallable, Reducer,
*                                                    Transducer, UnfoldStep
*
*   FIXTURES.  Every trait here is a SFINAE probe of one exact expression, so the
* fixtures are chosen to pin each probe from every side it can be got at.
*
*   The arity trilogy probes `declval<T&>()` -- a MUTABLE lvalue.  Four fixtures
* determine that expression uniquely and admit no other: DConstStep (a const
* operator(), accepted), DMutStep (a NON-const one, accepted -- so the probe is
* not `const T&`), DLrefStep (an &-qualified one, accepted -- so it is an lvalue),
* and DRrefStep (an &&-qualified one, REJECTED -- so it is not `T&&`).  The
* mutable half is load-bearing, not incidental: a pull source advances on every
* call and a tallying reducer accumulates, so both must have a non-const
* operator().  Note this is the OPPOSITE of functional_traits' is_callable, which
* probes a const lvalue -- and note has_find_method, in this same header, probes
* `declval<const T&>()` too.
*
*   has_find_method converts the result with static_cast<bool>, an EXPLICIT
* conversion, so DPatExplicit (whose find returns a type with an explicit
* operator bool) is accepted where an implicit-convertibility check would refuse
* it.  The optional-like traits require BOTH halves -- bool-testable AND
* dereferenceable -- so DSrcInt (testable, not dereferenceable) and DSrcDeref
* (dereferenceable, not testable) each fail on exactly one.
*
*   Fixtures are named functors rather than lambdas, and maybe<T> is a plain
* aggregate, so the suite builds on every floor the module claims: C++11 through
* C++20.  std::optional appears only under a C++17 gate.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/structural_traits_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.12
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (patterns, callables by arity and qualifier, sources, steps)
      ----------------------------------------------------------------------

I.    MEMBER DETECTION   (has_match_result_type, has_find_method)
      -----------------------------------------------------------

II.   ARITY TRILOGY      (is_nullary_callable, is_unary_callable,
      -----------------------------------------------------------

      is_binary_callable)

III.  OPTIONAL-LIKE      (DProducesOptionalLike, is_unfold_step)
      -----------------------------------------------------------

IV.   CONVENIENCE ALIASES(the _v shorthands)
      --------------------------------------

V.    PROTOCOL CONCEPTS  (BinaryCallable, Reducer, Transducer, UnfoldStep)
      --------------------------------------------------------------------
*/


#ifndef DJINTERP_TEST_STRUCTURAL_TRAITS_TESTS_
#define DJINTERP_TEST_STRUCTURAL_TRAITS_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
// djinterp (module under test)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/structural_traits.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// -- the optional-like carrier -------------------------------------------

// maybe
//   fixture carrier: bool-testable and dereferenceable -- the shape a pull
// source yields. A plain aggregate (no NSDMI, no user constructor) so it
// brace-initialises on every floor, including C++11.
template<typename _Type>
struct maybe
{
    _Type value;
    bool  ok;

    explicit operator bool() const { return ok; }
    const _Type& operator*() const { return value; }
};

// DDerefOnly: dereferenceable but NOT bool-testable -- fails exactly one half.
struct DDerefOnly
{
    int operator*() const { return 1; }
};

// DExplicitBool: convertible to bool only EXPLICITLY (static_cast reaches it,
// an implicit-conversion check would not).
struct DExplicitBool
{
    explicit operator bool() const { return true; }
};


// -- patterns (the scanner protocol) --------------------------------------

// DPatScan
//   fixture pattern: the canonical searchable shape --
// `bool find(const In&, size_t&, Result&) const` -- and it really scans, so the
// detected shape can be driven, not merely recognised. Finds each 'x' in the
// input, writing the position into the result and advancing pos.
struct DPatScan
{
    using match_result_type = int;

    bool find(
        const std::string& _in,
        std::size_t&       _pos,
        int&               _result
    ) const
    {
        const std::string::size_type p = _in.find('x', _pos);
        if (p == std::string::npos) { return false; }
        _pos    = static_cast<std::size_t>(p) + 1u;
        _result = static_cast<int>(p);
        return true;
    }
};

// DPatNone:      no find at all.
struct DPatNone
{
};

// DPatNonconst:  find is NOT const -- rejected, because has_find_method probes a
//                const lvalue (unlike the arity traits in this same header).
struct DPatNonconst
{
    bool find(const std::string&, std::size_t&, int&) { return true; }
};

// DPatVoid:      find yields void -- not static_cast-able to bool.
struct DPatVoid
{
    void find(const std::string&, std::size_t&, int&) const {}
};

// DPatString:    find yields a type with no conversion to bool.
struct DPatString
{
    std::string find(
        const std::string&,
        std::size_t&,
        int&
    ) const
    {
        return std::string();
    }
};

// DPatExplicit:  find yields an EXPLICITLY bool-convertible type -- accepted,
//                since the probe uses static_cast<bool>.
struct DPatExplicit
{
    DExplicitBool find(
        const std::string&,
        std::size_t&,
        int&
    ) const
    {
        return DExplicitBool();
    }
};

// DPatInt:       find yields int -- converts to bool, so accepted.
struct DPatInt
{
    int find(const std::string&, std::size_t&, int&) const { return 1; }
};

// DPatByvalue:   find takes size_t by value and the result by const ref -- still
//                callable with the probe's lvalues, so accepted.
struct DPatByvalue
{
    bool find(const std::string&, std::size_t, const int&) const { return true; }
};

// DPatArity:     find takes two arguments.
struct DPatArity
{
    bool find(const std::string&, std::size_t&) const { return true; }
};

// DPatWrongIn:  find takes the wrong input type.
struct DPatWrongIn
{
    bool find(int, std::size_t&, int&) const { return true; }
};

// DPatData:      `find` is a data member, not a member function.
struct DPatData
{
    int find;
};

// DMrtOnly / DMrtData: the nested-type detector, from both sides.
struct DMrtOnly
{
    using match_result_type = std::string;
};

struct DMrtData
{
    int match_result_type;      // a data member, NOT a nested type
};


// -- callables, by arity and by qualifier ---------------------------------

// DConstStep:  a const operator()          -- accepted (callable on an lvalue).
struct DConstStep
{
    int operator()(int _x) const { return _x * 2; }
};

// DMutStep:    a NON-const operator()      -- accepted; the probe is not const.
struct DMutStep
{
    int calls;

    DMutStep() : calls(0) {}

    int operator()(int _x) { ++calls; return _x * 2; }
};

// DLrefStep:   an &-qualified operator()   -- accepted; the probe is an lvalue.
struct DLrefStep
{
    int operator()(int _x) & { return _x; }
};

// DRrefStep:   an &&-qualified operator()  -- REJECTED; the probe is not T&&.
struct DRrefStep
{
    int operator()(int _x) && { return _x; }
};

// in_place:    takes int& -- undetectable, because clean_t strips the reference
//              off _Arg and the probe then offers an rvalue.
struct in_place
{
    void operator()(int& _r) const { _r *= 2; }
};

// DGenericStep: a templated operator().
struct DGenericStep
{
    template<typename _Type>
    _Type operator()(_Type _x) const { return _x; }
};

// DNilConst / DNilMut / DNilLref / DNilRref: the same quartet, at arity zero.
struct DNilConst
{
    int operator()() const { return 7; }
};

struct DNilMut
{
    int calls;

    DNilMut() : calls(0) {}

    int operator()() { return ++calls; }
};

struct DNilLref
{
    int operator()() & { return 1; }
};

struct DNilRref
{
    int operator()() && { return 1; }
};

// not_callable: no operator() at all.
struct not_callable
{
    int x;
};


// -- reducers and transducers ---------------------------------------------

// sum_reducer:  a pure binary step (acc, x) -> acc.
struct sum_reducer
{
    int operator()(int _acc, int _x) const { return _acc + _x; }
};

// DTallyReducer: a STATEFUL binary step -- non-const, as an accumulating reducer
//                must be.
struct DTallyReducer
{
    int seen;

    DTallyReducer() : seen(0) {}

    int operator()(int _acc, int _x) { ++seen; return _acc + _x; }
};

// void_reducer:  binary, but yields void -- still satisfies Reducer, which
//                checks arity only, not the (acc, x) -> acc result.
struct void_reducer
{
    void operator()(int, int) const {}
};

// DDoubledReducer: the reducer a transducer produces -- doubles each element
//                  before handing it to the wrapped reducer.
struct DDoubledReducer
{
    sum_reducer rf;

    int operator()(int _acc, int _x) const { return rf(_acc, _x * 2); }
};

// DDoublingXform:  a transducer -- reducer -> reducer.
struct DDoublingXform
{
    DDoubledReducer operator()(
        sum_reducer _rf
    ) const
    {
        DDoubledReducer r = { _rf };
        return r;
    }
};


// -- sources (nullary) and unfold steps (unary, state-threaded) -----------

// DSrcPull:  the canonical pull source -- stateful, so operator() is non-const;
//            yields three values, then exhausts.
struct DSrcPull
{
    int n;

    DSrcPull() : n(0) {}

    maybe<int> operator()()
    {
        if (n < 3)
        {
            maybe<int> m = { n * 10, true };
            ++n;
            return m;
        }
        maybe<int> m = { 0, false };
        return m;
    }
};

// DSrcConst: a const nullary source (also accepted -- const is callable on an
//            lvalue).
struct DSrcConst
{
    maybe<int> operator()() const
    {
        maybe<int> m = { 1, true };
        return m;
    }
};

// src_ptr:   a pointer-yielding source -- pointers are bool-testable and
//            dereferenceable, so they are optional-like.
struct src_ptr
{
    int v;

    src_ptr() : v(5) {}

    int* operator()() { return &v; }
};

// DSrcInt:   yields int -- bool-testable, NOT dereferenceable.
struct DSrcInt
{
    int operator()() const { return 1; }
};

// DSrcDeref: yields DDerefOnly -- dereferenceable, NOT bool-testable.
struct DSrcDeref
{
    DDerefOnly operator()() const { return DDerefOnly(); }
};

// DSrcVoid:  yields void -- neither.
struct DSrcVoid
{
    void operator()() const {}
};

// step_count: an unfold step -- State -> maybe<next State>; terminates at 3.
struct step_count
{
    maybe<int> operator()(
        int _s
    ) const
    {
        if (_s < 3)
        {
            maybe<int> m = { _s + 1, true };
            return m;
        }
        maybe<int> m = { 0, false };
        return m;
    }
};

// DStepMut:   a stateful unfold step -- non-const operator().
struct DStepMut
{
    int calls;

    DStepMut() : calls(0) {}

    maybe<int> operator()(
        int _s
    )
    {
        ++calls;
        maybe<int> m = { _s + 1, _s < 2 };
        return m;
    }
};

// DStepPlain: unary, but yields a plain value -- not optional-like.
struct DStepPlain
{
    int operator()(int _s) const { return _s; }
};

// DStepDeref: unary, yields DDerefOnly -- dereferenceable, not bool-testable.
struct DStepDeref
{
    DDerefOnly operator()(int) const { return DDerefOnly(); }
};


///////////////////////////////////////////////////////////////////////////////
///             CONCEPT-FACING HELPERS  (C++20)                             ///
///////////////////////////////////////////////////////////////////////////////
//   The protocol faces put to work: a reduction driven by a Reducer, an unfold
// driven by an UnfoldStep, and constrained/fallback overload pairs proving the
// concepts GATE resolution rather than merely evaluating to a bool.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// reduce_with: runs any Reducer over a range -- the `step` half of the
// step/driver split.
template<typename _Fn>
    requires Reducer<_Fn, int, int>
int reduce_with(
    const int* _first,
    const int* _last,
    int        _acc,
    _Fn        _f
)
{
    for (const int* it = _first; it != _last; ++it)
    {
        _acc = _f(_acc, *it);
    }
    return _acc;
}

// unfold_count: pulls an UnfoldStep until it yields nothing, threading state.
template<typename _Step>
    requires UnfoldStep<_Step, int>
int unfold_count(
    _Step _s,
    int   _state
)
{
    int n = 0;
    for (int guard = 0; guard < 100; ++guard)
    {
        auto r = _s(_state);
        if (!static_cast<bool>(r)) { break; }
        _state = *r;
        ++n;
    }
    return n;
}

// apply_xform: applies a Transducer to a reducer, yielding a reducer.
template<typename _Xf,
         typename _Rf>
    requires Transducer<_Xf, _Rf>
auto apply_xform(
    _Xf _xf,
    _Rf _rf
)
-> decltype(_xf(_rf))
{
    return _xf(_rf);
}

// constrained overload + unconstrained fallback, for each face.
template<typename _Fn>
    requires Reducer<_Fn, int, int>
int which_reducer(_Fn) { return 1; }

template<typename _Type>
int which_reducer(_Type) { return 0; }

template<typename _Step>
    requires UnfoldStep<_Step, int>
int which_unfold(_Step) { return 1; }

template<typename _Type>
int which_unfold(_Type) { return 0; }

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    MEMBER DETECTION                                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_has_match_result_type_positive();
bool tests_has_match_result_type_negative();
bool tests_has_match_result_type_cvref();
bool tests_has_find_method_positive();
bool tests_has_find_method_scan_protocol();
bool tests_has_find_method_negative();
bool tests_has_find_method_return_conversion();
bool tests_has_find_method_const_lvalue();
bool tests_has_find_method_parameter_binding();
bool tests_has_find_method_cvref();


///////////////////////////////////////////////////////////////////////////////
///             II.   ARITY TRILOGY                                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_nullary_callable_positive();
bool tests_is_nullary_callable_negative();
bool tests_is_unary_callable_positive();
bool tests_is_unary_callable_negative();
bool tests_is_binary_callable_positive();
bool tests_is_binary_callable_negative();
bool tests_arity_mutable_lvalue_contract();
bool tests_arity_argument_decay();
bool tests_arity_type_decay();
bool tests_arity_trilogy_exclusive();


///////////////////////////////////////////////////////////////////////////////
///             III.  OPTIONAL-LIKE                                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_produces_optional_like_positive();
bool tests_produces_optional_like_requires_bool();
bool tests_produces_optional_like_requires_deref();
bool tests_produces_optional_like_requires_nullary();
bool tests_produces_optional_like_pull_protocol();
bool tests_is_unfold_step_positive();
bool tests_is_unfold_step_negative();
bool tests_is_unfold_step_requires_both();
bool tests_is_unfold_step_unfold_protocol();
bool tests_optional_like_nullary_vs_unary();


///////////////////////////////////////////////////////////////////////////////
///             IV.   CONVENIENCE ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////

bool tests_has_match_result_type_v_agrees();
bool tests_has_find_method_v_agrees();
bool tests_arity_v_agree();
bool tests_optional_like_v_agree();
bool tests_aliases_are_constant_expressions();
bool tests_aliases_gating();


///////////////////////////////////////////////////////////////////////////////
///             V.    PROTOCOL CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

bool tests_concept_binary_callable_mirrors();
bool tests_concept_reducer();
bool tests_concept_reducer_ignores_result();
bool tests_concept_transducer();
bool tests_concept_unfold_step();
bool tests_concept_drives_the_protocols();
bool tests_concept_overload_gating();
bool tests_concepts_gating();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// structural_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
structural_traits_spec()
{
    return dt::module_spec{
        "structural_traits",
        "has_match_result_type, has_find_method (the pattern protocol)",
        {
            dt::block_spec{
                "members",
                "Members",
                {
                    { "tests_has_match_result_type_positive",
                      "A type exposing the pattern protocol's extraction-face "
                      "result is detected",
                      &tests_has_match_result_type_positive },
                    { "tests_has_match_result_type_negative",
                      "The detector requires a nested TYPE -- a data member "
                      "of the same name is not one",
                      &tests_has_match_result_type_negative },
                    { "tests_has_match_result_type_cvref",
                      "The generated detector strips cv-qualifiers and "
                      "references before looking",
                      &tests_has_match_result_type_cvref },
                    { "tests_has_find_method_positive",
                      "The searchable-pattern shape -- find(const In&, "
                      "size_t&, Result&) -> bool -- is detected, so "
                      "pattern_scanner can static_assert the requirement "
                      "instead of failing deep inside an instantiation",
                      &tests_has_find_method_positive },
                    { "tests_has_find_method_scan_protocol",
                      "The detected shape really is the \"scan for the next "
                      "occurrence\" protocol the pattern scanner pulls on "
                      "repeatedly: find writes the next position and the "
                      "captures, and returns false once no further occurrence "
                      "exists",
                      &tests_has_find_method_scan_protocol },
                    { "tests_has_find_method_negative",
                      "Types that do not carry the shape are rejected, cleanly",
                      &tests_has_find_method_negative },
                    { "tests_has_find_method_return_conversion",
                      "The result is taken through static_cast<bool> -- an "
                      "EXPLICIT conversion",
                      &tests_has_find_method_return_conversion },
                    { "tests_has_find_method_const_lvalue",
                      "has_find_method probes `declval<const "
                      "_Type&>().find(...)` -- a CONST lvalue -- so a "
                      "non-const find is not detected",
                      &tests_has_find_method_const_lvalue },
                    { "tests_has_find_method_parameter_binding",
                      "The probe passes lvalues -- a const Input&, a size_t&, "
                      "and a Result& -- so any find whose parameters bind to "
                      "those lvalues qualifies",
                      &tests_has_find_method_parameter_binding },
                    { "tests_has_find_method_cvref",
                      "clean_t is applied to all three parameters, so cv-ref "
                      "spellings of the pattern, the input, and the result "
                      "all resolve to the same query",
                      &tests_has_find_method_cvref },
                }
            },
            dt::block_spec{
                "arity",
                "Arity",
                {
                    { "tests_is_nullary_callable_positive",
                      "A nullary callable -- the \"next\" step that drives a "
                      "pull-based source -- is detected",
                      &tests_is_nullary_callable_positive },
                    { "tests_is_nullary_callable_negative",
                      "Rejected cleanly (SFINAE, not a hard error)",
                      &tests_is_nullary_callable_negative },
                    { "tests_is_unary_callable_positive",
                      "A unary callable -- the transform and predicate shape "
                      "-- is detected",
                      &tests_is_unary_callable_positive },
                    { "tests_is_unary_callable_negative",
                      "is unary callable negative",
                      &tests_is_unary_callable_negative },
                    { "tests_is_binary_callable_positive",
                      "A binary callable -- the reducer step (acc, x) -> acc "
                      "-- is detected",
                      &tests_is_binary_callable_positive },
                    { "tests_is_binary_callable_negative",
                      "is binary callable negative",
                      &tests_is_binary_callable_negative },
                    { "tests_arity_mutable_lvalue_contract",
                      "The defining contract of all three: the probe is "
                      "declval<_Type&>() -- a MUTABLE LVALUE",
                      &tests_arity_mutable_lvalue_contract },
                    { "tests_arity_argument_decay",
                      "clean_t is applied to _Arg, so every cv-ref spelling "
                      "of an argument collapses to the same query -- and the "
                      "probe then offers an RVALUE",
                      &tests_arity_argument_decay },
                    { "tests_arity_type_decay",
                      "clean_t is also applied to _Type, so cv-ref spellings "
                      "of the CALLABLE collapse too",
                      &tests_arity_type_decay },
                    { "tests_arity_trilogy_exclusive",
                      "The three traits partition by arity: each accepts its "
                      "own and refuses the others, so the source / transform "
                      "/ reducer roles cannot be confused",
                      &tests_arity_trilogy_exclusive },
                }
            },
            dt::block_spec{
                "optional",
                "Optional",
                {
                    { "tests_produces_optional_like_positive",
                      "A nullary callable whose result is both bool-testable "
                      "and dereferenceable is a well-formed pull source",
                      &tests_produces_optional_like_positive },
                    { "tests_produces_optional_like_requires_bool",
                      "BOTH halves are required",
                      &tests_produces_optional_like_requires_bool },
                    { "tests_produces_optional_like_requires_deref",
                      "The other half",
                      &tests_produces_optional_like_requires_deref },
                    { "tests_produces_optional_like_requires_nullary",
                      "The source step is NULLARY -- a unary step is a "
                      "different role (that is is_unfold_step's job)",
                      &tests_produces_optional_like_requires_nullary },
                    { "tests_produces_optional_like_pull_protocol",
                      "The detected shape really is the pull protocol: each "
                      "call advances the source and answers \"more values?\", "
                      "and the source finally exhausts",
                      &tests_produces_optional_like_pull_protocol },
                    { "tests_is_unfold_step_positive",
                      "The unary, state-threaded analog: Step(state) -> "
                      "maybe<next>",
                      &tests_is_unfold_step_positive },
                    { "tests_is_unfold_step_negative",
                      "is unfold step negative",
                      &tests_is_unfold_step_negative },
                    { "tests_is_unfold_step_requires_both",
                      "As with DProducesOptionalLike, both halves are "
                      "required of the result",
                      &tests_is_unfold_step_requires_both },
                    { "tests_is_unfold_step_unfold_protocol",
                      "The detected shape is a working unfold: thread the "
                      "state, take a value while the step yields one, stop "
                      "when it does not",
                      &tests_is_unfold_step_unfold_protocol },
                    { "tests_optional_like_nullary_vs_unary",
                      "The two traits are the nullary and the unary halves of "
                      "one idea, and they do not overlap: a source is not an "
                      "unfold step and an unfold step is not a source",
                      &tests_optional_like_nullary_vs_unary },
                }
            },
            dt::block_spec{
                "aliases",
                "Aliases",
                {
                    { "tests_has_match_result_type_v_agrees",
                      "The member-type detector's _v companion (emitted by "
                      "the trait macro) is exactly the trait's value",
                      &tests_has_match_result_type_v_agrees },
                    { "tests_has_find_method_v_agrees",
                      "has_find_method_v is the trait's value (it re-applies "
                      "clean_t, which is idempotent, so the two cannot drift)",
                      &tests_has_find_method_v_agrees },
                    { "tests_arity_v_agree",
                      "The trilogy's three shorthands agree with their traits",
                      &tests_arity_v_agree },
                    { "tests_optional_like_v_agree",
                      "The two source-protocol shorthands agree with their "
                      "traits",
                      &tests_optional_like_v_agree },
                    { "tests_aliases_are_constant_expressions",
                      "The shorthands are constexpr bools, usable wherever a "
                      "constant is required",
                      &tests_aliases_are_constant_expressions },
                    { "tests_aliases_gating",
                      "The shorthands exist exactly where the standard allows "
                      "them: gated on variable templates (C++14+), with the "
                      "::value forms as the always-present floor -- the "
                      "header degrades to C++11",
                      &tests_aliases_gating },
                }
            },
            dt::block_spec{
                "concepts",
                "Concepts",
                {
                    { "tests_concept_binary_callable_mirrors",
                      "BinaryCallable is the concept face of "
                      "is_binary_callable",
                      &tests_concept_binary_callable_mirrors },
                    { "tests_concept_reducer",
                      "Reducer is the reduction step -- the pure `step` half "
                      "of the step/driver split, defined over BinaryCallable",
                      &tests_concept_reducer },
                    { "tests_concept_reducer_ignores_result",
                      "Worth pinning honestly: Reducer is defined as "
                      "BinaryCallable, so it constrains ARITY ONLY -- it does "
                      "not check that the result is the accumulator type, "
                      "though the documented shape is (acc, x) -> acc",
                      &tests_concept_reducer_ignores_result },
                    { "tests_concept_transducer",
                      "Transducer is a reducer-to-reducer transformer: a "
                      "unary callable that, given a reducer, yields a "
                      "transformed one",
                      &tests_concept_transducer },
                    { "tests_concept_unfold_step",
                      "UnfoldStep is the concept face of is_unfold_step -- "
                      "the pull-based source step State -> maybe<(value, "
                      "state)>",
                      &tests_concept_unfold_step },
                    { "tests_concept_drives_the_protocols",
                      "The faces put to work: a Reducer-constrained driver "
                      "runs any reduction step, and an UnfoldStep-constrained "
                      "driver pulls a source to exhaustion",
                      &tests_concept_drives_the_protocols },
                    { "tests_concept_overload_gating",
                      "The concepts really GATE overload resolution -- they "
                      "are constraints, not merely bools that happen to be "
                      "true",
                      &tests_concept_overload_gating },
                    { "tests_concepts_gating",
                      "The faces exist exactly where the standard allows "
                      "them: guarded away below C++20, leaving the structural "
                      "traits -- which degrade to C++11 -- as the floor",
                      &tests_concepts_gating },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_STRUCTURAL_TRAITS_TESTS_
