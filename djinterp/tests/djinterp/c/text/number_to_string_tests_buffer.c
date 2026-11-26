/*******************************************************************************
* djinterp [c][text]                             number_to_string_tests_buffer.c
*
* The number_to_string suite's buffer block, and the check every language's
* tables share. All eighteen functions take a buffer as snprintf does: they
* write what fits, always terminated, and return the length of the whole text.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_buffer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header

// std
#include <string.h>  // memcmp, memset


// dj_tests_number_to_string_writer
//   struct: one of the eighteen functions, and its name for a check.
struct dj_tests_number_to_string_writer
{
    const char*                  name;  // the function's name
    dj_tests_number_to_string_fn fn;    // the function
};

// d_tests_number_to_string_writers
//   constant: every function the six headers declare.
static const struct dj_tests_number_to_string_writer
d_tests_number_to_string_writers[] =
{
    { "nts_cardinal_i64",          &nts_cardinal_i64          },
    { "nts_ordinal_i64",           &nts_ordinal_i64           },
    { "nts_ordinal_abbrev_i64",    &nts_ordinal_abbrev_i64    },
    { "nts_es_cardinal_i64",       &nts_es_cardinal_i64       },
    { "nts_es_ordinal_i64",        &nts_es_ordinal_i64        },
    { "nts_es_ordinal_abbrev_i64", &nts_es_ordinal_abbrev_i64 },
    { "nts_fr_cardinal_i64",       &nts_fr_cardinal_i64       },
    { "nts_fr_ordinal_i64",        &nts_fr_ordinal_i64        },
    { "nts_fr_ordinal_abbrev_i64", &nts_fr_ordinal_abbrev_i64 },
    { "nts_it_cardinal_i64",       &nts_it_cardinal_i64       },
    { "nts_it_ordinal_i64",        &nts_it_ordinal_i64        },
    { "nts_it_ordinal_abbrev_i64", &nts_it_ordinal_abbrev_i64 },
    { "nts_la_cardinal_i64",       &nts_la_cardinal_i64       },
    { "nts_la_ordinal_i64",        &nts_la_ordinal_i64        },
    { "nts_la_ordinal_abbrev_i64", &nts_la_ordinal_abbrev_i64 },
    { "nts_pt_cardinal_i64",       &nts_pt_cardinal_i64       },
    { "nts_pt_ordinal_i64",        &nts_pt_ordinal_i64        },
    { "nts_pt_ordinal_abbrev_i64", &nts_pt_ordinal_abbrev_i64 }
};

// D_TESTS_NUMBER_TO_STRING_WRITERS
//   constant: how many functions there are.
#define D_TESTS_NUMBER_TO_STRING_WRITERS                                      \
    (sizeof(d_tests_number_to_string_writers) /                               \
         sizeof(d_tests_number_to_string_writers[0]))


/*
d_tests_number_to_string_rows
  Writes each row's value with its flags through _fn, and checks the text and
the length returned in one comparison, named by the text expected: the length
says how many bytes are compared, so a wrong length is a wrong text.
*/
bool
d_tests_number_to_string_rows(
    dj_tests_number_to_string_fn                _fn,
    const struct dj_tests_number_to_string_row* _rows,
    size_t                                      _count
)
{
    char   text[D_TESTS_NUMBER_TO_STRING_CAPACITY];
    bool   ok = true;
    size_t i;

    for (i = 0u; i < _count; ++i)
    {
        size_t length = _fn(text,
                            sizeof(text),
                            _rows[i].value,
                            _rows[i].flags);

        // a length past the buffer is compared as far as the buffer goes
        if (length >= sizeof(text))
        {
            length = sizeof(text) - 1u;
        }

        ok = d_test_suite_check_text_at(NULL,
                                        _rows[i].expected,
                                        d_test_text_buffer(text, length),
                                        d_test_text_cstr(_rows[i].expected),
                                        __FILE__,
                                        __LINE__) && ok;
    }

    return ok;
}


/*
d_tests_number_to_string_measure
  Measuring.
  Tests the following:
  - each function given NULL and 0 returns the length it writes in full, for
    a short value and for INT64_MIN, the longest
  - a buffer given with a capacity of 0 is not written
*/
bool
d_tests_number_to_string_measure(void)
{
    static const int64_t values[] = { 1234, INT64_MIN };
    char                 text[D_TESTS_NUMBER_TO_STRING_CAPACITY];
    bool                 ok = true;
    size_t               i;
    size_t               j;

    for (i = 0u; i < D_TESTS_NUMBER_TO_STRING_WRITERS; ++i)
    {
        const struct dj_tests_number_to_string_writer* w =
            &d_tests_number_to_string_writers[i];

        for (j = 0u; j < (sizeof(values) / sizeof(values[0])); ++j)
        {
            const size_t full = w->fn(text, sizeof(text), values[j], 0u);
            char         untouched[1] = { 'x' };

            ok = d_test_suite_require( ( (w->fn(NULL, 0u, values[j], 0u) ==
                                          full) &&
                                         (w->fn(untouched,
                                                0u,
                                                values[j],
                                                0u) == full)  &&
                                         (untouched[0] == 'x')         &&
                                         (full > 0u)                   &&
                                         (full < sizeof(text)) ),
                                       w->name,
                                       __FILE__,
                                       __LINE__) && ok;
        }
    }

    return ok;
}

/*
d_tests_number_to_string_cut
  A buffer too short.
  Tests the following:
  - six bytes get the text's first five and a terminator, and the length of
    the whole text is returned
  - one byte gets the terminator alone
*/
bool
d_tests_number_to_string_cut(void)
{
    char   text[D_TESTS_NUMBER_TO_STRING_CAPACITY];
    bool   ok = true;
    size_t i;

    for (i = 0u; i < D_TESTS_NUMBER_TO_STRING_WRITERS; ++i)
    {
        const struct dj_tests_number_to_string_writer* w =
            &d_tests_number_to_string_writers[i];
        const size_t full = w->fn(text, sizeof(text), 1234, 0u);
        char         six[6];
        char         one[1];
        size_t       cut_six;
        size_t       cut_one;

        memset(six, 'x', sizeof(six));
        one[0]  = 'x';
        cut_six = w->fn(six, sizeof(six), 1234, 0u);
        cut_one = w->fn(one, sizeof(one), 1234, 0u);

        // every value here is at least five bytes in every language
        ok = d_test_suite_require( ( (full >= 5u)                     &&
                                     (cut_six == full)                &&
                                     (memcmp(six, text, 5u) == 0)     &&
                                     (six[5] == '\0')                 &&
                                     (cut_one == full)                &&
                                     (one[0] == '\0') ),
                                   w->name,
                                   __FILE__,
                                   __LINE__) && ok;
    }

    return ok;
}
