/*******************************************************************************
* djinterp [net]                                                   ftp_command.h
*
* FTP commands: the verb vocabulary, formatting, and parsing.
*   A table of the RFC 959 commands and their standard extensions with each
* one's argument policy; command lines formatted so that no argument can
* inject a second command; and server-side parsing of received lines.
*
*
* path:      /inc/djinterp/net/ftp/ftp_command.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Wire syntax
         1.  D_FTP_VERB_MAX
2.  TYPES
    -----
    1.  Commands
         1.  d_ftp_command
         2.  d_ftp_argument_policy
         3.  d_ftp_command_line
3.  COMMANDS
    --------
    1.  Vocabulary
    2.  Formatting
    3.  Parsing
    4.  Telnet
*/

#ifndef DJINTERP_NET_FTP_FTP_COMMAND_H
#define DJINTERP_NET_FTP_FTP_COMMAND_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_span, d_ftp_error, d_ftp_buffer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_command.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Wire syntax
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_VERB_MAX
//   constant: the longest command verb accepted, in characters. RFC 959 verbs
// are three or four letters; extensions such as XSHA256 run longer, so the
// limit is generous rather than exact.
#define D_FTP_VERB_MAX 16


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Commands
//------------------------------------------------------------------------------
// 2.1.1
// d_ftp_command
//   enum: every command this module knows by name, grouped by the document
// that defines it. D_FTP_COMMAND_COUNT is the number of values, not a command.
enum d_ftp_command
{
    D_FTP_COMMAND_UNKNOWN = 0,

    // RFC 959: access control
    D_FTP_COMMAND_USER,
    D_FTP_COMMAND_PASS,
    D_FTP_COMMAND_ACCT,
    D_FTP_COMMAND_CWD,
    D_FTP_COMMAND_CDUP,
    D_FTP_COMMAND_SMNT,
    D_FTP_COMMAND_REIN,
    D_FTP_COMMAND_QUIT,

    // RFC 959: transfer parameters
    D_FTP_COMMAND_PORT,
    D_FTP_COMMAND_PASV,
    D_FTP_COMMAND_TYPE,
    D_FTP_COMMAND_STRU,
    D_FTP_COMMAND_MODE,

    // RFC 959: service
    D_FTP_COMMAND_RETR,
    D_FTP_COMMAND_STOR,
    D_FTP_COMMAND_STOU,
    D_FTP_COMMAND_APPE,
    D_FTP_COMMAND_ALLO,
    D_FTP_COMMAND_REST,
    D_FTP_COMMAND_RNFR,
    D_FTP_COMMAND_RNTO,
    D_FTP_COMMAND_ABOR,
    D_FTP_COMMAND_DELE,
    D_FTP_COMMAND_RMD,
    D_FTP_COMMAND_MKD,
    D_FTP_COMMAND_PWD,
    D_FTP_COMMAND_LIST,
    D_FTP_COMMAND_NLST,
    D_FTP_COMMAND_SITE,
    D_FTP_COMMAND_SYST,
    D_FTP_COMMAND_STAT,
    D_FTP_COMMAND_HELP,
    D_FTP_COMMAND_NOOP,

    // RFC 2228 and RFC 4217: security
    D_FTP_COMMAND_AUTH,
    D_FTP_COMMAND_ADAT,
    D_FTP_COMMAND_PBSZ,
    D_FTP_COMMAND_PROT,
    D_FTP_COMMAND_CCC,
    D_FTP_COMMAND_MIC,
    D_FTP_COMMAND_CONF,
    D_FTP_COMMAND_ENC,

    // RFC 2389: feature negotiation
    D_FTP_COMMAND_FEAT,
    D_FTP_COMMAND_OPTS,

    // RFC 2428: IPv6 and NAT
    D_FTP_COMMAND_EPRT,
    D_FTP_COMMAND_EPSV,

    // RFC 2640: internationalization
    D_FTP_COMMAND_LANG,

    // RFC 3659: extensions
    D_FTP_COMMAND_MDTM,
    D_FTP_COMMAND_SIZE,
    D_FTP_COMMAND_MLST,
    D_FTP_COMMAND_MLSD,

    // RFC 7151: virtual hosts
    D_FTP_COMMAND_HOST,

    // RFC 1639 (experimental) and RFC 775 (obsolete) forms still seen
    D_FTP_COMMAND_LPRT,
    D_FTP_COMMAND_LPSV,
    D_FTP_COMMAND_XCUP,
    D_FTP_COMMAND_XCWD,
    D_FTP_COMMAND_XMKD,
    D_FTP_COMMAND_XPWD,
    D_FTP_COMMAND_XRMD,

