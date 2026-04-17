/******************************************************************************
* djinterp [test]                                                   test_cli.h
*
* 
* 
*
* path:      /inc/test/test_cli.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.24
******************************************************************************/

#ifndef DJINTERP_TEST_CLI_
#define	DJINTERP_TEST_CLI_ 1

#include <stddef.h>
#include "../djinterp.h"
#include "../util/cli/cli_option.h"


// d_test_cli_option
//   struct: defines a single CLI option entry. the set of all options
// for a program is a simple static array of these structs.
struct d_test_cli_option
{
    const char*  key;            // verbose option name (e.g. "help")
    const char*  flag;           // short flag (e.g. "h")
    const char*  description;    // help text
    const void*  default_value;  // pointer to default value
    void*        value;          // pointer to current value
    fn_cli_match handler;        // handler function (NULL = toggle bool)
};

// I.    direct lookup by key or flag

// d_test_cli_option_find_key
//   finds an option row by its verbose key string.
// returns pointer to the matching row, or NULL.
const struct d_test_cli_option* d_test_cli_option_find_key(
    const struct d_test_cli_option* _options,
    size_t                          _count,
    const char*                     _key);

// d_test_cli_option_find_flag
//   finds an option row by its short flag string.
// returns pointer to the matching row, or NULL.
const struct d_test_cli_option*
d_test_cli_option_find_flag(const struct d_test_cli_option* _options,
                            size_t                          _count,
                            const char*                     _flag);


// II.   custom lookup with function pointer + context

// d_test_cli_option_find_with
//   finds an option using a user-supplied lookup function.
// the lookup function receives the raw option array, count, key,
// and an opaque context pointer.
const void*
d_test_cli_option_find_with(const void*   _options,
                            size_t        _count,
                            const char*   _key,
                            fn_cli_lookup _lookup,
                            void*         _context);


/******************************************************************************
 * PARSE FUNCTION
 *****************************************************************************/

// III.  argv parsing with operator specification

// d_test_cli_option_parse
//   parses argc/argv against an option table. matches flags using
// _short_op prefix (e.g. "-") and keys using _long_op prefix
// (e.g. "--"). when a match is found, calls the handler if set,
// or toggles the bool at value if handler is NULL.
// returns true if parsing completed, false if a handler requested stop.
bool
d_test_cli_option_parse(struct d_test_cli_option* _options,
                        size_t                    _count,
                        const char*               _short_op,
                        const char*               _long_op,
                        int                       _argc,
                        char*                     _argv[]);


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

// IV.   helpers

// d_test_cli_option_count
//   returns the number of options in a static array.
// usage: d_test_cli_option_count(D_TEST_OPTIONS)
#define d_test_cli_option_count(_array)               \
    (sizeof(_array) / sizeof((_array)[0]))

// d_test_cli_option_print_usage
//   prints a formatted usage message for all options in the table.
void
d_test_cli_option_print_usage(const char*                      _program_name,
                              const struct d_test_cli_option*  _options,
                              size_t                           _count,
                              const char*                      _short_op,
                              const char*                      _long_op);



#endif	// DJINTERP_TEST_CLI_