#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_add_alias
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
bool
d_tests_sa_registry_add_alias
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    result = true;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_add_alias(NULL, "k", "a") == false,
        "alias_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);

        row.key = "beta";   row.value = 20;
        d_registry_add(reg, &row);

        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_add_alias(reg, NULL, "a") == false,
            "alias_null_key",
            "NULL key should return false",
            _counter) && result;

        // test 3: NULL alias
        result = d_assert_standalone(
            d_registry_add_alias(reg, "alpha", NULL) == false,
            "alias_null_alias",
            "NULL alias should return false",
            _counter) && result;

        // test 4: key not found
        result = d_assert_standalone(
            d_registry_add_alias(reg, "missing", "m") == false,
            "alias_key_not_found",
            "Non-existent key should return false",
            _counter) && result;

        // test 5: successful add
        result = d_assert_standalone(
            d_registry_add_alias(reg, "alpha", "a") == true,
            "alias_success",
            "Adding alias 'a' for 'alpha' should succeed",
            _counter) && result;

        found = (struct test_row*)d_registry_get(reg, "a");
        result = d_assert_standalone(
            found != NULL && found->value == 10,
            "alias_lookup",
            "Alias 'a' should resolve to alpha (value 10)",
            _counter) && result;

        // test 6: alias already in use (as existing key)
        result = d_assert_standalone(
            d_registry_add_alias(reg, "alpha", "beta") == false,
            "alias_dup_key",
            "Alias that matches existing key should be rejected",
            _counter) && result;

        // test 7: alias already in use (as existing alias)
        result = d_assert_standalone(
            d_registry_add_alias(reg, "beta", "a") == false,
            "alias_dup_alias",
            "Alias that matches existing alias should be rejected",
            _counter) && result;

        // test 8: multiple aliases for same row
        result = d_assert_standalone(
            d_registry_add_alias(reg, "alpha", "A") == true,
            "alias_multi",
            "Second alias 'A' for 'alpha' should succeed",
            _counter) && result;

        found = (struct test_row*)d_registry_get(reg, "A");
        result = d_assert_standalone(
            found != NULL && found->value == 10,
            "alias_multi_lookup",
            "Second alias 'A' should also resolve to alpha",
            _counter) && result;

        // test 9: frozen registry
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_add_alias(reg, "beta", "b") == false,
            "alias_frozen",
            "Frozen registry should reject add_alias",
            _counter) && result;
        d_registry_thaw(reg);

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_add_aliases
  Tests the d_registry_add_aliases function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - NULL aliases array returns false
  - zero count returns false
  - successful batch add of multiple aliases
  - partial failure if one alias is a duplicate
*/
bool
d_tests_sa_registry_add_aliases
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    const char*        aliases[3];

    result = true;

    // test 1: NULL registry
    aliases[0] = "a";
    result = d_assert_standalone(
        d_registry_add_aliases(NULL, "k", aliases, 1) == false,
        "aliases_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);

        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_add_aliases(reg, NULL, aliases, 1) == false,
            "aliases_null_key",
            "NULL key should return false",
            _counter) && result;

        // test 3: NULL aliases array
        result = d_assert_standalone(
            d_registry_add_aliases(reg, "alpha", NULL, 1) == false,
            "aliases_null_arr",
            "NULL aliases array should return false",
            _counter) && result;

        // test 4: zero count
        result = d_assert_standalone(
            d_registry_add_aliases(reg, "alpha", aliases, 0) == false,
            "aliases_zero",
            "Zero count should return false",
            _counter) && result;

        // test 5: successful batch
        aliases[0] = "a";
        aliases[1] = "al";
        aliases[2] = "alp";

        result = d_assert_standalone(
            d_registry_add_aliases(reg, "alpha", aliases, 3) == true,
            "aliases_batch",
            "Batch add of 3 aliases should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_get(reg, "a") != NULL
            && d_registry_get(reg, "al") != NULL
            && d_registry_get(reg, "alp") != NULL,
            "aliases_batch_lookup",
            "All 3 aliases should be findable",
            _counter) && result;

        // test 6: partial failure (duplicate)
        aliases[0] = "new1";
        aliases[1] = "a";     // duplicate
        aliases[2] = "new2";

        result = d_assert_standalone(
            d_registry_add_aliases(reg, "alpha", aliases, 3) == false,
            "aliases_partial_fail",
            "Batch with duplicate should fail",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_remove_alias
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
bool
d_tests_sa_registry_remove_alias
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_remove_alias(NULL, "a") == false,
        "rm_alias_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);
        d_registry_add_alias(reg, "alpha", "a");
        d_registry_add_alias(reg, "alpha", "al");

        // test 2: NULL alias
        result = d_assert_standalone(
            d_registry_remove_alias(reg, NULL) == false,
            "rm_alias_null_alias",
            "NULL alias should return false",
            _counter) && result;

        // test 3: alias not found
        result = d_assert_standalone(
            d_registry_remove_alias(reg, "missing") == false,
            "rm_alias_not_found",
            "Non-existent alias should return false",
            _counter) && result;

        // test 4: removing canonical key should fail
        result = d_assert_standalone(
            d_registry_remove_alias(reg, "alpha") == false,
            "rm_alias_canonical",
            "Removing canonical key via remove_alias should fail",
            _counter) && result;

        // test 5: successful removal
        result = d_assert_standalone(
            d_registry_remove_alias(reg, "a") == true,
            "rm_alias_success",
            "Removing alias 'a' should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_get(reg, "a") == NULL,
            "rm_alias_gone",
            "Removed alias 'a' should not be findable",
            _counter) && result;

        // test 6: canonical key still works
        result = d_assert_standalone(
            d_registry_get(reg, "alpha") != NULL,
            "rm_alias_canon_ok",
            "Canonical key 'alpha' should still work",
            _counter) && result;

        // test 7: other alias still works
        result = d_assert_standalone(
            d_registry_get(reg, "al") != NULL,
            "rm_alias_other_ok",
            "Other alias 'al' should still work",
            _counter) && result;

        // test 8: frozen
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_remove_alias(reg, "al") == false,
            "rm_alias_frozen",
            "Frozen registry should reject remove_alias",
            _counter) && result;
        d_registry_thaw(reg);

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_clear_aliases
  Tests the d_registry_clear_aliases function.
  Tests the following:
  - NULL registry does not crash
  - frozen registry is a no-op
  - removes all alias entries, keeps canonical entries
  - canonical keys still work after clear
  - alias count becomes zero
