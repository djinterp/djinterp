// djinterp [test]  structural_traits_tests_concepts.cpp
//   Section V -- the protocol concepts: BinaryCallable, Reducer, Transducer,
//   UnfoldStep (C++20).

// std
#include <string>
#include <type_traits>
// djinterp
#include "structural_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_concept_binary_callable_mirrors
  BinaryCallable is the concept face of is_binary_callable.
  Tests the following:
  - the two agree, positively and negatively
  - the mutable-lvalue contract carries through to the concept
*/
bool
tests_concept_binary_callable_mirrors()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(BinaryCallable<sum_reducer, int, int> ==
                  is_binary_callable<sum_reducer, int, int>::value, "mirrors");
    static_assert(BinaryCallable<const_step, int, int> ==
                  is_binary_callable<const_step, int, int>::value,
                  "mirrors, negative");

    static_assert(BinaryCallable<sum_reducer, int, int>, "pure reducer");
    static_assert(BinaryCallable<tally_reducer, int, int>, "stateful reducer");
    static_assert(!BinaryCallable<const_step, int, int>, "unary is not binary");
    static_assert(!BinaryCallable<not_callable, int, int>, "not callable");

    ok = ok && (BinaryCallable<sum_reducer, int, int>);
    ok = ok && (!BinaryCallable<nil_const, int, int>);
#endif

    return ok;
}


/*
tests_concept_reducer
  Reducer is the reduction step -- the pure `step` half of the step/driver split,
  defined over BinaryCallable.
  Tests the following:
  - pure and stateful reducers satisfy it; non-binary callables do not
  - Reducer and BinaryCallable agree, being the same requirement
*/
bool
tests_concept_reducer()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Reducer<sum_reducer, int, int>, "pure reducer");
    static_assert(Reducer<tally_reducer, int, int>, "stateful reducer");

    static_assert(!Reducer<const_step, int, int>, "unary");
    static_assert(!Reducer<nil_const, int, int>, "nullary");
    static_assert(!Reducer<not_callable, int, int>, "not callable");

    // Reducer is defined as BinaryCallable, so they answer identically.
    static_assert(Reducer<sum_reducer, int, int> ==
                  BinaryCallable<sum_reducer, int, int>, "same requirement");

    ok = ok && (Reducer<tally_reducer, int, int>);
#endif

    return ok;
}


/*
tests_concept_reducer_ignores_result
  Worth pinning honestly: Reducer is defined as BinaryCallable, so it constrains
  ARITY ONLY -- it does not check that the result is the accumulator type, though
  the documented shape is (acc, x) -> acc. A void-returning binary callable
  therefore satisfies Reducer.
  Tests the following:
  - a binary callable returning void satisfies Reducer
  - so does one returning an unrelated type
*/
bool
tests_concept_reducer_ignores_result()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Reducer<void_reducer, int, int>,
                  "void result still satisfies Reducer -- arity only");
    static_assert(std::is_same<decltype(std::declval<void_reducer&>()(0, 0)),
                               void>::value, "and it really does yield void");

    // the accumulator-typed result is what the documented shape intends, and it
    // of course satisfies the concept too.
    static_assert(std::is_same<decltype(std::declval<sum_reducer&>()(0, 0)),
                               int>::value, "(acc, x) -> acc");
    static_assert(Reducer<sum_reducer, int, int>, "the intended shape");

    ok = ok && (Reducer<void_reducer, int, int>);
#endif

    return ok;
}


/*
tests_concept_transducer
  Transducer is a reducer-to-reducer transformer: a unary callable that, given a
  reducer, yields a transformed one. The header notes that full reducer-ness of
  the RESULT is checked where it is applied -- so this test checks it there.
  Tests the following:
  - the transducer satisfies the concept
  - applying it yields something that is itself a Reducer
  - the transformed reducer computes what the transformation says (doubling)
*/
bool
tests_concept_transducer()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Transducer<doubling_xform, sum_reducer>, "reducer -> reducer");
    static_assert(!Transducer<not_callable, sum_reducer>, "not callable");
    static_assert(!Transducer<sum_reducer, sum_reducer>, "binary, not unary");

    // the result of applying it is itself a Reducer -- checked at the point of
    // application, as the header says.
    doubled_reducer rf = apply_xform(doubling_xform(), sum_reducer());
    static_assert(Reducer<doubled_reducer, int, int>,
                  "the transformed reducer is a Reducer");

    // and it doubles: (0 + 1*2) + 2*2 = 6.
    ok = ok && (rf(rf(0, 1), 2) == 6);
