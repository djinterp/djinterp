// djinterp [test]  structural_traits_tests_optional.cpp
//   Section III -- produces_optional_like and is_unfold_step: the pull-based
//   source protocols (bool-testable AND dereferenceable).

// std
#include <string>
#include <type_traits>
// djinterp
#include "structural_traits_tests.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#  include <optional>
#endif


NS_DJINTERP
NS_TESTING


/*
tests_produces_optional_like_positive
  A nullary callable whose result is both bool-testable and dereferenceable is a
  well-formed pull source. maybe<>, a raw pointer, and std::optional all satisfy
  it -- the header's own claim.
  Tests the following:
  - a stateful maybe-source, a const one, and a pointer-yielding one
  - std::optional, where the standard provides it
*/
bool
tests_produces_optional_like_positive()
{
    bool ok = true;

    ok = ok && (produces_optional_like<src_pull>::value);    // maybe<>, stateful
    ok = ok && (produces_optional_like<src_const>::value);   // maybe<>, const
    ok = ok && (produces_optional_like<src_ptr>::value);     // T*

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std::optional is bool-testable (explicitly) and dereferenceable.
    auto opt_src = [](){ return std::optional<int>(42); };
    ok = ok && (produces_optional_like<decltype(opt_src)>::value);
#endif

    return ok;
}


/*
tests_produces_optional_like_requires_bool
  BOTH halves are required. A result that is dereferenceable but NOT
  bool-testable fails on the "is there a value?" half.
  Tests the following:
  - a source yielding a dereferenceable-only type is rejected
  - that type really is dereferenceable (so only the bool half is missing)
*/
bool
tests_produces_optional_like_requires_bool()
{
    bool ok = true;

    ok = ok && (!produces_optional_like<src_deref>::value);

    // it IS a nullary callable, and its result IS dereferenceable.
    ok = ok && (is_nullary_callable<src_deref>::value);
    src_deref s;
    ok = ok && (*s() == 1);

    return ok;
}


/*
tests_produces_optional_like_requires_deref
  The other half. A result that is bool-testable but NOT dereferenceable fails on
  the "give me the value" half -- so a plain int-yielding source, which would pass
  a bool-only check, is correctly refused.
  Tests the following:
  - a source yielding int is rejected
  - it is nullary, and its result is bool-testable (so only deref is missing)
*/
bool
tests_produces_optional_like_requires_deref()
{
    bool ok = true;

    ok = ok && (!produces_optional_like<src_int>::value);

    ok = ok && (is_nullary_callable<src_int>::value);
    src_int    s;
    const bool testable = static_cast<bool>(s());
    ok = ok && (testable);

    return ok;
}


/*
tests_produces_optional_like_requires_nullary
  The source step is NULLARY -- a unary step is a different role (that is
  is_unfold_step's job).
  Tests the following:
  - a unary optional-yielding step is not a source
  - a void-yielding nullary is not one either
  - a non-callable is rejected
*/
bool
tests_produces_optional_like_requires_nullary()
{
    bool ok = true;

    ok = ok && (!produces_optional_like<step_count>::value);   // unary
    ok = ok && (!produces_optional_like<src_void>::value);     // void
    ok = ok && (!produces_optional_like<not_callable>::value);
    ok = ok && (!produces_optional_like<int>::value);

    return ok;
}


/*
tests_produces_optional_like_pull_protocol
  The detected shape really is the pull protocol: each call advances the source
  and answers "more values?", and the source finally exhausts.
  Tests the following:
  - three values are pulled, in order, then the source reports empty
  - the source is stateful, hence its non-const operator()
*/
bool
tests_produces_optional_like_pull_protocol()
{
    bool ok = true;

    src_pull source;
    int      got[8];
    int      n = 0;

    for (int guard = 0; guard < 8; ++guard)
    {
        maybe<int> m = source();
        if (!static_cast<bool>(m)) { break; }
        got[n] = *m;
        ++n;
    }

    ok = ok && (n == 3);
    ok = ok && (got[0] == 0);
    ok = ok && (got[1] == 10);
    ok = ok && (got[2] == 20);

    // exhausted: it keeps reporting empty.
    ok = ok && (!static_cast<bool>(source()));

    return ok;
}