    // widely deployed, never standardized
    D_FTP_COMMAND_MFMT,
    D_FTP_COMMAND_PRET,
    D_FTP_COMMAND_CLNT,

    D_FTP_COMMAND_COUNT
};

// 2.1.2
// d_ftp_argument_policy
//   enum: whether a command takes an argument.
enum d_ftp_argument_policy
{
    D_FTP_ARGUMENT_NONE = 0,   // the verb stands alone
    D_FTP_ARGUMENT_OPTIONAL,   // the argument may be omitted
    D_FTP_ARGUMENT_REQUIRED    // the argument must be present
};

// 2.1.3
// d_ftp_command_line
//   struct: a command line split into verb and argument, as spans into the
// line that was parsed.
struct d_ftp_command_line
{
    enum d_ftp_command command;       // D_FTP_COMMAND_UNKNOWN if not known
    struct d_ftp_span  verb;          // the verb as it was sent
    struct d_ftp_span  argument;      // everything after the first space
    bool               has_argument;  // a space followed the verb
};


//==============================================================================
// 3.  COMMANDS
//==============================================================================


// 3.1    Vocabulary
//------------------------------------------------------------------------------
// Table lookups. An unknown verb is D_FTP_COMMAND_UNKNOWN, whose name is NULL
// and whose argument policy is OPTIONAL; only the transfer commands (RETR,
// STOR, STOU, APPE, LIST, NLST, MLSD) use the data connection.
const char*                d_ftp_command_name(enum d_ftp_command _command);
enum d_ftp_command         d_ftp_command_lookup(const char* _verb,
                                                size_t      _length);
enum d_ftp_argument_policy d_ftp_command_policy(enum d_ftp_command _command);
bool                       d_ftp_command_uses_data(enum d_ftp_command _command);

// 3.2    Formatting
//------------------------------------------------------------------------------
/**
 * @brief Appends a known command with an optional argument, ready to send.
 *
 * A NULL `_argument` writes the verb alone; any other, even "", writes the
 * verb, one space, and the argument. An argument containing CR or LF is
 * refused: it would end the command early and smuggle in a second one.
 *
 * @param[in]     _command  the command; not D_FTP_COMMAND_UNKNOWN.
 * @param[in]     _argument the argument, or NULL.
 * @param[in,out] _out      the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_command_format(enum d_ftp_command   _command,
                                      const char*          _argument,
                                      struct d_ftp_buffer* _out);
/**
 * @brief Appends a command given by its verb, for extensions and SITE-style
 *        commands this module does not name.
 *
 * @param[in]     _verb     a letter followed by letters or digits, at most
 *                          D_FTP_VERB_MAX of them.
 * @param[in]     _argument the argument, or NULL; as for
 *                          d_ftp_command_format().
 * @param[in,out] _out      the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_command_format_raw(const char*          _verb,
                                          const char*          _argument,
                                          struct d_ftp_buffer* _out);

// 3.3    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Splits a received command line into verb and argument.
 *
 * The line terminator and any leading spaces are ignored. The verb runs to
 * the first space; the argument is everything after that one space,
 * interior and trailing spaces included, since file names may hold them.
 * Verbs match case-insensitively. Strip Telnet sequences first with
 * d_ftp_telnet_strip().
 *
 * @param[in]  _line   the command line.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the parts, as spans into `_line`.
 * @return D_FTP_OK (with D_FTP_COMMAND_UNKNOWN for a well-formed but
 *         unknown verb), D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_MALFORMED for a missing, overlong, or non-alphanumeric
 *         verb.
 */
enum d_ftp_error d_ftp_command_parse(const char*                _line,
                                     size_t                     _length,
                                     struct d_ftp_command_line* _out);

// 3.4    Telnet
//------------------------------------------------------------------------------
/**
 * @brief Removes Telnet command sequences from a received line, in place.
 *
 * Clients send IAC IP and IAC DM ahead of ABOR (RFC 959 4.1.3), and a few
 * send option negotiations; none of these are command text. IAC IAC becomes
 * one 0xFF data byte. A server passes each command line through this before
 * d_ftp_command_parse(); the reply parser filters replies itself.
 *
 * @param[in,out] _line   the bytes to filter.
 * @param[in]     _length their count.
 * @return the filtered length, never greater than `_length`.
 */
size_t d_ftp_telnet_strip(char*  _line,
                          size_t _length);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_COMMAND_H