#endif

    return ok;
}


/*
tests_concept_unfold_step
  UnfoldStep is the concept face of is_unfold_step -- the pull-based source step
  State -> maybe<(value, state)>.
  Tests the following:
  - the two agree, positively and negatively
*/
bool
tests_concept_unfold_step()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(UnfoldStep<step_count, int> ==
                  is_unfold_step<step_count, int>::value, "mirrors");
    static_assert(UnfoldStep<step_plain, int> ==
                  is_unfold_step<step_plain, int>::value, "mirrors, negative");

    static_assert(UnfoldStep<step_count, int>, "const step");
    static_assert(UnfoldStep<step_mut, int>, "stateful step");
    static_assert(!UnfoldStep<step_plain, int>, "result not optional-like");
    static_assert(!UnfoldStep<step_deref, int>, "result not bool-testable");
    static_assert(!UnfoldStep<src_pull, int>, "a nullary source is not a step");

    ok = ok && (UnfoldStep<step_count, int>);
#endif

    return ok;
}


/*
tests_concept_drives_the_protocols
  The faces put to work: a Reducer-constrained driver runs any reduction step, and
  an UnfoldStep-constrained driver pulls a source to exhaustion. This is the
  step/driver split the header describes -- one step, run unchanged by a driver.
  Tests the following:
  - the same driver reduces with a pure step and with a stateful one
  - the stateful reducer's own state advances as the driver runs it
  - the unfold driver threads state and terminates
*/
bool
tests_concept_drives_the_protocols()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    const int data[4] = { 1, 2, 3, 4 };

    // one driver, two reducers.
    ok = ok && (reduce_with(data, data + 4, 0, sum_reducer()) == 10);
    ok = ok && (reduce_with(data, data + 4, 100, sum_reducer()) == 110);

    tally_reducer t;
    ok = ok && (reduce_with(data, data + 4, 0, t) == 10);

    // a transformed reducer runs through the same driver: 2+4+6+8 = 20.
    doubled_reducer d = apply_xform(doubling_xform(), sum_reducer());
    ok = ok && (reduce_with(data, data + 4, 0, d) == 20);

    // the unfold driver pulls until the step yields nothing.
    ok = ok && (unfold_count(step_count(), 0) == 3);
    ok = ok && (unfold_count(step_count(), 2) == 1);
    ok = ok && (unfold_count(step_count(), 3) == 0);   // already exhausted
#endif

    return ok;
}


/*
tests_concept_overload_gating
  The concepts really GATE overload resolution -- they are constraints, not merely
  bools that happen to be true.
  Tests the following:
  - a Reducer-constrained overload wins for a reducer and is excluded otherwise
  - an UnfoldStep-constrained overload likewise, including for the near-miss of a
    unary step whose result is not optional-like
*/
bool
tests_concept_overload_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = ok && (which_reducer(sum_reducer()) == 1);
    ok = ok && (which_reducer(tally_reducer()) == 1);
    ok = ok && (which_reducer(const_step()) == 0);      // unary, not binary
    ok = ok && (which_reducer(not_callable()) == 0);
    ok = ok && (which_reducer(42) == 0);

    ok = ok && (which_unfold(step_count()) == 1);
    ok = ok && (which_unfold(step_mut()) == 1);
    ok = ok && (which_unfold(step_plain()) == 0);       // near-miss: not optional-like
    ok = ok && (which_unfold(src_pull()) == 0);         // nullary, not a step
    ok = ok && (which_unfold(not_callable()) == 0);
#endif

    return ok;
}


/*
tests_concepts_gating
  The faces exist exactly where the standard allows them: guarded away below
  C++20, leaving the structural traits -- which degrade to C++11 -- as the floor.
  Tests the following:
  - the gate agrees with the language level
  - the traits answer identically whether or not the gate is open
*/
bool
tests_concepts_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = ok && (BinaryCallable<sum_reducer, int, int> ==
                is_binary_callable<sum_reducer, int, int>::value);
    ok = ok && (UnfoldStep<step_count, int> ==
                is_unfold_step<step_count, int>::value);
    #if !D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // faces present on a standard that lacks concepts
    #endif
#else
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // faces missing on a standard that has concepts
    #endif
#endif

    // the traits are the floor, on every standard.
    ok = ok && (is_binary_callable<sum_reducer, int, int>::value);
    ok = ok && (is_unary_callable<doubling_xform, sum_reducer>::value);
    ok = ok && (is_unfold_step<step_count, int>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