/*
tests_is_unfold_step_positive
  The unary, state-threaded analog: Step(state) -> maybe<next>. The compile-time
  form of a stateful source.
  Tests the following:
  - a const step and a stateful (non-const) step over int state
  - a lambda step
*/
bool
tests_is_unfold_step_positive()
{
    bool ok = true;

    ok = ok && (is_unfold_step<step_count, int>::value);
    ok = ok && (is_unfold_step<step_mut, int>::value);

    auto lam = [](int _s){ maybe<int> m = { _s + 1, _s < 2 }; return m; };
    ok = ok && (is_unfold_step<decltype(lam), int>::value);

    return ok;
}


/*
tests_is_unfold_step_negative
  Tests the following:
  - a unary step whose result is a plain value is not an unfold step
  - the state type must be one the step accepts
  - a non-callable is rejected
*/
bool
tests_is_unfold_step_negative()
{
    bool ok = true;

    ok = ok && (!is_unfold_step<step_plain, int>::value);       // not optional-like
    ok = ok && (!is_unfold_step<step_count, std::string>::value); // wrong state
    ok = ok && (!is_unfold_step<not_callable, int>::value);
    ok = ok && (!is_unfold_step<int, int>::value);

    // step_plain IS a unary callable -- it fails only the optional-like half.
    ok = ok && (is_unary_callable<step_plain, int>::value);

    return ok;
}


/*
tests_is_unfold_step_requires_both
  As with produces_optional_like, both halves are required of the result.
  Tests the following:
  - bool-testable but not dereferenceable (a plain int) is rejected
  - dereferenceable but not bool-testable is rejected
*/
bool
tests_is_unfold_step_requires_both()
{
    bool ok = true;

    ok = ok && (!is_unfold_step<step_plain, int>::value);   // no deref
    ok = ok && (!is_unfold_step<step_deref, int>::value);   // no bool

    // both are unary callables; each fails exactly one half.
    ok = ok && (is_unary_callable<step_plain, int>::value);
    ok = ok && (is_unary_callable<step_deref, int>::value);

    return ok;
}


/*
tests_is_unfold_step_unfold_protocol
  The detected shape is a working unfold: thread the state, take a value while
  the step yields one, stop when it does not.
  Tests the following:
  - the state advances 0 -> 1 -> 2 -> 3 and then terminates
*/
bool
tests_is_unfold_step_unfold_protocol()
{
    bool ok = true;

    const step_count step;
    int              state = 0;
    int              n     = 0;

    for (int guard = 0; guard < 8; ++guard)
    {
        maybe<int> m = step(state);
        if (!static_cast<bool>(m)) { break; }
        state = *m;
        ++n;
    }

    ok = ok && (n == 3);
    ok = ok && (state == 3);
    ok = ok && (!static_cast<bool>(step(state)));   // terminated

    return ok;
}


/*
tests_optional_like_nullary_vs_unary
  The two traits are the nullary and the unary halves of one idea, and they do not
  overlap: a source is not an unfold step and an unfold step is not a source.
  clean_t on the state means its cv-ref spellings collapse, as elsewhere.
  Tests the following:
  - each fixture is recognised by exactly one of the two
  - int, int&, and const int& all give the same answer for the state
*/
bool
tests_optional_like_nullary_vs_unary()
{
    bool ok = true;

    // a nullary source is not an unfold step (it takes no state).
    ok = ok && (produces_optional_like<src_pull>::value);
    ok = ok && (!is_unfold_step<src_pull, int>::value);

    // an unfold step is not a nullary source.
    ok = ok && (is_unfold_step<step_count, int>::value);
    ok = ok && (!produces_optional_like<step_count>::value);

    // the state decays, like every other argument in this header.
    ok = ok && (is_unfold_step<step_count, int&>::value);
    ok = ok && (is_unfold_step<step_count, const int&>::value);

    // and the step type itself decays.
    ok = ok && (is_unfold_step<const step_count&, int>::value);
    ok = ok && (produces_optional_like<const src_pull&>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
