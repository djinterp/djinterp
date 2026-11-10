/*******************************************************************************
* djinterp [container]                                  registry_tests_aliases.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_aliases.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_add_alias
  Tests the d_registry_add_alias function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - NULL alias returns false
  - key not found returns false
  - frozen registry returns false
  - static registry returns false
  - alias string already in use returns false
  - successful add makes row findable by alias
  - multiple aliases can point to the same row
  - lookup grows as aliases are added
*/
bool d_tests_registry_add_alias(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_add_alias");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_null_reg",
                              d_registry_add_alias(NULL, "k", "a") == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);

        row.key = "beta";   row.value = 20;
        d_registry_add(reg, &row);

        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_null_key",
                                  d_registry_add_alias(reg, NULL, "a") == false,
                                  "NULL key should return false") && result;

        // test 3: NULL alias
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_null_alias",
                                  d_registry_add_alias(reg, "alpha", NULL) == false,
                                  "NULL alias should return false") && result;

        // test 4: key not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_key_not_found",
                                  d_registry_add_alias(reg, "missing", "m") == false,
                                  "Non-existent key should return false") && result;

        // test 5: successful add
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_success",
                                  d_registry_add_alias(reg, "alpha", "a") == true,
                                  "Adding alias 'a' for 'alpha' should succeed") && result;

        found = (struct test_row*)d_registry_get(reg, "a");
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_lookup",
                                  found != NULL && found->value == 10,
                                  "Alias 'a' should resolve to alpha (value 10)") && result;

        // test 6: alias already in use (as existing key)
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_dup_key",
                                  d_registry_add_alias(reg, "alpha", "beta") == false,
                                  "Alias that matches existing key should be rejected") && result;

        // test 7: alias already in use (as existing alias)
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_dup_alias",
                                  d_registry_add_alias(reg, "beta", "a") == false,
                                  "Alias that matches existing alias should be rejected") && result;

        // test 8: multiple aliases for same row
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_multi",
                                  d_registry_add_alias(reg, "alpha", "A") == true,
                                  "Second alias 'A' for 'alpha' should succeed") && result;

        found = (struct test_row*)d_registry_get(reg, "A");
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_multi_lookup",
                                  found != NULL && found->value == 10,
                                  "Second alias 'A' should also resolve to alpha") && result;

        // test 9: frozen registry
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_frozen",
                                  d_registry_add_alias(reg, "beta", "b") == false,
                                  "Frozen registry should reject add_alias") && result;
        d_registry_thaw(reg);

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_add_aliases
  Tests the d_registry_add_aliases function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - NULL aliases array returns false
  - zero count returns false
  - successful batch add of multiple aliases
  - partial failure if one alias is a duplicate
*/
bool d_tests_registry_add_aliases(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    const char*        aliases[3];

    d_test_suite_test(_suite, "registry_add_aliases");

    result = true;

    // test 1: NULL registry
    aliases[0] = "a";
    result = D_TEST_CHECK_MSG(_suite,
                              "aliases_null_reg",
                              d_registry_add_aliases(NULL, "k", aliases, 1) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);

        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "aliases_null_key",
                                  d_registry_add_aliases(reg, NULL, aliases, 1) == false,
                                  "NULL key should return false") && result;

        // test 3: NULL aliases array
        result = D_TEST_CHECK_MSG(_suite,
                                  "aliases_null_arr",
                                  d_registry_add_aliases(reg, "alpha", NULL, 1) == false,
                                  "NULL aliases array should return false") && result;

        // test 4: zero count
        result = D_TEST_CHECK_MSG(_suite,
                                  "aliases_zero",
                                  d_registry_add_aliases(reg, "alpha", aliases, 0) == false,
                                  "Zero count should return false") && result;

        // test 5: successful batch
        aliases[0] = "a";
        aliases[1] = "al";
        aliases[2] = "alp";

        result = D_TEST_CHECK_MSG(_suite,
                                  "aliases_batch",
                                  d_registry_add_aliases(reg, "alpha", aliases, 3) == true,
                                  "Batch add of 3 aliases should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "aliases_batch_lookup",
                                  d_registry_get(reg, "a") != NULL && d_registry_get(reg, "al") != NULL && d_registry_get(reg, "alp") != NULL,
                                  "All 3 aliases should be findable") && result;

        // test 6: partial failure (duplicate)
        aliases[0] = "new1";
        aliases[1] = "a";     // duplicate
        aliases[2] = "new2";

        result = D_TEST_CHECK_MSG(_suite,
                                  "aliases_partial_fail",
                                  d_registry_add_aliases(reg, "alpha", aliases, 3) == false,
                                  "Batch with duplicate should fail") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_remove_alias
  Tests the d_registry_remove_alias function.
  Tests the following:
  - NULL registry returns false
  - NULL alias returns false
  - frozen registry returns false
  - static registry returns false
  - alias not found returns false
  - removing a canonical key returns false (only aliases removable)
  - successful removal makes alias unfindable
  - canonical key still works after alias removal
