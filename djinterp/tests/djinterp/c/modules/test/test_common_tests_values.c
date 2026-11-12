/*******************************************************************************
* djinterp [c][test]                                  test_common_tests_values.c
*
* test_common.h's suite, block "values": what a build fixes at compile time.
*
*
* path:      /tests/djinterp/c/modules/test/test_common_tests_values.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_common_tests.h"  // corresponding header


/*
d_tests_test_common_status_values
  The value set is written once, as macros, and both notations read it.
  Tests the following:
  - passed, failed, skipped, pending and error are 0 to 4, and the count is 5
  - each C enumerator equals its macro
*/
bool
d_tests_test_common_status_values(void)
{
    D_TEST_REQUIRE(D_TEST_STATUS_PASSED == 0);
    D_TEST_REQUIRE(D_TEST_STATUS_FAILED == 1);
    D_TEST_REQUIRE(D_TEST_STATUS_SKIPPED == 2);
    D_TEST_REQUIRE(D_TEST_STATUS_PENDING == 3);
    D_TEST_REQUIRE(D_TEST_STATUS_ERROR == 4);
    D_TEST_REQUIRE(D_TEST_STATUS_COUNT == 5);

    D_TEST_REQUIRE((int)D_TEST_STATUS_PASSED_E == D_TEST_STATUS_PASSED);
    D_TEST_REQUIRE((int)D_TEST_STATUS_FAILED_E == D_TEST_STATUS_FAILED);
    D_TEST_REQUIRE((int)D_TEST_STATUS_SKIPPED_E == D_TEST_STATUS_SKIPPED);
    D_TEST_REQUIRE((int)D_TEST_STATUS_PENDING_E == D_TEST_STATUS_PENDING);
    D_TEST_REQUIRE((int)D_TEST_STATUS_ERROR_E == D_TEST_STATUS_ERROR);

    return true;
}

/*
d_tests_test_common_identifier_types
  cfg_test_object.h requires a signed status and type id and an unsigned
callable id, each 1, 2, 4 or 8 bytes wide; the header asserts it, and this
reads the same facts at run time.
  Tests the following:
  - dj_test_status and dj_test_type_id are signed
  - dj_test_callable_id is unsigned, and D_TEST_NO_CALLABLE is its zero
  - every width is one the assertions allow
*/
bool
d_tests_test_common_identifier_types(void)
{
    D_TEST_REQUIRE((dj_test_status)-1 < 0);
    D_TEST_REQUIRE((dj_test_type_id)-1 < 0);
    D_TEST_REQUIRE((dj_test_callable_id)-1 > 0);

    D_TEST_REQUIRE(D_TEST_NO_CALLABLE == 0);
    D_TEST_REQUIRE(sizeof(D_TEST_NO_CALLABLE) ==
                   sizeof(dj_test_callable_id));

    D_TEST_REQUIRE( (sizeof(dj_test_status) == 1) ||
                    (sizeof(dj_test_status) == 2) ||
                    (sizeof(dj_test_status) == 4) ||
                    (sizeof(dj_test_status) == 8) );
    D_TEST_REQUIRE( (sizeof(dj_test_type_id) == 1) ||
                    (sizeof(dj_test_type_id) == 2) ||
                    (sizeof(dj_test_type_id) == 4) ||
                    (sizeof(dj_test_type_id) == 8) );
    D_TEST_REQUIRE( (sizeof(dj_test_callable_id) == 1) ||
                    (sizeof(dj_test_callable_id) == 2) ||
                    (sizeof(dj_test_callable_id) == 4) ||
                    (sizeof(dj_test_callable_id) == 8) );

    return true;
}
