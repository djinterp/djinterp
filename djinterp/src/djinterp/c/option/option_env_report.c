/*******************************************************************************
* djinterp [c][option]                                       option_env_report.c
*
*   The stock report (option_env.h): one line through djinterp's stream for
* each value an import could not bind, "DJINTERP_X=maybe: not a bool",
* worded from the variable's row.
*
*   IT WRITES THROUGH THE STREAM, NEVER A FILE*. Its context is a struct
* dj_ostream*, so a file, a buffer and a module's host take the same bytes;
* null is standard error, through the stream's console, as the log's is
* (option_env's decision 86). A program with a FILE* wraps it with
* d_ostream_init_file. So option_env.c needs no FILE, and a build whose C
* library has none still compiles it.
*
*   IT IS A UNIT OF ITS OWN. The stream's console is below option_env, but a
* program that never reports links neither this unit nor the console; one
* that builds with D_CFG_OPTION_ENV_IMPORT_REPORT 1, or passes this report
* to an import, links both.
*
*
* path:      /src/djinterp/c/option/option_env_report.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.25
*                                                            revised: 2026.11.25
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_env.h"  // corresponding
                                                           // header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // the stream
#include "../../../../inc/djinterp/c/util/ostream/ostream_console.h"  // stderr
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, uint32_t


#if RE_STD_HAS_INT64

// D_INTERNAL_OPTION_ENV_REPORT_HEAD_MAX
//   constant: the longest first part of a setting's name the report names as
// a djin's; a longer one is no djin's name (option_env.c's name limit).
#define D_INTERNAL_OPTION_ENV_REPORT_HEAD_MAX  63u


/*
d_internal_option_env_report_text
  Counted text to the stream, each control byte as \xNN, so a value read from
the environment cannot move a terminal's cursor or retitle its window.
*/
static void
d_internal_option_env_report_text(
    struct dj_ostream* _os,
    const char*        _text,
    size_t             _size
)
{
    static const char HEX[] = "0123456789ABCDEF";
    size_t            i;

    for (i = 0u; (_text != NULL) && (i < _size); ++i)
    {
        const unsigned char c = (unsigned char)_text[i];

        if ( (c < 0x20u) || (c == 0x7Fu) )
        {
            (void)d_ostream_write_cstr(_os, "\\x");
            (void)d_ostream_write_char(_os, HEX[c >> 4]);
            (void)d_ostream_write_char(_os, HEX[c & 0x0Fu]);
        }
        else
        {
            (void)d_ostream_write_char(_os, (char)c);
        }
    }

    return;
}

/*
d_internal_option_env_report_cstr
  A NUL-terminated text, as d_internal_option_env_report_text writes it;
nothing for null.
*/
static void
d_internal_option_env_report_cstr(
    struct dj_ostream* _os,
    const char*        _text
)
{
    if (_text != NULL)
    {
        d_internal_option_env_report_text(_os, _text, strlen(_text));
    }

    return;
}

/*
d_internal_option_env_report_djin
  "no djin named net", for a setting's full name, net.timeout; the reason's
stock words when the name has no first part, or one no djin's name can be.
*/
static void
d_internal_option_env_report_djin(
    struct dj_ostream* _os,
    const char*        _setting
)
{
    const char*  dot  = (_setting != NULL) ? strchr(_setting, '.') : NULL;
    const size_t size = (dot != NULL) ? (size_t)(dot - _setting) : 0u;

    if ( (size == 0u) ||
         (size > D_INTERNAL_OPTION_ENV_REPORT_HEAD_MAX) )
    {
        (void)d_ostream_write_cstr(
                  _os,
                  d_option_env_reason_name((int32_t)D_OPTION_ENV_REASON_FRAME));

        return;
    }

    (void)d_ostream_write_cstr(_os, "no djin named ");
    d_internal_option_env_report_text(_os, _setting, size);

    return;
}

/*
d_internal_option_env_report_names
  A row's names after a lead: the first after _first, each next after _next.
*/
static void
d_internal_option_env_report_names(
    struct dj_ostream*              _os,
    const struct dj_option_env_var* _decl,
    const char*                     _first,
    const char*                     _next
)
{
    uint32_t i;

    for (i = 0u; i < _decl->name_count; ++i)
    {
        (void)d_ostream_write_cstr(_os, (i == 0u) ? _first : _next);
        (void)d_ostream_write_cstr(_os, _decl->names[i].name);
    }

    return;
}

