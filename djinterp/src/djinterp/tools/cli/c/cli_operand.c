/*******************************************************************************
* djinterp [tools]                                                 cli_operand.c
*
* The command-line kit's operands that name something other than a file,
* declared in cli_operand.h.
*   The prefix is matched as bytes; whether a file has the operand's name is
* asked of filesys, as the kit's own writes ask it, following a symbolic
* link to what it names. Only the want of memory is a failure: a name that
* cannot be looked at for any other reason -- not there, not reachable, too
* long -- is no file to warn of.
*
*
* path:      /src/djinterp/tools/cli/c/cli_operand.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/cli/c/cli_operand.h"  // corresponding header
// std
#include <errno.h>    // errno, ENOMEM
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // strlen, strncmp
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_stat.h"  // d_file_mode
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // statuses
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // d_cli_report
#include "./cli_internal.h"  // d_cli_internal_mode_kind
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  NAMED OPERANDS
//==============================================================================

// 1.1    The prefix
//------------------------------------------------------------------------------

/**
 * @brief Whether a prefix is one R13 allows: two lowercase ASCII letters or
 *        more, so that a Windows drive is never one.
 *
 * @param[in] _prefix  the prefix, without its colon.
 * @return whether it is.
 */
static
bool d_cli_internal_operand_prefix(
    const char* _prefix
)
{
    size_t length = 0u;

    // lowercase letters, each
    for (; _prefix[length] != '\0'; ++length)
    {
        if ( (_prefix[length] < 'a') ||
             (_prefix[length] > 'z') )
        {
            return false;
        }
    }

    return (length >= 2u);
}

// 1.2    The file
//------------------------------------------------------------------------------

/**
 * @brief Looks for something other than a directory with a named operand's
 *        name, and says it is there, or that there was no room to look.
 *
 * @param[in] _program  the program, which says it.
 * @param[in] _word     the operand.
 * @param[in] _what     what it names, as the warning says it.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY, said, when there was no room
 *         to look.
 */
static
int d_cli_internal_operand_file(
    const struct dj_cli_program* _program,
    const char*                  _word,
    const char*                  _what
)
{
    uint32_t mode = 0u;

    errno = 0;

    // nothing there, or nothing that can be looked at, is no file
    if (d_file_mode(_word,
                    &mode) != 0)
    {
        if (errno != ENOMEM)
        {
            return D_ARG_OK;
        }

        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "%s",
                     "out of memory");

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // a directory is nothing a command reads in a named operand's place
    if (d_cli_internal_mode_kind(mode) != 2)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_WARNING,
                     "'%s' is %s; the file is './%s'",
                     _word,
                     _what,
                     _word);
    }

    return D_ARG_OK;
}

// 1.3    Reading one
//------------------------------------------------------------------------------

/*
d_cli_named_operand
  The prefix, its colon and a name of a byte or more; then the file, said
where it is there.
*/
int d_cli_named_operand(
    const struct dj_cli_program* _program,
    const char*                  _word,
    const char*                  _prefix,
    const char*                  _what,
    const char**                 _name
)
{
    if (_name != NULL)
    {
        *_name = NULL;
    }

    // parameter validation
    if ( (!_program) ||
         (!_word)    ||
         (!_prefix)  ||
         (!_what)    ||
         (!_name)    ||
         (!d_cli_internal_operand_prefix(_prefix)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t length = strlen(_prefix);

    // a file's name
    if ( (strncmp(_word,
                  _prefix,
                  length) != 0) ||
         (_word[length] != ':') ||
         (_word[length + 1u] == '\0') )
    {
        return D_ARG_OK;
    }

    *_name = _word + length + 1u;

    return d_cli_internal_operand_file(_program,
                                       _word,
                                       _what);
}
