#include "transducer_tests.hpp"

// std
#include <type_traits>


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

/*
test_traits_is_reducing_state
  Verifies is_reducing_state recognises reducing_state specializations (cv/ref
  stripped) and rejects non-states, and that reducing_state_acc_t recovers the
  accumulator type.
  Tests the following:
  - is_reducing_state is true for reducing_state<int>, including const& form
  - is_reducing_state is false for a plain int
  - reducing_state_acc_t<reducing_state<int>> is int
*/
bool
test_traits_is_reducing_state(
)
{
    static_assert(is_reducing_state<reducing_state<int> >::value,
        "reducing_state<int> is a reducing_state");
    static_assert(is_reducing_state<const reducing_state<double>&>::value,
        "cv/ref-qualified reducing_state still matches");
    static_assert(!is_reducing_state<int>::value,
        "int is not a reducing_state");
    static_assert(
        std::is_same<reducing_state_acc_t<reducing_state<int> >, int>::value,
        "accumulator type is int");

    D_INTERNAL_TRD_CHECK(is_reducing_state<reducing_state<int> >::value);
    D_INTERNAL_TRD_CHECK(!is_reducing_state<int>::value);

    return true;
}


/*
test_traits_is_reducer
  Verifies is_reducer detects the (reducing_state<Acc>&, const Value&) step
  shape.
  Tests the following:
  - int_reducer models is_reducer<_, int, int>
  - not_a_reducer (a plain struct) does not
  - square (wrong arity / shape) does not
*/
bool
test_traits_is_reducer(
)
{
    static_assert(is_reducer<int_reducer, int, int>::value,
        "int_reducer is a reducer over int/int");
    static_assert(!is_reducer<not_a_reducer, int, int>::value,
        "a plain struct is not a reducer");
    static_assert(!is_reducer<square, int, int>::value,
        "square has the wrong call shape for a reducer");

    D_INTERNAL_TRD_CHECK((is_reducer<int_reducer, int, int>::value));
    D_INTERNAL_TRD_CHECK((!is_reducer<not_a_reducer, int, int>::value));

    return true;
}


/*
test_traits_is_transducer
  Verifies the marker-based is_transducer recognises this module's helpers and
  rejects non-transducers.
  Tests the following:
  - map / filter / a composed pipeline are transducers
  - int_reducer and int are not
*/
bool
test_traits_is_transducer(
)
{
    auto m    = transducers::map(square());
    auto f    = transducers::filter(is_even());
    auto comp = m | f;

    static_assert(is_transducer<decltype(m)>::value,
        "map is a transducer");
    static_assert(is_transducer<decltype(f)>::value,
        "filter is a transducer");
    static_assert(is_transducer<decltype(comp)>::value,
        "a composed pipeline is a transducer");
    static_assert(!is_transducer<int_reducer>::value,
        "a reducer is not a transducer");
    static_assert(!is_transducer<int>::value,
        "int is not a transducer");

    D_INTERNAL_TRD_CHECK(is_transducer<decltype(comp)>::value);
    D_INTERNAL_TRD_CHECK(!is_transducer<int>::value);

    return true;
}


/*
test_traits_transduces_reducer
  Verifies the behavioural transduces_reducer trait: a transducer applied to a
  concrete reducer yields a callable.
  Tests the following:
  - map / filter / composed all transduce an int_reducer
  - a non-transducer (int) does not
*/
bool
test_traits_transduces_reducer(
)
{
    auto m    = transducers::map(square());
    auto f    = transducers::filter(is_even());
    auto comp = m | f;

    static_assert(transduces_reducer<decltype(m), int_reducer>::value,
        "map transduces an int_reducer");
    static_assert(transduces_reducer<decltype(f), int_reducer>::value,
        "filter transduces an int_reducer");
    static_assert(transduces_reducer<decltype(comp), int_reducer>::value,
        "a composed pipeline transduces an int_reducer");
    static_assert(!transduces_reducer<int, int_reducer>::value,
        "int does not transduce a reducer");

    D_INTERNAL_TRD_CHECK((transduces_reducer<decltype(comp), int_reducer>::value));
    D_INTERNAL_TRD_CHECK((!transduces_reducer<int, int_reducer>::value));

    return true;
}


