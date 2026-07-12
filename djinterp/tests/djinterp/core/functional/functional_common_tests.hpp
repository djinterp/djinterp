/******************************************************************************
* djinterp [test]                                  functional_common_tests.hpp
*
*   Unit-test declarations for core/functional/functional_traits.hpp -- the
* shared callable vocabulary (and, since the merge, its C++20 concept faces).
* One declaration group per section of the module under test:
*
*     functional_traits_tests_callable.cpp   -- I.   is_callable,
*                                                     callable_result_t
*     functional_traits_tests_predicate.cpp  -- II.  is_predicate
*     functional_traits_tests_aliases.cpp    -- III. is_callable_v,
*                                                     is_predicate_v  (C++14+)
*     functional_traits_tests_concepts.cpp   -- IV.  Callable, Predicate (C++20)
*
*   FIXTURES.  The traits probe an EXPRESSION -- "can a const-lvalue F be called
* on Args?" -- so the fixtures are chosen to pin exactly that contract from both
* sides.  const_fn is callable; nonconst_fn (a non-const operator()) and
* rvalue_fn (an &&-qualified one) are NOT, because neither can be invoked on a
* const lvalue.  generic_fn / generic_pred have TEMPLATED operator()s, the shape
* the header promises to accept and the one std::function-style probes reject.
* On the predicate side the sharp edge is convertibility: pred_implicit returns a
* type with an implicit operator bool (a predicate), pred_explicit returns one
* with an EXPLICIT operator bool (not a predicate -- not implicitly convertible),
* and pred_int / pred_ptr return types that convert to bool by the ordinary
* standard conversions.
*
*   Fixtures are named functors rather than generic lambdas so the suite builds
* on every floor the module supports: C++11 (traits), C++14 (the _v shorthands),
* and C++20 (the concept faces).  The concept-facing helpers are gated with the
* module's own D_ENV_CPP_FEATURE_LANG_CONCEPTS.
*   All tests are flat in djinterp::testing.
*
* 
* path:      /tests/djinterp/core/functional/functional/
*                functional_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (callables, non-callables, predicates, bool-conversion probes)
I.    CALL TRAITS        (is_callable, callable_result_t)
II.   PREDICATE TRAIT    (is_predicate)
III.  CONVENIENCE ALIASES(is_callable_v, is_predicate_v)
IV.   CONCEPT FACES      (Callable, Predicate)
*/


#ifndef DJINTERP_TEST_FUNCTIONAL_TRAITS_TESTS_
#define DJINTERP_TEST_FUNCTIONAL_TRAITS_TESTS_ 1

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/core/functional/functional_traits.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// -- callables ------------------------------------------------------------

// const_fn: the ordinary shape -- a const operator(), callable on a const
// lvalue.
struct const_fn
{
    int operator()(int _x) const { return _x * 2; }
};

// nonconst_fn: operator() is NOT const, so a const-lvalue call is ill-formed --
// the trait must reject it.
struct nonconst_fn
{
    int operator()(int _x) { return _x * 2; }
};

// rvalue_fn: an &&-qualified operator(), callable only on an rvalue -- also
// rejected by a const-lvalue probe.
struct rvalue_fn
{
    int operator()(int _x) const && { return _x; }
};

// generic_fn: a TEMPLATED operator() -- the generic-lambda shape the header
// promises to accept.
struct generic_fn
{
    template<typename _Type>
    _Type operator()(_Type _x) const { return _x; }
};

// overloaded_fn: an overload set -- the trait must resolve per _Args.
struct overloaded_fn
{
    int         operator()(int) const                 { return 1; }
    std::string operator()(const std::string& _s) const { return _s; }
};

// void_fn: callable, but yields void (so callable, never a predicate).
struct void_fn
{
    void operator()(int) const {}
};

// ref_fn: takes and returns a reference -- pins argument-category exactness and
// reference preservation in the result type.
struct ref_fn
{
    int& operator()(int& _r) const { return _r; }
};

// niladic_fn / two_arg_fn / three_arg_fn: the arity spread.
struct niladic_fn
{
    int operator()() const { return 7; }
};

struct two_arg_fn
{
    int operator()(int _a, double _b) const { return _a + static_cast<int>(_b); }
};

struct three_arg_fn
{
    int operator()(int _a, int _b, int _c) const { return _a + _b + _c; }
};

// not_callable: no operator() at all.
struct not_callable
{
    int x;
};

// free functions (function pointers / references are callables too).
inline int  free_fn(int _x)   { return _x + 1; }
inline bool free_pred(int _x) { return _x > 0; }


// -- bool-conversion probes (the predicate edge) --------------------------

// implicit_bool: implicitly convertible to bool.
struct implicit_bool
{
    operator bool() const { return true; }
};

