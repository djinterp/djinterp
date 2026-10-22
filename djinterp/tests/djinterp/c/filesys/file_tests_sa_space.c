/*******************************************************************************
* djinterp [fs]                                            file_tests_sa_space.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_space.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * XV.  file_space  --  FILESYSTEM CAPACITY
 *****************************************************************************/

#if D_FILE_SPACE_IS_AVAILABLE

/*
d_tests_file_space
  Tests d_file_space.
  The assertion worth having is available <= free <= capacity. It looks
trivial and is not: `free` and `available` differ by the root reserve, by
quotas and by container limits, and on the machine this suite was written on
the gap was 233 GiB -- 249 free against 10 available. A program sizing a write
off `free` would have overcommitted by that much.
*/
static struct d_test_object*
d_tests_file_space_query
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_space_t      sp;
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_EQUAL("query",
                                  d_file_space(D_TEST_FILE_TEMP_DIR, &sp), 0,
                                  "d_file_space reports the filesystem's capacity");
    items[idx++] = D_ASSERT_TRUE("capacity", sp.capacity > 0,
                                 "the filesystem has a size");
    items[idx++] = D_ASSERT_TRUE("free_bounded", sp.free <= sp.capacity,
                                 "free cannot exceed capacity");

    // the ordering that makes the free/available distinction real
    items[idx++] = D_ASSERT_TRUE("available_bounded",
                                 sp.available <= sp.free,
                                 "available cannot exceed free (the root reserve)");

    // a failed query must ZERO the output, so a caller who ignores the return
    // code cannot read a plausible-looking stale number and act on it
    (void)d_file_space("no/such/path", &sp);
    items[idx++] = D_ASSERT_TRUE("zeroed_on_failure",
                                 (sp.capacity == 0) &&
                                 (sp.free == 0) &&
                                 (sp.available == 0),
                                 "a failed query leaves zeroes, not stale numbers");

    items[idx++] = D_ASSERT_EQUAL("missing", d_file_space("no/such/path", &sp), -1,
                                  "d_file_space fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_path", d_file_space(NULL, &sp), -1,
                                  "d_file_space rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_out",
                                  d_file_space(D_TEST_FILE_TEMP_DIR, NULL), -1,
                                  "d_file_space rejects a NULL output");
    #endif

    return d_tests_file_group("d_file_space", items, idx);
}


/*
d_tests_file_space_all
  Runs every file_space test.
*/
struct d_test_object*
d_tests_file_space_all
(
    void
)
{
    struct d_test_object* items[2];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_space_query();

    return d_tests_file_group("XV. file_space", items, idx);
}

#endif  // D_FILE_SPACE_IS_AVAILABLE
