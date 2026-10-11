/*******************************************************************************
* djinterp [tools]                                              test_dsettings.c
*
* Exercises `dsettings.h`.
*
*
* path:      /tests/djinterp/tools/dawk/test_dsettings.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
/* The settings registry's promises: registration refuses a name or alias that
   is already taken; aliases resolve; values validate; a value yields only to a
   source of equal or higher standing, so application order cannot matter. */
#include "../../../../inc/djinterp/tools/dawk/dsettings.h"
#include <stdio.h>
#include <string.h>

static int failures = 0, checks = 0;
#define CHECK(c, what) do { ++checks; if (!(c)) { ++failures; printf("  FAIL  %s\n", what); } } while (0)

int
main(void)
{
    static const char* const modes[]   = { "first", "all", NULL };
    static const char* const unbuilt[] = { "layers", NULL };
    static const struct d_setting_def defs[] = {
        { "fix", D_SETTING_BOOL, "false", NULL, NULL, 0, 0, NULL, "" },
        { "cascade.resolve", D_SETTING_ENUM, "first", modes, unbuilt, 0, 0, "resolve", "" },
        { "report.refusals", D_SETTING_INT, "8", NULL, NULL, 0, 100, NULL, "" } };
    static const struct d_setting_def dup[]   = { { "fix", D_SETTING_BOOL, "true", NULL, NULL, 0, 0, NULL, "" } };
    static const struct d_setting_def clash[] = { { "x", D_SETTING_BOOL, "true", NULL, NULL, 0, 0, "resolve", "" } };
    static const struct d_setting_def bad[]   = { { "y", D_SETTING_INT, "999", NULL, NULL, 0, 10, NULL, "" } };
    char e[256];

    struct d_settings* s = d_settings_new();
    CHECK(d_settings_register(s, defs, 3), "registers");
    CHECK(!d_settings_register(s, dup, 1), "a taken name is refused");
    CHECK(!d_settings_register(s, clash, 1), "a taken alias is refused");
    CHECK(!d_settings_register(s, bad, 1), "a default that fails its own definition is refused");
    CHECK(d_settings_count(s) == 3, "refused registrations add nothing");

    CHECK(d_settings_is(s, "resolve", "first"), "alias reads the real setting");
    CHECK(d_settings_set(s, "resolve", "all", D_SETTING_SHEET, e, sizeof e), "alias writes");
    CHECK(d_settings_is(s, "cascade.resolve", "all"), "...the real setting");

    /* precedence: apply high first, then low -- the low must not win */
    CHECK(d_settings_set(s, "fix", "true", D_SETTING_COMMAND_LINE, e, sizeof e), "cli set");
    CHECK(d_settings_set(s, "fix", "false", D_SETTING_SHEET, e, sizeof e), "a lower source is accepted...");
    CHECK(d_settings_bool(s, "fix"), "...but does not override a higher one");
    CHECK(d_settings_source(s, "fix") == D_SETTING_COMMAND_LINE, "source stays with the winner");
    CHECK(d_settings_set(s, "report.refusals", "3", D_SETTING_ENVIRONMENT, e, sizeof e), "env set");
    CHECK(d_settings_set(s, "report.refusals", "5", D_SETTING_ENVIRONMENT, e, sizeof e), "equal standing");
    CHECK(d_settings_int(s, "report.refusals") == 5, "equal standing overrides");

    CHECK(!d_settings_set(s, "fix", "maybe", D_SETTING_COMMAND_LINE, e, sizeof e), "bad flag refused");
    CHECK(!d_settings_set(s, "report.refusals", "101", D_SETTING_COMMAND_LINE, e, sizeof e), "range enforced");
    CHECK(!d_settings_set(s, "cascade.resolve", "layers", D_SETTING_COMMAND_LINE, e, sizeof e), "unbuilt refused");
    CHECK(strstr(e, "not yet built") != NULL, "...and says so");
    CHECK(!d_settings_set(s, "cascade.reslove", "all", D_SETTING_COMMAND_LINE, e, sizeof e), "unknown refused");
    CHECK(strstr(e, "did you mean 'cascade.resolve'") != NULL, "...with the nearest name");
    CHECK(d_settings_int(s, "report.refusals") == 5, "a refused set changes nothing");

    CHECK(d_settings_set(s, "fix", "off", D_SETTING_COMMAND_LINE, e, sizeof e), "spellings of false");
    CHECK(strcmp(d_settings_text(s, "fix"), "false") == 0, "stored in canonical form");
    CHECK(d_settings_apply(s, "fix=on, report.refusals=2", D_SETTING_COMMAND_LINE, e, sizeof e), "list applies");
    CHECK(d_settings_bool(s, "fix") && d_settings_int(s, "report.refusals") == 2, "list values land");
    CHECK(!d_settings_apply(s, "fix", D_SETTING_COMMAND_LINE, e, sizeof e), "list without = refused");

    /* the shared command-line parser */
    {
        char* argv[] = { "prog", "--fix", "--no-fix", "--set", "report.refusals=7",
                         "--resolve=first", "positional", "--settings", "--", "--bogus=1", NULL };
        int at = 1;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 1 && d_settings_bool(s, "fix"), "--name sets a flag true");
        at = 2;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 1 && !d_settings_bool(s, "fix"), "--no-name sets it false");
        at = 3;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 1 && at == 4 && d_settings_int(s, "report.refusals") == 7, "--set consumes its argument");
        at = 5;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 1 && d_settings_is(s, "cascade.resolve", "first"), "--alias=value");
        at = 6;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 0, "a positional is not taken");
        at = 7;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 0, "--settings is left to the caller");
        at = 8;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == 0, "a lone -- is left to the caller");
        at = 9;
        CHECK(d_settings_take_option(s, 10, argv, &at, e, sizeof e) == -1, "an unknown setting is an error");
    }

    d_settings_free(s);
    printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
