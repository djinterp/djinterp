/*******************************************************************************
* djinterp                                                            cli_unix.h
*
*   Unix CLI convention layer built on `cli_option.h`. Handles:
*
*   --key           long option, strip "--", look up "key"
*   --key=val       long option with embedded value, next_arg = "val"
*   -v              single short flag, strip "-", look up "v"
*   -xvf            combined short flags, expand to "x", "v", "f"
*   -o file         last short flag consumes next argv if handler says so
*   --              stop token, all subsequent args are positional
*
*   The entry table uses bare keys (no operators):
*
*   struct d_string_cli_entry opts[] = {
*       { "v",       1, &verbose },
*       { "verbose", 7, &verbose },
*       { "o",       1, &output  },
*       { "output",  6, &output  },
*   };
*   D_CLI_UNIX_PARSE(opts, argc, argv, my_handler, ctx);
*
*   All lookup is delegated to d_cli_find from cli_option.h.
*
*
* path:      /tests/djinterp/c/cli/cli_unix.h
* link(s):   cli_option.h
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.23
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CLI_UNIX_
#define DJINTERP_CLI_UNIX_ 1

// djinterp
#include "../../../../inc/djinterp/c/cli/cli_option.h"


/******************************************************************************
 * FUNCTION POINTER TYPE
 *****************************************************************************/

// fn_cli_unix_match
//   function pointer: handler invoked when a Unix-style CLI argument
// matches an entry during parsing.
//
// parameters:
//   _key:        the bare key that matched (e.g. "verbose", "o")
//   _key_length: length of _key in bytes (excludes null terminator)
//   _value:      the entry's void* value pointer from the table
//   _next_arg:   the next argv element or the embedded value after '='
//                (NULL if none available)
//   _context:    opaque user context
//
// return value:
//   < 0   stop parsing immediately
//     0   continue parsing; did NOT consume _next_arg
//   > 0   continue parsing; consumed _next_arg (skip N argv elements)
//
// note: when _next_arg comes from an embedded '=' (--key=val), the
// return value does not affect argv advancement regardless.
typedef int (*fn_cli_unix_match)(const char* _key,
                                 size_t      _key_length,
                                 void*       _value,
                                 const char* _next_arg,
                                 void*       _context);


/******************************************************************************
 * PARSE FUNCTION
 *****************************************************************************/

// d_cli_unix_parse
//   parses argc/argv using Unix conventions against a d_string_cli_entry
// table. processes argv elements starting from index 1:
//
//   "--"         bare stop token; all subsequent args are skipped
//   "--key"      strip "--", look up "key", next_arg = argv[i+1]
//   "--key=val"  strip "--", split on '=', look up "key", next_arg = "val"
//   "-abc"       strip "-", expand each char: look up "a", "b", "c"
//                for non-last chars, next_arg = NULL
//                for last char, next_arg = argv[i+1]
//   anything else is skipped (positional argument)
//
// returns true if parsing completed, false if a handler stopped it.
bool
d_cli_unix_parse(const struct d_string_cli_entry* _entries,
                 size_t                            _count,
                 int                               _argc,
                 char*                             _argv[],
                 fn_cli_unix_match                 _on_match,
                 void*                             _context);

// D_CLI_UNIX_PARSE
//   macro: convenience on a static array.
#define D_CLI_UNIX_PARSE(_array, _argc, _argv, _handler, _ctx)          \
    d_cli_unix_parse((_array), D_CLI_COUNT(_array),                      \
                     (_argc), (_argv), (_handler), (_ctx))


#endif  // DJINTERP_CLI_UNIX_
