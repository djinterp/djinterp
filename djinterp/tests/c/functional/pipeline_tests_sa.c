#include ".\pipeline_tests_sa.h"


/*
d_tests_sa_pipeline_all
  Runs all pipeline unit tests across all sections:
  i.   pipeline creation (begin, begin_copy)
  ii.  pipeline operations (map, filter, fold, for_each, take, skip, chaining)
  iii. pipeline finalization (end, free)
*/
bool
d_tests_sa_pipeline_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    // i.   pipeline creation
    all_passed &= d_tests_sa_pipeline_creation_all(_test_info);

    // ii.  pipeline operations (chainable)
    all_passed &= d_tests_sa_pipeline_operations_all(_test_info);

    // iii. pipeline finalization
    all_passed &= d_tests_sa_pipeline_finalization_all(_test_info);

    return all_passed;
}
