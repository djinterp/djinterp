/*******************************************************************************
* djinterp [net]                                            imap_tests_sa_main.c
*
* The IMAP kernel's standalone suite: its runner and entry point.
*   Runs every test and exits 0 only if each passed.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


/*
d_tests_sa_imap_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_imap_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_imap_case CASES[] =
    {
        { "errors and backends",      d_tests_sa_imap_errors },
        { "names and lookups",        d_tests_sa_imap_names },
        { "commands and states",      d_tests_sa_imap_command_states },
        { "tags",                     d_tests_sa_imap_tags },
        { "options",                  d_tests_sa_imap_options },
        { "choosing a mechanism",     d_tests_sa_imap_auth_select },
        { "astrings",                 d_tests_sa_imap_astring },
        { "quoted strings",           d_tests_sa_imap_quoted },
        { "literal prefixes",         d_tests_sa_imap_literal },
        { "modified UTF-7",           d_tests_sa_imap_mutf7 },
        { "mailbox names",            d_tests_sa_imap_mailbox },
        { "sinks that refuse",        d_tests_sa_imap_sink_refusals },
        { "capabilities",             d_tests_sa_imap_capabilities },
        { "flags",                    d_tests_sa_imap_flags },
        { "mailbox attributes",       d_tests_sa_imap_mailbox_attributes },
        { "STATUS",                   d_tests_sa_imap_status },
        { "SEARCH",                   d_tests_sa_imap_search },
        { "LIST",                     d_tests_sa_imap_list },
        { "sequence sets",            d_tests_sa_imap_seqset },
        { "dates and times",          d_tests_sa_imap_dates },
        { "parsing dates",            d_tests_sa_imap_date_parsing },
        { "responses",                d_tests_sa_imap_responses },
        { "responses refused",        d_tests_sa_imap_response_refusals },
        { "the lexer",                d_tests_sa_imap_lexer },
        { "input that stops short",   d_tests_sa_imap_lexer_incomplete },
        { "string values",            d_tests_sa_imap_decode },
        { "a client's commands",      d_tests_sa_imap_session_client },
        { "a session over a stream",  d_tests_sa_imap_session_stream }
    };
    bool all = true;

    // each case in order, counted and reported
    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        const bool passed = CASES[i].run(_counter);

        _counter->tests_total  += 1u;
        _counter->tests_passed += (passed) ? 1u : 0u;
        printf("  %s  %s\n",
               (passed) ? "pass" : "FAIL",
               CASES[i].name);
        all = (passed) && all;
    }

    return all;
}

/*
main
  Runs every test and prints tests before assertions, as the other suites
do.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] imap standalone tests\n");

    const bool passed = d_tests_sa_imap_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
