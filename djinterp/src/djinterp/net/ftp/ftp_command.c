/*******************************************************************************
* djinterp [net]                                                   ftp_command.c
*
* Implementation of the command handling declared in ftp_command.h.
*   One table, indexed by enum d_ftp_command, holds every verb with its
* argument policy and whether it opens a data connection.
*
*
* path:      /src/djinterp/net/ftp/ftp_command.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_command.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_ftp_internal_command_info
//   struct: one row of the command table.
struct d_ftp_internal_command_info
{
    const char*                name;       // the verb; NULL for UNKNOWN
    enum d_ftp_argument_policy policy;     // whether it takes an argument
    bool                       uses_data;  // whether it transfers data
};

// COMMANDS
//   constant: the command table, indexed by enum d_ftp_command.
static const struct d_ftp_internal_command_info COMMANDS[D_FTP_COMMAND_COUNT] =
{
    [D_FTP_COMMAND_UNKNOWN] = { NULL,   D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_USER]    = { "USER", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_PASS]    = { "PASS", D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_ACCT]    = { "ACCT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_CWD]     = { "CWD",  D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_CDUP]    = { "CDUP", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_SMNT]    = { "SMNT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_REIN]    = { "REIN", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_QUIT]    = { "QUIT", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_PORT]    = { "PORT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_PASV]    = { "PASV", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_TYPE]    = { "TYPE", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_STRU]    = { "STRU", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_MODE]    = { "MODE", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_RETR]    = { "RETR", D_FTP_ARGUMENT_REQUIRED, true  },
    [D_FTP_COMMAND_STOR]    = { "STOR", D_FTP_ARGUMENT_REQUIRED, true  },
    [D_FTP_COMMAND_STOU]    = { "STOU", D_FTP_ARGUMENT_OPTIONAL, true  },
    [D_FTP_COMMAND_APPE]    = { "APPE", D_FTP_ARGUMENT_REQUIRED, true  },
    [D_FTP_COMMAND_ALLO]    = { "ALLO", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_REST]    = { "REST", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_RNFR]    = { "RNFR", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_RNTO]    = { "RNTO", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_ABOR]    = { "ABOR", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_DELE]    = { "DELE", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_RMD]     = { "RMD",  D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_MKD]     = { "MKD",  D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_PWD]     = { "PWD",  D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_LIST]    = { "LIST", D_FTP_ARGUMENT_OPTIONAL, true  },
    [D_FTP_COMMAND_NLST]    = { "NLST", D_FTP_ARGUMENT_OPTIONAL, true  },
    [D_FTP_COMMAND_SITE]    = { "SITE", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_SYST]    = { "SYST", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_STAT]    = { "STAT", D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_HELP]    = { "HELP", D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_NOOP]    = { "NOOP", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_AUTH]    = { "AUTH", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_ADAT]    = { "ADAT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_PBSZ]    = { "PBSZ", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_PROT]    = { "PROT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_CCC]     = { "CCC",  D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_MIC]     = { "MIC",  D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_CONF]    = { "CONF", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_ENC]     = { "ENC",  D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_FEAT]    = { "FEAT", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_OPTS]    = { "OPTS", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_EPRT]    = { "EPRT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_EPSV]    = { "EPSV", D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_LANG]    = { "LANG", D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_MDTM]    = { "MDTM", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_SIZE]    = { "SIZE", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_MLST]    = { "MLST", D_FTP_ARGUMENT_OPTIONAL, false },
    [D_FTP_COMMAND_MLSD]    = { "MLSD", D_FTP_ARGUMENT_OPTIONAL, true  },
    [D_FTP_COMMAND_HOST]    = { "HOST", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_LPRT]    = { "LPRT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_LPSV]    = { "LPSV", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_XCUP]    = { "XCUP", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_XCWD]    = { "XCWD", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_XMKD]    = { "XMKD", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_XPWD]    = { "XPWD", D_FTP_ARGUMENT_NONE,     false },
    [D_FTP_COMMAND_XRMD]    = { "XRMD", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_MFMT]    = { "MFMT", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_PRET]    = { "PRET", D_FTP_ARGUMENT_REQUIRED, false },
    [D_FTP_COMMAND_CLNT]    = { "CLNT", D_FTP_ARGUMENT_REQUIRED, false }
};

//==============================================================================
// 3.  COMMANDS
//==============================================================================

/*
d_ftp_internal_is_verb
  File-local: reports whether `_length` bytes form a command verb: a letter,
then letters or digits, D_FTP_VERB_MAX at most.
*/
D_STATIC bool
d_ftp_internal_is_verb(
    const char* _text,
    size_t      _length
)
{
    // one to D_FTP_VERB_MAX characters, the first a letter
    if ( (_length == 0u)                      ||
         (_length > D_FTP_VERB_MAX)           ||
         (!d_ftp_internal_is_alpha(_text[0])) )
    {
        return false;
    }

    // the rest letters or digits
    for (size_t index = 1u; index < _length; index++)
    {
        if ( (!d_ftp_internal_is_alpha(_text[index])) &&
             (!d_ftp_internal_is_digit(_text[index])) )
        {
            return false;
        }
    }

    return true;
}

/*
d_ftp_internal_command_index
  File-local: the table row of a command, or 0 (UNKNOWN) for a value outside
the enumeration. Comparing as unsigned also catches negative values.
*/
D_STATIC size_t
d_ftp_internal_command_index(
    enum d_ftp_command _command
)
{
    const unsigned value = (unsigned)_command;

    // anything outside the enumeration is unknown
    if (value >= (unsigned)D_FTP_COMMAND_COUNT)
    {
        return 0u;
    }

    return (size_t)value;
}