*/
bool d_tests_registry_remove_alias(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_remove_alias");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "rm_alias_null_reg",
                              d_registry_remove_alias(NULL, "a") == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);
        d_registry_add_alias(reg, "alpha", "a");
        d_registry_add_alias(reg, "alpha", "al");

        // test 2: NULL alias
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_null_alias",
                                  d_registry_remove_alias(reg, NULL) == false,
                                  "NULL alias should return false") && result;

        // test 3: alias not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_not_found",
                                  d_registry_remove_alias(reg, "missing") == false,
                                  "Non-existent alias should return false") && result;

        // test 4: removing canonical key should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_canonical",
                                  d_registry_remove_alias(reg, "alpha") == false,
                                  "Removing canonical key via remove_alias should fail") && result;

        // test 5: successful removal
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_success",
                                  d_registry_remove_alias(reg, "a") == true,
                                  "Removing alias 'a' should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_gone",
                                  d_registry_get(reg, "a") == NULL,
                                  "Removed alias 'a' should not be findable") && result;

        // test 6: canonical key still works
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_canon_ok",
                                  d_registry_get(reg, "alpha") != NULL,
                                  "Canonical key 'alpha' should still work") && result;

        // test 7: other alias still works
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_other_ok",
                                  d_registry_get(reg, "al") != NULL,
                                  "Other alias 'al' should still work") && result;

        // test 8: frozen
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rm_alias_frozen",
                                  d_registry_remove_alias(reg, "al") == false,
                                  "Frozen registry should reject remove_alias") && result;
        d_registry_thaw(reg);

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_clear_aliases
  Tests the d_registry_clear_aliases function.
  Tests the following:
  - NULL registry does not crash
  - frozen registry is a no-op
  - removes all alias entries, keeps canonical entries
  - canonical keys still work after clear
  - alias count becomes zero
*/
bool d_tests_registry_clear_aliases(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_clear_aliases");

    result = true;

    // test 1: NULL does not crash
    d_registry_clear_aliases(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "clr_aliases_null",
                              true,
                              "NULL registry clear_aliases should not crash") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);

        d_registry_add_alias(reg, "alpha", "a");
        d_registry_add_alias(reg, "alpha", "al");
        d_registry_add_alias(reg, "beta", "b");

        // verify aliases exist
        result = D_TEST_CHECK_MSG(_suite,
                                  "clr_aliases_before",
                                  d_registry_alias_count(reg) == 3,
                                  "Should have 3 aliases before clear") && result;

        // test 2: clear aliases
        d_registry_clear_aliases(reg);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clr_aliases_count",
                                  d_registry_alias_count(reg) == 0,
                                  "Alias count should be 0 after clear") && result;

        // test 3: canonical keys still work
        result = D_TEST_CHECK_MSG(_suite,
                                  "clr_aliases_canon_ok",
                                  d_registry_get(reg, "alpha") != NULL && d_registry_get(reg, "beta") != NULL,
                                  "Canonical keys should still work") && result;

        // test 4: aliases no longer work
        result = D_TEST_CHECK_MSG(_suite,
                                  "clr_aliases_gone",
                                  d_registry_get(reg, "a") == NULL && d_registry_get(reg, "b") == NULL,
                                  "Aliases should no longer be findable") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_alias_count
  Tests the d_registry_alias_count function.
  Tests the following:
  - NULL registry returns 0
  - empty registry returns 0
  - registry with no aliases returns 0
  - counts only non-canonical entries
  - count updates after adding and removing aliases
*/
bool d_tests_registry_alias_count(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_alias_count");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "alias_cnt_null",
                              d_registry_alias_count(NULL) == 0,
                              "NULL registry should return 0") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty registry
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_cnt_empty",
                                  d_registry_alias_count(reg) == 0,
                                  "Empty registry should return 0") && result;

        // add rows (no aliases yet)
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);

        // test 3: no aliases
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_cnt_no_aliases",
                                  d_registry_alias_count(reg) == 0,
                                  "Registry with rows but no aliases should return 0") && result;

        // add aliases
        d_registry_add_alias(reg, "alpha", "a");
        d_registry_add_alias(reg, "beta", "b");
        d_registry_add_alias(reg, "beta", "B");

        // test 4: correct count
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_cnt_three",
                                  d_registry_alias_count(reg) == 3,
                                  "Should count 3 aliases") && result;

        // test 5: after removing one
        d_registry_remove_alias(reg, "b");
        result = D_TEST_CHECK_MSG(_suite,
                                  "alias_cnt_after_remove",
                                  d_registry_alias_count(reg) == 2,
                                  "Should count 2 aliases after removing one") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_aliases_all
  Aggregation function that runs all alias tests.
*/
bool d_tests_registry_aliases_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_aliases_all");

    result = true;

    result = d_tests_registry_add_alias(_suite) && result;
    result = d_tests_registry_add_aliases(_suite) && result;
    result = d_tests_registry_remove_alias(_suite) && result;
    result = d_tests_registry_clear_aliases(_suite) && result;
    result = d_tests_registry_alias_count(_suite) && result;

    return result;
}