/*
d_internal_option_env_report_why
  A reason in the row's own terms: the kind a text is not, the range and the
names an int takes, a choice's names, the source refused.
*/
static void
d_internal_option_env_report_why(
    struct dj_ostream*                 _os,
    const struct dj_option_env_report* _report,
    const struct dj_option_env_var*    _decl
)
{
    // by D_OPTION_ENV_KIND_*, from BOOL, 1, to FLAGS, 6
    static const char* const KINDS[] =
    {
        "readable", "a bool", "an int", "a real", "text",
        "one of its names", "a set of its flags"
    };
    static const char* const SOURCES[] =
    {
        "the default", "the program", "a settings file", "the environment",
        "the command line", "an override"
    };

    // without the row, every reason is said in its stock words
    switch ( ( (_decl != NULL) ||
               (_report->reason == (int32_t)D_OPTION_ENV_REASON_SOURCE) ||
               (_report->reason == (int32_t)D_OPTION_ENV_REASON_FRAME) )
                 ? _report->reason : -1 )
    {
        case D_OPTION_ENV_REASON_FRAME:
            d_internal_option_env_report_djin(_os, _report->setting);
            break;
        case D_OPTION_ENV_REASON_TEXT:
            (void)d_ostream_write_cstr(_os, "not ");
            (void)d_ostream_write_cstr(
                      _os,
                      ( (_decl->kind >= (int32_t)D_OPTION_ENV_KIND_BOOL) &&
                        (_decl->kind <= (int32_t)D_OPTION_ENV_KIND_FLAGS) )
                          ? KINDS[_decl->kind] : KINDS[0]);
            break;
        case D_OPTION_ENV_REASON_RANGE:
            (void)d_ostream_write_cstr(_os, "not from ");
            (void)d_ostream_write_int64(_os, _decl->minimum);
            (void)d_ostream_write_cstr(_os, " to ");
            (void)d_ostream_write_int64(_os, _decl->maximum);
            d_internal_option_env_report_names(_os, _decl, ", nor ", " or ");
            break;
        case D_OPTION_ENV_REASON_CHOICE:
            (void)d_ostream_write_cstr(_os, "not one of ");
            d_internal_option_env_report_names(_os, _decl, "", ", ");
            break;
        case D_OPTION_ENV_REASON_SOURCE:
            (void)d_ostream_write_cstr(_os, "not taken from ");
            (void)d_ostream_write_cstr(_os,
                                       ( (_report->source >= 0) &&
                                         (_report->source <= 5) )
                                           ? SOURCES[_report->source]
                                           : "this source");
            (void)d_ostream_write_cstr(_os, " in this program");
            break;
        default:
            (void)d_ostream_write_cstr(_os,
                                       d_option_env_reason_name(
                                           _report->reason));
            break;
    }

    return;
}

/*
d_internal_option_env_report_line
  "VARIABLE=value: why" and a newline, to the stream.
*/
static void
d_internal_option_env_report_line(
    struct dj_ostream*                 _os,
    const struct dj_option_env_report* _report
)
{
    d_internal_option_env_report_cstr(_os, _report->variable);
    (void)d_ostream_write_char(_os, '=');
    d_internal_option_env_report_cstr(_os, _report->value);
    (void)d_ostream_write_cstr(_os, ": ");
    d_internal_option_env_report_why(_os, _report,
                                     (_report->frame != NULL)
                                         ? d_option_env_var_at(_report->frame,
                                                               _report->var)
                                         : NULL);
    (void)d_ostream_write_char(_os, '\n');

    return;
}

/*
d_option_env_report_write
  A line for each refusal, to the caller's stream; with none, to standard
error through a console stream made for the line and closed after it. A
record too old to carry its reason is skipped.
*/
void
d_option_env_report_write(
    void*                              _context,
    const struct dj_option_env_report* _report
)
{
    struct dj_ostream         console_os;
    struct dj_ostream_console console;

    if ( (_report == NULL) ||
         (_report->size < (uint32_t)sizeof(struct dj_option_env_report)) )
    {
        return;
    }

    if (_context != NULL)
    {
        d_internal_option_env_report_line((struct dj_ostream*)_context,
                                          _report);

        return;
    }

    // refused without a console, a GUI process's say: the line goes nowhere
    if (d_ostream_init_console(&console_os, &console,
                               D_OSTREAM_CONSOLE_ERR, 0) == 0)
    {
        return;
    }

    d_internal_option_env_report_line(&console_os, _report);
    (void)d_ostream_close(&console_os);

    return;
}


#endif  // RE_STD_HAS_INT64