// explicit_bool: EXPLICITLY convertible only -- NOT implicitly convertible, so
// a function returning it is not a predicate.
struct explicit_bool
{
    explicit operator bool() const { return true; }
};


// -- predicates -----------------------------------------------------------

// pred_bool / pred_int / pred_ptr: results that are bool or convert to it.
struct pred_bool
{
    bool operator()(const int& _x) const { return _x > 0; }
};

struct pred_int
{
    int operator()(int _x) const { return _x; }
};

struct pred_ptr
{
    const char* operator()(int _x) const { return _x > 0 ? "y" : 0; }
};

// pred_implicit / pred_explicit: the convertibility edge.
struct pred_implicit
{
    implicit_bool operator()(int) const { return implicit_bool(); }
};

struct pred_explicit
{
    explicit_bool operator()(int) const { return explicit_bool(); }
};

// pred_str: callable, but the result does not convert to bool.
struct pred_str
{
    std::string operator()(int) const { return "no"; }
};

// generic_pred: a TEMPLATED predicate -- a predicate for many _Arg at once.
struct generic_pred
{
    template<typename _Type>
    bool operator()(const _Type& _x) const { return static_cast<bool>(_x); }
};

// str_pred: a predicate over std::string, not over int.
struct str_pred
{
    bool operator()(const std::string& _s) const { return !_s.empty(); }
};


///////////////////////////////////////////////////////////////////////////////
///             CONCEPT-FACING HELPERS  (C++20)                             ///
///////////////////////////////////////////////////////////////////////////////
//   The header's own USAGE forms, compiled: a constrained template parameter
// (partial application of Predicate) and a requires-clause. Plus an overload
// pair that proves the concepts actually GATE resolution rather than merely
// evaluating to a bool.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// keep_if: the USAGE example -- Predicate<const int&> as a constrained template
// parameter (the concept partially applied to its second argument).
template<Predicate<const int&> _Pred>
int keep_if(
    const std::vector<int>& _v,
    _Pred                   _p
)
{
    int n = 0;
    for (std::vector<int>::const_iterator it = _v.begin(); it != _v.end(); ++it)
    {
        if (_p(*it)) { ++n; }
    }
    return n;
}

// apply_to: the USAGE example -- Callable in a requires-clause.
template<typename _Fn,
         typename _Type>
    requires Callable<_Fn, const _Type&>
auto apply_to(
    const _Type& _t,
    _Fn          _f
)
-> decltype(_f(_t))
{
    return _f(_t);
}

// which_pred / which_callable: constrained overload + unconstrained fallback.
// The constrained one must WIN for a matching argument and be excluded
// otherwise.
template<typename _Pred>
    requires Predicate<_Pred, int>
int which_pred(_Pred) { return 1; }

template<typename _Type>
int which_pred(_Type) { return 0; }

template<typename _Fn>
    requires Callable<_Fn, int>
int which_callable(_Fn) { return 1; }

template<typename _Type>
int which_callable(_Type) { return 0; }

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             I.    CALL TRAITS                                           ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_callable_positive();
bool tests_is_callable_negative();
bool tests_is_callable_const_lvalue();
bool tests_is_callable_templated_operator();
bool tests_is_callable_overload_set();
bool tests_is_callable_arg_categories();
bool tests_is_callable_arity_spread();
bool tests_is_callable_integral_constant();
bool tests_is_callable_reuses_invocable_with();
bool tests_callable_result_basic();
bool tests_callable_result_nonesuch();
bool tests_callable_result_preserves_reference();
bool tests_callable_result_is_alias();
bool tests_reexports_function_traits();


///////////////////////////////////////////////////////////////////////////////
///             II.   PREDICATE TRAIT                                       ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_predicate_positive();
bool tests_is_predicate_negative();
bool tests_is_predicate_convertible_results();
bool tests_is_predicate_explicit_operator_bool();
bool tests_is_predicate_const_lvalue();
bool tests_is_predicate_arg_type();
bool tests_is_predicate_templated_operator();
bool tests_is_predicate_single_argument();
bool tests_is_predicate_reuses_invocable_r_with();
bool tests_is_predicate_integral_constant();


///////////////////////////////////////////////////////////////////////////////
///             III.  CONVENIENCE ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_callable_v_agrees();
bool tests_is_predicate_v_agrees();
bool tests_aliases_are_constant_expressions();
bool tests_is_callable_v_variadic();
bool tests_aliases_gating();


///////////////////////////////////////////////////////////////////////////////
///             IV.   CONCEPT FACES                                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_concept_callable_mirrors_trait();
bool tests_concept_predicate_mirrors_trait();
bool tests_concept_constrained_parameter();
bool tests_concept_requires_clause();
bool tests_concept_overload_gating();
bool tests_concept_callable_but_not_predicate();
bool tests_concepts_gating();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTIONAL_TRAITS_TESTS_