*/
bool
d_tests_sa_registry_clear_aliases
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL does not crash
    d_registry_clear_aliases(NULL);
    result = d_assert_standalone(
        true,
        "clr_aliases_null",
        "NULL registry clear_aliases should not crash",
        _counter) && result;

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
        result = d_assert_standalone(
            d_registry_alias_count(reg) == 3,
            "clr_aliases_before",
            "Should have 3 aliases before clear",
            _counter) && result;

        // test 2: clear aliases
        d_registry_clear_aliases(reg);

        result = d_assert_standalone(
            d_registry_alias_count(reg) == 0,
            "clr_aliases_count",
            "Alias count should be 0 after clear",
            _counter) && result;

        // test 3: canonical keys still work
        result = d_assert_standalone(
            d_registry_get(reg, "alpha") != NULL
            && d_registry_get(reg, "beta") != NULL,
            "clr_aliases_canon_ok",
            "Canonical keys should still work",
            _counter) && result;

        // test 4: aliases no longer work
        result = d_assert_standalone(
            d_registry_get(reg, "a") == NULL
            && d_registry_get(reg, "b") == NULL,
            "clr_aliases_gone",
            "Aliases should no longer be findable",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_alias_count
  Tests the d_registry_alias_count function.
  Tests the following:
  - NULL registry returns 0
  - empty registry returns 0
  - registry with no aliases returns 0
  - counts only non-canonical entries
  - count updates after adding and removing aliases
*/
bool
d_tests_sa_registry_alias_count
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_alias_count(NULL) == 0,
        "alias_cnt_null",
        "NULL registry should return 0",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty registry
        result = d_assert_standalone(
            d_registry_alias_count(reg) == 0,
            "alias_cnt_empty",
            "Empty registry should return 0",
            _counter) && result;

        // add rows (no aliases yet)
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);

        // test 3: no aliases
        result = d_assert_standalone(
            d_registry_alias_count(reg) == 0,
            "alias_cnt_no_aliases",
            "Registry with rows but no aliases should return 0",
            _counter) && result;

        // add aliases
        d_registry_add_alias(reg, "alpha", "a");
        d_registry_add_alias(reg, "beta", "b");
        d_registry_add_alias(reg, "beta", "B");

        // test 4: correct count
        result = d_assert_standalone(
            d_registry_alias_count(reg) == 3,
            "alias_cnt_three",
            "Should count 3 aliases",
            _counter) && result;

        // test 5: after removing one
        d_registry_remove_alias(reg, "b");
        result = d_assert_standalone(
            d_registry_alias_count(reg) == 2,
            "alias_cnt_after_remove",
            "Should count 2 aliases after removing one",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_aliases_all
  Aggregation function that runs all alias tests.
*/
bool
d_tests_sa_registry_aliases_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Alias Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_registry_add_alias(_counter) && result;
    result = d_tests_sa_registry_add_aliases(_counter) && result;
    result = d_tests_sa_registry_remove_alias(_counter) && result;
    result = d_tests_sa_registry_clear_aliases(_counter) && result;
    result = d_tests_sa_registry_alias_count(_counter) && result;

    return result;
}
