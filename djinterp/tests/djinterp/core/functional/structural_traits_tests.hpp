/******************************************************************************
* djinterp [test]                                  structural_traits_tests.hpp
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
*     structural_traits_tests_optional.cpp  -- III. produces_optional_like,
*                                                    is_unfold_step
*     structural_traits_tests_aliases.cpp   -- IV.  the _v shorthands (C++14+)
*     structural_traits_tests_concepts.cpp  -- V.   BinaryCallable, Reducer,
*                                                    Transducer, UnfoldStep
*
*   FIXTURES.  Every trait here is a SFINAE probe of one exact expression, so the
* fixtures are chosen to pin each probe from every side it can be got at.
*
*   The arity trilogy probes `declval<T&>()` -- a MUTABLE lvalue.  Four fixtures
* determine that expression uniquely and admit no other: const_step (a const
* operator(), accepted), mut_step (a NON-const one, accepted -- so the probe is
* not `const T&`), lref_step (an &-qualified one, accepted -- so it is an lvalue),
* and rref_step (an &&-qualified one, REJECTED -- so it is not `T&&`).  The
* mutable half is load-bearing, not incidental: a pull source advances on every
* call and a tallying reducer accumulates, so both must have a non-const
* operator().  Note this is the OPPOSITE of functional_traits' is_callable, which
* probes a const lvalue -- and note has_find_method, in this same header, probes
* `declval<const T&>()` too.
*
*   has_find_method converts the result with static_cast<bool>, an EXPLICIT
* conversion, so pat_explicit (whose find returns a type with an explicit
* operator bool) is accepted where an implicit-convertibility check would refuse
* it.  The optional-like traits require BOTH halves -- bool-testable AND
* dereferenceable -- so src_int (testable, not dereferenceable) and src_deref
* (dereferenceable, not testable) each fail on exactly one.
*
*   Fixtures are named functors rather than lambdas, and maybe<T> is a plain
* aggregate, so the suite builds on every floor the module claims: C++11 through
* C++20.  std::optional appears only under a C++17 gate.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/functional/structural_traits_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (patterns, callables by arity and qualifier, sources, steps)
I.    MEMBER DETECTION   (has_match_result_type, has_find_method)
II.   ARITY TRILOGY      (is_nullary_callable, is_unary_callable,
                          is_binary_callable)
III.  OPTIONAL-LIKE      (produces_optional_like, is_unfold_step)
IV.   CONVENIENCE ALIASES(the _v shorthands)
V.    PROTOCOL CONCEPTS  (BinaryCallable, Reducer, Transducer, UnfoldStep)
*/


#ifndef DJINTERP_TEST_STRUCTURAL_TRAITS_TESTS_
#define DJINTERP_TEST_STRUCTURAL_TRAITS_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp (module under test)
#include "../../core/functional/structural_traits.hpp"


NS_DJINTERP
NS_TESTING


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

// deref_only: dereferenceable but NOT bool-testable -- fails exactly one half.
struct deref_only
{
    int operator*() const { return 1; }
};

// explicit_bool: convertible to bool only EXPLICITLY (static_cast reaches it,
// an implicit-conversion check would not).
struct explicit_bool
{
    explicit operator bool() const { return true; }
};


// -- patterns (the scanner protocol) --------------------------------------

// pat_scan
//   fixture pattern: the canonical searchable shape --
// `bool find(const In&, size_t&, Result&) const` -- and it really scans, so the
// detected shape can be driven, not merely recognised. Finds each 'x' in the
// input, writing the position into the result and advancing pos.
struct pat_scan
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

// pat_none:      no find at all.
struct pat_none
{
};

// pat_nonconst:  find is NOT const -- rejected, because has_find_method probes a
//                const lvalue (unlike the arity traits in this same header).
struct pat_nonconst
{
    bool find(const std::string&, std::size_t&, int&) { return true; }
};

// pat_void:      find yields void -- not static_cast-able to bool.
struct pat_void
{
    void find(const std::string&, std::size_t&, int&) const {}
};

// pat_string:    find yields a type with no conversion to bool.
struct pat_string
{
    std::string find(const std::string&, std::size_t&, int&) const
    {
        return std::string();
    }
};

// pat_explicit:  find yields an EXPLICITLY bool-convertible type -- accepted,
//                since the probe uses static_cast<bool>.
struct pat_explicit
{
    explicit_bool find(const std::string&, std::size_t&, int&) const
    {
        return explicit_bool();
    }
};

// pat_int:       find yields int -- converts to bool, so accepted.
struct pat_int
{
    int find(const std::string&, std::size_t&, int&) const { return 1; }
};

