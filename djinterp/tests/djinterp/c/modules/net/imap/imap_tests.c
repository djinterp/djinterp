/*******************************************************************************
* djinterp [c][net]                                                 imap_tests.c
*
* The section of net's IMAP suite, in DTest's C face: every test, in the
* order run, each a test of its own in module imap.
*
*
* path:      /tests/djinterp/c/modules/net/imap/imap_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./imap_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_imap
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_imap(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_imap_case CASES[] =
    {
        { "errors and backends",      d_tests_imap_errors },
        { "names and lookups",        d_tests_imap_names },
        { "commands and states",      d_tests_imap_command_states },
        { "tags",                     d_tests_imap_tags },
        { "options",                  d_tests_imap_options },
        { "choosing a mechanism",     d_tests_imap_auth_select },
        { "astrings",                 d_tests_imap_astring },
        { "quoted strings",           d_tests_imap_quoted },
        { "literal prefixes",         d_tests_imap_literal },
        { "modified UTF-7",           d_tests_imap_mutf7 },
        { "mailbox names",            d_tests_imap_mailbox },
        { "sinks that refuse",        d_tests_imap_sink_refusals },
        { "capabilities",             d_tests_imap_capabilities },
        { "flags",                    d_tests_imap_flags },
        { "mailbox attributes",       d_tests_imap_mailbox_attributes },
        { "STATUS",                   d_tests_imap_status },
        { "SEARCH",                   d_tests_imap_search },
        { "LIST",                     d_tests_imap_list },
        { "sequence sets",            d_tests_imap_seqset },
        { "dates and times",          d_tests_imap_dates },
        { "parsing dates",            d_tests_imap_date_parsing },
        { "responses",                d_tests_imap_responses },
        { "responses refused",        d_tests_imap_response_refusals },
        { "the lexer",                d_tests_imap_lexer },
        { "input that stops short",   d_tests_imap_lexer_incomplete },
        { "string values",            d_tests_imap_decode },
        { "a client's commands",      d_tests_imap_session_client },
        { "a session over a stream",  d_tests_imap_session_stream }
    };
    bool all = true;

    (void)d_test_suite_module(_suite, "imap");
    (void)d_test_suite_describe(_suite,
                                "IMAP4rev1's client: strings, values, "
                                "responses, options and sessions");

    // each case in order, in a test of its own
    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        (void)d_test_suite_test(_suite,
                                CASES[i].name);

        const bool passed = CASES[i].run(_suite);

        all = (passed) && all;
    }

    return all;
}
