/*******************************************************************************
* djinterp [c][env]                                          env_tests_profile.c
*
*   The build profile env's suite notes on its module: the report
* env_printer.h's d_env_report_here collects for this unit, every entry once,
* as `section.name=value` lines in the printer's keyvalue form, so a profile
* compares line by line with another build's, and matches what
* d_env_print_file(f, "style=keyvalue; show_title=0; sections=all") prints
* from a unit compiled the same way.
*   The note is the module's: the report prints it when an env test fails, and
* at --checks. Nothing is linked for it: the report is constant tables the
* header compiles into this unit, and the lines are formatted here, so env's
* suite needs none of the printer's machinery.
*
*
* path:      /tests/djinterp/c/env_tests_profile.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.07
*                                                            revised: 2026.11.07
*******************************************************************************/
// djinterp
#include "./env_tests.h"  // the suite's declarations, DTest's C face
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, free
// djinterp
#include "../../../inc/djinterp/env/env_printer.h"  // d_env_report_here


/*
d_internal_tests_env_profile_line
  Appends one formatted line at _used, as snprintf does: it writes what fits
of the _size bytes, and says how long the line is either way. Returns the new
length, or (size_t)-1 when snprintf refuses the line.
*/
static size_t
d_internal_tests_env_profile_line(
    char*       _buffer,
    size_t      _size,
    size_t      _used,
    const char* _slug,
    const char* _name,
    const char* _text,
    long        _number
)
{
    char*  at;
    size_t room;
    int    written;

    at   = (_used < _size) ? (_buffer + _used) : NULL;
    room = (_used < _size) ? (_size - _used)   : 0;

    if (_name == NULL)
    {
        written = snprintf(at, room, "# %s: %s\n", _slug, _text);
    }
    else if (_text != NULL)
    {
        written = snprintf(at, room, "%s.%s=%s\n", _slug, _name, _text);
    }
    else
    {
        written = snprintf(at, room, "%s.%s=%ld\n", _slug, _name, _number);
    }

    if (written < 0)
    {
        return (size_t)-1;
    }

    return _used + (size_t)written;
}

/*
d_internal_tests_env_profile_write
  Writes a report into _buffer as snprintf would: at most _size bytes, ended
with a NUL when _size is not 0. Returns the length the whole profile needs,
or (size_t)-1 when a line cannot be formatted. A section with no entries
writes its note, as the keyvalue style does.
*/
static size_t
d_internal_tests_env_profile_write(
    const struct dj_env_report* _report,
    char*                       _buffer,
    size_t                      _size
)
{
    const struct dj_env_section* section;
    const struct dj_env_entry*   entry;
    const char*                  text;
    size_t                       used;
    size_t                       s;
    size_t                       e;

    used = 0;

    if (_size != 0)
    {
        _buffer[0] = '\0';
    }

    for (s = 0; (s < _report->count) && (used != (size_t)-1); s++)
    {
        section = &_report->sections[s];

        if ( (section->entry_count == 0) &&
             (section->note != NULL) )
        {
            used = d_internal_tests_env_profile_line(_buffer, _size, used,
                                                     section->slug, NULL,
                                                     section->note, 0L);
        }

        for (e = 0; (e < section->entry_count) && (used != (size_t)-1); e++)
        {
            entry = &section->entries[e];
            text  = (entry->kind == D_ENV_VALUE_TEXT)
                        ? ( (entry->text) ? entry->text : "" )
                    : (entry->kind == D_ENV_VALUE_BOOLEAN)
                        ? ( (entry->integer) ? "true" : "false" )
                        : NULL;
            used  = d_internal_tests_env_profile_line(_buffer, _size, used,
                                                      section->slug,
                                                      entry->name, text,
                                                      entry->integer);
        }
    }

    return used;
}

/*
d_tests_env_profile
  Measures the profile, then writes it into a buffer of that size, so nothing
is cut. A profile that cannot be made is itself the note, since the suite's
checks do not depend on it.
*/
bool
d_tests_env_profile(
    struct dj_test_suite* _suite
)
{
    const struct dj_env_report* report;
    char*                       text;
    size_t                      length;
    bool                        noted;

    report = d_env_report_here();
    length = d_internal_tests_env_profile_write(report, NULL, 0);

    if (length == (size_t)-1)
    {
        return d_test_suite_note(_suite,
                                 "build profile: a line could not be "
                                 "formatted");
    }

    text = (char*)malloc(length + 1);

    if (!text)
    {
        return d_test_suite_note(_suite,
                                 "build profile: no memory for %lu bytes",
                                 (unsigned long)(length + 1));
    }

    (void)d_internal_tests_env_profile_write(report, text, length + 1);
    noted = d_test_suite_note(_suite,
                              "build profile (d_env_report_here):\n%s",
                              text);
    free(text);

    return noted;
}
