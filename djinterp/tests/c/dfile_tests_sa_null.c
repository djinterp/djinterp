/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_null.c
*
*   Tests for NULL parameter handling across all functions.
*
*
* path:      \src	est\dfile_tests_sa_null.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * NULL PARAMETER TESTS
 *****************************************************************************/

/*
d_tests_dfile_null_params_all
  Tests that all functions handle NULL parameters gracefully.
  Tests the following:
  - various functions don't crash with NULL input
  - proper error codes or NULL returns for NULL params
*/
struct d_test_object*
d_tests_dfile_null_params_all
(
    void
)
{
    struct d_test_object* group;
    bool                  test_fopen;
    bool                  test_stat;
    bool                  test_mkdir;
    bool                  test_getcwd;
    bool                  test_path_join;
    size_t                idx;

    // test various NULL parameter handling
    test_fopen = (d_fopen(NULL, "r") == NULL);
    test_stat = (d_stat(NULL, NULL) != 0);
    test_mkdir = (d_mkdir(NULL, 0) != 0);
    test_getcwd = (d_getcwd(NULL, 0) == NULL);
    test_path_join = (d_path_join(NULL, 0, "a", "b") == NULL);

    // build result tree
    group = d_test_object_new_interior("NULL Parameter Handling", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("fopen",
                                           test_fopen,
                                           "d_fopen handles NULL gracefully");
    group->elements[idx++] = D_ASSERT_TRUE("stat",
                                           test_stat,
                                           "d_stat handles NULL gracefully");
    group->elements[idx++] = D_ASSERT_TRUE("mkdir",
                                           test_mkdir,
                                           "d_mkdir handles NULL gracefully");
    group->elements[idx++] = D_ASSERT_TRUE("getcwd",
                                           test_getcwd,
                                           "d_getcwd handles NULL gracefully");
    group->elements[idx++] = D_ASSERT_TRUE("path_join",
                                           test_path_join,
                                           "d_path_join handles NULL gracefully");

    return group;
}


