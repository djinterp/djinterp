#include "extractor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_pipeline_then_extract_adapter
  Verifies `inner | then_extract(outer)` composes via the pipeline operator.
  Tests the following:
  - get_age | then_extract(times_two) yields 2 * age
*/
bool
test_pipeline_then_extract_adapter(
)
{
    person p = { 1, 21, 10 };
    int v = ( extractors::from_function(get_age())
              | extractors::then_extract(times_two()) )(p);

    D_INTERNAL_EXT_CHECK(v == 42);

    return true;
}


/*
test_pipeline_mapped_adapter
  Verifies `e | mapped(fn)` post-transforms via the pipeline operator.
  Tests the following:
  - from_member(&person::age) | mapped(times_two) yields 2 * age
*/
bool
test_pipeline_mapped_adapter(
)
{
    person p = { 1, 16, 10 };
    int v = ( extractors::from_member(&person::age)
              | extractors::mapped(times_two()) )(p);

    D_INTERNAL_EXT_CHECK(v == 32);

    return true;
}


/*
test_pipeline_filtered_adapter
  Verifies `e | filtered(p)` gates via the pipeline operator and yields
  maybe<T>.
  Tests the following:
  - on an adult, the piped filter yields a present maybe holding the age
  - on a minor, it yields nothing
*/
bool
test_pipeline_filtered_adapter(
)
{
    person adult = { 1, 30, 10 };
    person minor = { 2, 15, 20 };

    maybe<int> m_ok = ( extractors::from_member(&person::age)
                        | extractors::filtered(is_adult()) )(adult);
    D_INTERNAL_EXT_CHECK(m_ok.has_value());
    D_INTERNAL_EXT_CHECK(m_ok.value() == 30);

    maybe<int> m_no = ( extractors::from_member(&person::age)
                        | extractors::filtered(is_adult()) )(minor);
    D_INTERNAL_EXT_CHECK(m_no.is_nothing());

    return true;
}


/*
test_pipeline_chained
  Verifies a multi-stage pipeline mixing mapped then filtered. Stage order
  must be respected: map the age first, then gate the mapped value.
  Tests the following:
  - age | mapped(times_two) | filtered(is_adult): a minor aged 15 maps to 30,
    which then PASSES the adult gate (30 >= 18) -> present maybe holding 30
*/
bool
test_pipeline_chained(
)
{
    person minor = { 2, 15, 20 };

    maybe<int> m = ( extractors::from_member(&person::age)
                     | extractors::mapped(times_two())
                     | extractors::filtered(is_adult()) )(minor);

    D_INTERNAL_EXT_CHECK(m.has_value());
    D_INTERNAL_EXT_CHECK(m.value() == 30);

    return true;
}


/*
test_pipeline_equivalence
  Confirms the pipeline forms are equivalent to the direct factory calls.
  Tests the following:
  - e | then_extract(f) equals then_extract(e, f)
  - e | mapped(f) equals mapped(e, f)
*/
bool
test_pipeline_equivalence(
)
{
    person p = { 1, 21, 10 };

    int piped_then = ( extractors::from_function(get_age())
                       | extractors::then_extract(times_two()) )(p);
    int direct_then =
        extractors::then_extract(extractors::from_function(get_age()),
                                 times_two())(p);
    D_INTERNAL_EXT_CHECK(piped_then == direct_then);

    int piped_map = ( extractors::from_member(&person::age)
                      | extractors::mapped(times_two()) )(p);
    int direct_map =
        extractors::mapped(extractors::from_member(&person::age),
                           times_two())(p);
    D_INTERNAL_EXT_CHECK(piped_map == direct_map);

    return true;
}


/*
run_pipeline_tests
  Aggregates every pipeline-section test.
  Tests the following:
  - all operator| (then_extract / mapped / filtered) tests pass, including a
    chained pipeline and equivalence with the direct factory forms
*/
bool
run_pipeline_tests(
)
{
    return ( test_pipeline_then_extract_adapter() &&
             test_pipeline_mapped_adapter()        &&
             test_pipeline_filtered_adapter()      &&
             test_pipeline_chained()               &&
             test_pipeline_equivalence() );
}


NS_END  // testing
NS_END  // djinterp