// pat_byvalue:   find takes size_t by value and the result by const ref -- still
//                callable with the probe's lvalues, so accepted.
struct pat_byvalue
{
    bool find(const std::string&, std::size_t, const int&) const { return true; }
};

// pat_arity:     find takes two arguments.
struct pat_arity
{
    bool find(const std::string&, std::size_t&) const { return true; }
};

// pat_wrong_in:  find takes the wrong input type.
struct pat_wrong_in
{
    bool find(int, std::size_t&, int&) const { return true; }
};

// pat_data:      `find` is a data member, not a member function.
struct pat_data
{
    int find;
};

// mrt_only / mrt_data: the nested-type detector, from both sides.
struct mrt_only
{
    using match_result_type = std::string;
};

struct mrt_data
{
    int match_result_type;      // a data member, NOT a nested type
};


// -- callables, by arity and by qualifier ---------------------------------

// const_step:  a const operator()          -- accepted (callable on an lvalue).
struct const_step
{
    int operator()(int _x) const { return _x * 2; }
};

// mut_step:    a NON-const operator()      -- accepted; the probe is not const.
struct mut_step
{
    int calls;

    mut_step() : calls(0) {}

    int operator()(int _x) { ++calls; return _x * 2; }
};

// lref_step:   an &-qualified operator()   -- accepted; the probe is an lvalue.
struct lref_step
{
    int operator()(int _x) & { return _x; }
};

// rref_step:   an &&-qualified operator()  -- REJECTED; the probe is not T&&.
struct rref_step
{
    int operator()(int _x) && { return _x; }
};

// in_place:    takes int& -- undetectable, because clean_t strips the reference
//              off _Arg and the probe then offers an rvalue.
struct in_place
{
    void operator()(int& _r) const { _r *= 2; }
};

// generic_step: a templated operator().
struct generic_step
{
    template<typename _Type>
    _Type operator()(_Type _x) const { return _x; }
};

// nil_const / nil_mut / nil_lref / nil_rref: the same quartet, at arity zero.
struct nil_const
{
    int operator()() const { return 7; }
};

struct nil_mut
{
    int calls;

    nil_mut() : calls(0) {}

    int operator()() { return ++calls; }
};

struct nil_lref
{
    int operator()() & { return 1; }
};

struct nil_rref
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

// tally_reducer: a STATEFUL binary step -- non-const, as an accumulating reducer
//                must be.
struct tally_reducer
{
    int seen;

    tally_reducer() : seen(0) {}

    int operator()(int _acc, int _x) { ++seen; return _acc + _x; }
};

// void_reducer:  binary, but yields void -- still satisfies Reducer, which
//                checks arity only, not the (acc, x) -> acc result.
struct void_reducer
{
    void operator()(int, int) const {}
};

// doubled_reducer: the reducer a transducer produces -- doubles each element
//                  before handing it to the wrapped reducer.
struct doubled_reducer
{
    sum_reducer rf;

    int operator()(int _acc, int _x) const { return rf(_acc, _x * 2); }
};

// doubling_xform:  a transducer -- reducer -> reducer.
struct doubling_xform
{
    doubled_reducer operator()(sum_reducer _rf) const
    {
        doubled_reducer r = { _rf };
        return r;
    }
};


// -- sources (nullary) and unfold steps (unary, state-threaded) -----------

// src_pull:  the canonical pull source -- stateful, so operator() is non-const;
//            yields three values, then exhausts.
struct src_pull
{
    int n;

    src_pull() : n(0) {}

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

// src_const: a const nullary source (also accepted -- const is callable on an
//            lvalue).
struct src_const
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

// src_int:   yields int -- bool-testable, NOT dereferenceable.
struct src_int
{
    int operator()() const { return 1; }
};

// src_deref: yields deref_only -- dereferenceable, NOT bool-testable.
struct src_deref
{
    deref_only operator()() const { return deref_only(); }
};

// src_void:  yields void -- neither.
struct src_void
{
    void operator()() const {}
};

// step_count: an unfold step -- State -> maybe<next State>; terminates at 3.
struct step_count
{
    maybe<int> operator()(int _s) const
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

// step_mut:   a stateful unfold step -- non-const operator().
struct step_mut
{
    int calls;

    step_mut() : calls(0) {}

    maybe<int> operator()(int _s)
    {
        ++calls;
        maybe<int> m = { _s + 1, _s < 2 };
        return m;
    }
};

// step_plain: unary, but yields a plain value -- not optional-like.
struct step_plain
{
    int operator()(int _s) const { return _s; }
};

// step_deref: unary, yields deref_only -- dereferenceable, not bool-testable.
struct step_deref
{
    deref_only operator()(int) const { return deref_only(); }
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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_STRUCTURAL_TRAITS_TESTS_
