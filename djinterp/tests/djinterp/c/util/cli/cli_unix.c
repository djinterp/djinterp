/******************************************************************************
* djinterp                                                         cli_unix.c
*
*   Implementation of Unix CLI convention parsing.
*
*
* path:      /src/c/util/cli_unix.c
* link(s):   cli_option.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.23
******************************************************************************/

#include "../../../inc/c/util/cli_unix.h"


/******************************************************************************
 * INTERNAL CONSTANTS
 *****************************************************************************/

#define CLI_UNIX_KEY_MAX 256


/******************************************************************************
 * d_cli_unix_parse
 *****************************************************************************/

/*
d_cli_unix_parse
  Parses argc/argv using Unix conventions against a d_string_cli_entry
table. Delegates all lookup to d_cli_find from cli_option.h.

Processing rules for each argv element (starting from index 1):

  "--"
    Bare stop token. All subsequent argv elements are ignored.

  "--key"
    Strip "--", look up "key" in the table via d_cli_find.
    Handler receives key_length = strlen of the stripped token,
    next_arg = argv[i+1] (or NULL).
    If handler returns > 0, skip that many additional argv elements.

  "--key=val"
    Strip "--", split on the first '='. Look up "key".
    Handler receives key_length = length up to '=',
    next_arg = "val" (the portion after '=').
    Handler return does NOT advance argv (value was embedded).

  "-abc"
    Strip "-", expand each character into a 1-char key string.
    For each non-last character: look up, call handler with
    key_length = 1, next_arg = NULL.
    For the last character: look up, call handler with
    key_length = 1, next_arg = argv[i+1].
    If handler returns > 0, skip that many additional argv elements.

  anything else
    Skipped (treated as a positional argument).

Unrecognized keys (no match in table) are silently skipped.
If _on_match is NULL, all matches are silently consumed.

Parameter(s):
  _entries:  array of d_string_cli_entry structs (bare keys, no operators)
  _count:    number of entries
  _argc:     argument count from main
  _argv:     argument vector from main
  _on_match: handler called per match; NULL = silent consume
  _context:  opaque pointer passed to _on_match
Return:
  true if parsing completed, false if a handler stopped it.
*/
bool
d_cli_unix_parse
(
    const struct d_string_cli_entry* _entries,
    size_t                            _count,
    int                               _argc,
    char*                             _argv[],
    fn_cli_unix_match                 _on_match,
    void*                             _context
)
{
    int         i;
    char*       arg;
    size_t      arg_len;
    void*       value;
    const char* next_arg;
    int         handler_rc;

    if ( (!_entries) ||
         (!_argv)    ||
         (_count == 0) )
    {
        return true;
    }

    for (i = 1; i < _argc; i++)
    {
        arg = _argv[i];

        if (!arg)
        {
            continue;
        }

        arg_len = strlen(arg);

        // ---------------------------------------------------------------
        // "--" bare stop token
        // ---------------------------------------------------------------
        if ( (arg_len == 2)  &&
             (arg[0] == '-') &&
             (arg[1] == '-') )
        {
            break;
        }

        // ---------------------------------------------------------------
        // "--key" or "--key=val" long option
        // ---------------------------------------------------------------
        if ( (arg_len > 2)   &&
             (arg[0] == '-') &&
             (arg[1] == '-') )
        {
            const char* token;
            const char* eq;

            token = arg + 2;
            eq    = strchr(token, '=');

            if (eq)
            {
                // --key=val: extract key portion
                char   key_buf[CLI_UNIX_KEY_MAX];
                size_t key_len;

                key_len = (size_t)(eq - token);

                if (key_len >= CLI_UNIX_KEY_MAX)
                {
                    continue;
                }

                memcpy(key_buf, token, key_len);
                key_buf[key_len] = '\0';

                value = d_cli_find(_entries, _count,
                                   key_buf, key_len);

                if ( (value) && (_on_match) )
                {
                    handler_rc = _on_match(key_buf, key_len,
                                           value,
                                           eq + 1, _context);

                    if (handler_rc < 0)
                    {
                        return false;
                    }

                    // embedded value: do NOT advance argv
                }
            }
            else
            {
                // --key: plain long option
                size_t token_len;

                token_len = arg_len - 2;

                value = d_cli_find(_entries, _count,
                                   token, token_len);

                if ( (value) && (_on_match) )
                {
                    next_arg = ((i + 1) < _argc)
                                   ? _argv[i + 1]
                                   : NULL;

                    handler_rc = _on_match(token, token_len,
                                           value,
                                           next_arg, _context);

                    if (handler_rc < 0)
                    {
                        return false;
                    }

                    if (handler_rc > 0)
                    {
                        i += handler_rc;
                    }
                }
            }

            continue;
        }

        // ---------------------------------------------------------------
        // "-abc" short flags (one or more characters)
        // ---------------------------------------------------------------
        if ( (arg_len > 1)   &&
             (arg[0] == '-') )
        {
            const char* chars;
            size_t      j;
            size_t      chars_len;
            char        key_buf[2];
            bool        is_last;

            chars     = arg + 1;
            chars_len = arg_len - 1;

            for (j = 0; j < chars_len; j++)
            {
                key_buf[0] = chars[j];
                key_buf[1] = '\0';
                is_last    = (j + 1 == chars_len);

                value = d_cli_find(_entries, _count,
                                   key_buf, 1);

                if ( (!value) || (!_on_match) )
                {
                    continue;
                }

                if (is_last)
                {
                    next_arg = ((i + 1) < _argc)
                                   ? _argv[i + 1]
                                   : NULL;
                }
                else
                {
                    next_arg = NULL;
                }

                handler_rc = _on_match(key_buf, 1,
                                       value,
                                       next_arg, _context);

                if (handler_rc < 0)
                {
                    return false;
                }

                if ( (is_last) && (handler_rc > 0) )
                {
                    i += handler_rc;
                }
            }

            continue;
        }

        // ---------------------------------------------------------------
        // anything else: positional, skip
        // ---------------------------------------------------------------
    }

    return true;
}