/*
d_ftp_command_name
  The UNKNOWN row's name is NULL, so an out-of-range value maps there too.
*/
const char*
d_ftp_command_name(
    enum d_ftp_command _command
)
{
    return COMMANDS[d_ftp_internal_command_index(_command)].name;
}

/*
d_ftp_command_lookup
  A linear scan: the table is small, and lookups happen once per command.
*/
enum d_ftp_command
d_ftp_command_lookup(
    const char* _verb,
    size_t      _length
)
{
    // parameter validation
    if (!_verb)
    {
        return D_FTP_COMMAND_UNKNOWN;
    }

    // every named row, compared without regard to case
    for (size_t index = 1u; index < (size_t)D_FTP_COMMAND_COUNT; index++)
    {
        const bool match = d_ftp_internal_equals_nocase(_verb,
                                                        _length,
                                                        COMMANDS[index].name);

        // the first match is the only one
        if (match)
        {
            return (enum d_ftp_command)index;
        }
    }

    return D_FTP_COMMAND_UNKNOWN;
}

/*
d_ftp_command_policy
  The UNKNOWN row is OPTIONAL: a server cannot know what an unknown verb
expects, and answers it with 500 regardless.
*/
enum d_ftp_argument_policy
d_ftp_command_policy(
    enum d_ftp_command _command
)
{
    return COMMANDS[d_ftp_internal_command_index(_command)].policy;
}

/*
d_ftp_command_uses_data
  A table lookup, as above.
*/
bool
d_ftp_command_uses_data(
    enum d_ftp_command _command
)
{
    return COMMANDS[d_ftp_internal_command_index(_command)].uses_data;
}

/*
d_ftp_command_format
  Resolves the verb, then shares every check with the raw form.
*/
enum d_ftp_error
d_ftp_command_format(
    enum d_ftp_command   _command,
    const char*          _argument,
    struct d_ftp_buffer* _out
)
{
    const char* const name = d_ftp_command_name(_command);

    // only named commands have a verb to send
    if (!name)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    return d_ftp_command_format_raw(name,
                                    _argument,
                                    _out);
}

/*
d_ftp_command_format_raw
  The total length is checked before anything is written, so the command
lands whole or not at all.
*/
enum d_ftp_error
d_ftp_command_format_raw(
    const char*          _verb,
    const char*          _argument,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!_verb) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t verb_length     = strlen(_verb);
    const size_t argument_length = (_argument) ? strlen(_argument) : 0u;
    const bool   verb_ok         = d_ftp_internal_is_verb(_verb,
                                                          verb_length);
    const bool   breaks          = d_ftp_internal_has_line_break(
                                       _argument,
                                       argument_length);

    // a well-formed verb, and no line break to smuggle in a second command
    if ( (!verb_ok) ||
         (breaks) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t separator = (_argument) ? 1u : 0u;

    // the whole command, or nothing
    if ((verb_length + separator + argument_length + 2u) >
        d_ftp_internal_room(_out))
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    d_ftp_internal_append(_out,
                          _verb,
                          verb_length);

    // an argument, even an empty one, follows a single space
    if (_argument)
    {
        d_ftp_internal_append_char(_out,
                                   ' ');
        d_ftp_internal_append(_out,
                              _argument,
                              argument_length);
    }

    d_ftp_internal_append(_out,
                          D_FTP_EOL,
                          2u);

    return D_FTP_OK;
}

/*
d_ftp_command_parse
  The verb is taken up to the first space, never beyond, so an argument full
of spaces -- a file name, say -- survives intact after that one separator.
*/
enum d_ftp_error
d_ftp_command_parse(
    const char*                _line,
    size_t                     _length,
    struct d_ftp_command_line* _out
)
{
    // parameter validation
    if ( (!_line) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_ftp_span line  = d_ftp_internal_trim_eol(_line,
                                                            _length);
    size_t                  start = 0;

    // stray leading spaces are tolerated
    while ( (start < line.length) &&
            (line.data[start] == ' ') )
    {
        start++;
    }

    size_t verb_end = start;

    // the verb runs to the first space
    while ( (verb_end < line.length) &&
            (line.data[verb_end] != ' ') )
    {
        verb_end++;
    }

    const size_t verb_length = verb_end - start;

    // letters and digits only, within the limit
    if (!d_ftp_internal_is_verb(line.data + start,
                                verb_length))
    {
        return D_FTP_ERROR_MALFORMED;
    }

    _out->command      = d_ftp_command_lookup(line.data + start,
                                              verb_length);
    _out->verb.data    = line.data + start;
    _out->verb.length  = verb_length;
    _out->has_argument = (verb_end < line.length);

    // the argument is everything after the one separating space
    if (_out->has_argument)
    {
        _out->argument.data   = line.data + verb_end + 1u;
        _out->argument.length = line.length - verb_end - 1u;
    }
    else
    {
        _out->argument.data   = line.data + line.length;
        _out->argument.length = 0u;
    }

    return D_FTP_OK;
}

/*
d_ftp_telnet_strip
  Runs the same filter the reply parser uses, compacting surviving bytes over
the removed sequences. A sequence cut off by the end of the line is dropped.
*/
size_t
d_ftp_telnet_strip(
    char*  _line,
    size_t _length
)
{
    // parameter validation
    if (!_line)
    {
        return 0u;
    }

    unsigned char state   = (unsigned char)D_FTP_INTERNAL_TELNET_STATE_DATA;
    size_t        written = 0;

    // keep the data bytes, in order
    for (size_t index = 0; index < _length; index++)
    {
        const unsigned char byte = (unsigned char)_line[index];

        // command sequences are dropped
        if (d_ftp_internal_telnet_accept(&state,
                                         byte))
        {
            _line[written] = (char)byte;
            written++;
        }
    }

    return written;
}
