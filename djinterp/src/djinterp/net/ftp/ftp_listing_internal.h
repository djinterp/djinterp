/*******************************************************************************
* djinterp [net]                                          ftp_listing_internal.h
*
* Private helpers shared by the listing parser's files.
*   ftp_listing.c holds detection, the line dispatcher, and the token scanning
* every dialect uses; ftp_listing_unix.c, ftp_listing_dos.c, and
* ftp_listing_mlsx.c each hold one dialect. What crosses between them is
* declared here. Not installed, and not part of the public interface.
*
*
* path:      /src/djinterp/net/ftp/ftp_listing_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TOKENS
    ------
    1.  Scanning
    2.  Clocks
2.  DIALECTS
    --------
    1.  Unix
    2.  DOS
    3.  MLSx
*/

#ifndef DJINTERP_NET_FTP_FTP_LISTING_INTERNAL_H
#define DJINTERP_NET_FTP_FTP_LISTING_INTERNAL_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"     // d_ftp_span
#include "../../../../inc/djinterp/net/ftp/ftp_fact.h"       // d_ftp_time
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"    // d_ftp_entry


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TOKENS
//==============================================================================


// 1.1    Scanning
//------------------------------------------------------------------------------
// A line is read as blank-separated tokens: skip_blanks() returns the first
// position at or after `_position` that is not a space or tab, token_end()
// the first that is, and span_at() the part of `_line` from `_start` to
// `_end`, both clamped to the line.
size_t            d_ftp_internal_skip_blanks(struct d_ftp_span _line,
                                             size_t            _position);
size_t            d_ftp_internal_token_end(struct d_ftp_span _line,
                                           size_t            _position);
struct d_ftp_span d_ftp_internal_span_at(struct d_ftp_span _line,
                                         size_t            _start,
                                         size_t            _end);


// 1.2    Clocks
//------------------------------------------------------------------------------
// d_ftp_internal_read_clock() reads "HH:MM" into `_time`'s hour and minute,
// as both the Unix and the DOS dialects write a time of day.
bool d_ftp_internal_read_clock(struct d_ftp_span  _token,
                               struct d_ftp_time* _time);


//==============================================================================
// 2.  DIALECTS
//==============================================================================
// Each parse function reads one line of its dialect into `_out`, returning
// D_FTP_LINE_ENTRY, D_FTP_LINE_SKIP for a line that names nothing, or
// D_FTP_LINE_MALFORMED. The recognizers are what detection sniffs with.


// 2.1    Unix
//------------------------------------------------------------------------------
// d_ftp_internal_unix_mode() reads a permission field, "drwxr-xr-x" and
// kin, into an entry type and mode bits; `_now` places year-less dates.
bool                   d_ftp_internal_unix_mode(
                           struct d_ftp_span      _field,
                           enum d_ftp_entry_type* _out_type,
                           uint32_t*              _out_mode);
enum d_ftp_line_result d_ftp_internal_parse_unix(
                           struct d_ftp_span        _line,
                           const struct d_ftp_time* _now,
                           struct d_ftp_entry*      _out);


// 2.2    DOS
//------------------------------------------------------------------------------
// d_ftp_internal_is_dos_date() recognizes "MM-DD-YY" and "MM-DD-YYYY".
bool                   d_ftp_internal_is_dos_date(struct d_ftp_span _token);
enum d_ftp_line_result d_ftp_internal_parse_dos(
                           struct d_ftp_span   _line,
                           struct d_ftp_entry* _out);


// 2.3    MLSx
//------------------------------------------------------------------------------
// d_ftp_internal_mlsx_split() divides a line into its facts and its name at
// the first space after a ';', reporting whether the line had facts.
bool                   d_ftp_internal_mlsx_split(
                           struct d_ftp_span  _line,
                           struct d_ftp_span* _out_facts,
                           struct d_ftp_span* _out_name);
enum d_ftp_line_result d_ftp_internal_parse_mlsx(
                           struct d_ftp_span   _line,
                           struct d_ftp_entry* _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_LISTING_INTERNAL_H