/*
test_traits_result_type
  Verifies transducer_result_t yields a concrete (non-nonesuch) reducer type
  for a valid transducer / reducer pairing.
  Tests the following:
  - transducer_result_t<map, int_reducer> is not the call_nonesuch sentinel
*/
bool
test_traits_result_type(
)
{
    auto m = transducers::map(square());
    using produced = transducer_result_t<decltype(m), int_reducer>;

    static_assert(
        !std::is_same<produced, internal::call_nonesuch>::value,
        "applying map to a reducer yields a real reducer type");

    D_INTERNAL_TRD_CHECK(
        (!std::is_same<produced, internal::call_nonesuch>::value));

    return true;
}


/*
test_traits_value_aliases
  Verifies the variable-template _v shorthands agree with their trait classes.
  Available only when variable templates are supported (C++14+), which the
  module already requires.
  Tests the following:
  - is_reducing_state_v, is_reducer_v, is_transducer_v, transduces_reducer_v
    match the corresponding ::value
*/
bool
test_traits_value_aliases(
)
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    auto m = transducers::map(square());

    static_assert(is_reducing_state_v<reducing_state<int> >,
        "is_reducing_state_v matches");
    static_assert(!is_reducing_state_v<int>,
        "is_reducing_state_v false for int");
    static_assert((is_reducer_v<int_reducer, int, int>),
        "is_reducer_v matches");
    static_assert(is_transducer_v<decltype(m)>,
        "is_transducer_v matches");
    static_assert((transduces_reducer_v<decltype(m), int_reducer>),
        "transduces_reducer_v matches");

    D_INTERNAL_TRD_CHECK(is_transducer_v<decltype(m)>);
    D_INTERNAL_TRD_CHECK((is_reducer_v<int_reducer, int, int>));
#endif

    return true;
}


/*
test_traits_concepts
  Verifies the C++20 concepts accept the right shapes and reject the wrong
  ones. Compiled only under C++20+.
  Tests the following:
  - reducing_state_c accepts reducing_state<int>
  - reducer_c accepts int_reducer, rejects not_a_reducer
  - transducer_c accepts map over int_reducer, rejects a bare reducer
*/
bool
test_traits_concepts(
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    auto m = transducers::map(square());

    static_assert(reducing_state_c<reducing_state<int> >,
        "concept: reducing_state");
    static_assert(reducer_c<int_reducer, int, int>,
        "concept: reducer accepts int_reducer");
    static_assert(!reducer_c<not_a_reducer, int, int>,
        "concept: reducer rejects a plain struct");
    static_assert(transducer_c<decltype(m), int_reducer>,
        "concept: transducer accepts map over int_reducer");
    static_assert(!transducer_c<int_reducer, int_reducer>,
        "concept: a bare reducer is not a transducer");

    D_INTERNAL_TRD_CHECK((transducer_c<decltype(m), int_reducer>));
    D_INTERNAL_TRD_CHECK((!reducer_c<not_a_reducer, int, int>));
#endif

    return true;
}


/*
run_traits_tests
  Aggregates every structural-trait and concept test.
*/
bool
run_traits_tests(
)
{
    return ( test_traits_is_reducing_state()   &&
             test_traits_is_reducer()          &&
             test_traits_is_transducer()       &&
             test_traits_transduces_reducer()  &&
             test_traits_result_type()         &&
             test_traits_value_aliases()       &&
             test_traits_concepts() );
}

#else  // !DJINTERP_TEST_TRANSDUCER_ENABLED

bool
run_traits_tests(
)
{
    return true;
}

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp
