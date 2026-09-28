/*******************************************************************************
* djinterp [net]                                                   ftp_listing.h
*
* FTP directory listings: Unix, DOS, MLSx, and bare names.
*   Detects the dialect of a listing line and parses it into an entry -- type,
* size, modification time, permissions, owner, group, and link target, as far
* as the dialect provides them -- as spans into the line.
*
*
* path:      /inc/djinterp/net/ftp/ftp_listing.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Entries
         1.  d_ftp_entry_type
         2.  d_ftp_entry_known
         3.  d_ftp_entry
    2.  Dialects and results
         1.  d_ftp_listing_format
         2.  d_ftp_line_result
2.  DIRECTORY LISTINGS
    ------------------
    1.  Detection and parsing
*/

#ifndef DJINTERP_NET_FTP_FTP_LISTING_H
#define DJINTERP_NET_FTP_FTP_LISTING_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_span
#include "./ftp_fact.h"        // d_ftp_time


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_listing.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Entries
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_entry_type
//   enum: what a listing entry names.
enum d_ftp_entry_type
{
    D_FTP_ENTRY_UNKNOWN = 0,        // the listing did not say
    D_FTP_ENTRY_FILE,               // a regular file
    D_FTP_ENTRY_DIRECTORY,          // a directory
    D_FTP_ENTRY_SYMLINK,            // a symbolic link
    D_FTP_ENTRY_CURRENT_DIRECTORY,  // MLSx "cdir": the listed directory
    D_FTP_ENTRY_PARENT_DIRECTORY,   // MLSx "pdir": its parent
    D_FTP_ENTRY_OTHER               // a device, pipe, socket, or OS type
};

// 1.1.2
// d_ftp_entry_known
//   enum: bit flags recording which d_ftp_entry fields a line supplied;
// fields whose bit is clear hold zero and mean nothing.
enum d_ftp_entry_known
{
    D_FTP_ENTRY_KNOWN_NONE        = 0,
    D_FTP_ENTRY_KNOWN_SIZE        = 0x0001,  // size
    D_FTP_ENTRY_KNOWN_MODIFIED    = 0x0002,  // modified: its date
    D_FTP_ENTRY_KNOWN_TIME        = 0x0004,  // modified: its time of day
    D_FTP_ENTRY_KNOWN_UTC         = 0x0008,  // modified is UTC, not local
    D_FTP_ENTRY_KNOWN_MODE        = 0x0010,  // mode
    D_FTP_ENTRY_KNOWN_OWNER       = 0x0020,  // owner
    D_FTP_ENTRY_KNOWN_GROUP       = 0x0040,  // group
    D_FTP_ENTRY_KNOWN_UNIQUE      = 0x0080,  // unique
    D_FTP_ENTRY_KNOWN_PERMISSIONS = 0x0100,  // permissions
    D_FTP_ENTRY_KNOWN_LINK_TARGET = 0x0200   // link_target
};

// 1.1.3
// d_ftp_entry
//   struct: one parsed listing line. Every span points into that line.
struct d_ftp_entry
{
    struct d_ftp_span     name;         // the entry's name; never empty
    struct d_ftp_span     link_target;  // a symbolic link's target
    struct d_ftp_span     owner;        // owning user
    struct d_ftp_span     group;        // owning group
    struct d_ftp_span     unique;       // MLSx "unique" fact
    struct d_ftp_span     permissions;  // MLSx "perm" fact, e.g. "adfrw"
    enum d_ftp_entry_type type;         // what the entry names
    uint64_t              size;         // size in bytes
    struct d_ftp_time     modified;     // last modification time
    uint32_t              mode;         // Unix mode bits, e.g. 0755
    unsigned              known;        // D_FTP_ENTRY_KNOWN_* bits
};

// 1.2    Dialects and results
//------------------------------------------------------------------------------
// 1.2.1
// d_ftp_listing_format
//   enum: the dialect of a directory listing line.
enum d_ftp_listing_format
{
    D_FTP_LISTING_AUTO = 0,  // detect per line
    D_FTP_LISTING_UNIX,      // "ls -l" style LIST output
    D_FTP_LISTING_DOS,       // Windows / IIS style LIST output
    D_FTP_LISTING_MLSX,      // RFC 3659 MLSD and MLST output
    D_FTP_LISTING_NAMES      // NLST output: one bare name per line
};

// 1.2.2
// d_ftp_line_result
//   enum: what parsing one listing line produced.
enum d_ftp_line_result
{
    D_FTP_LINE_ENTRY = 0,  // an entry was parsed
    D_FTP_LINE_SKIP,       // a line that names nothing, e.g. "total 48"
    D_FTP_LINE_MALFORMED   // a line in no recognized form
};


//==============================================================================
// 2.  DIRECTORY LISTINGS
//==============================================================================
// LIST output is unstandardized; it is parsed here as the two dialects that
// cover nearly every server. Prefer MLSD, whose machine-readable form
// (RFC 3659 7) carries exact sizes and UTC times.


// 2.1    Detection and parsing
//------------------------------------------------------------------------------
/**
 * @brief Guesses the dialect of one listing line from its shape.
 *
 * @param[in] _line   the line.
 * @param[in] _length its length in bytes.
 * @return the dialect, or D_FTP_LISTING_AUTO when the line matches none.
 */
enum d_ftp_listing_format d_ftp_listing_detect(const char* _line,
                                               size_t      _length);
/**
 * @brief Parses one line of a directory listing.
 *
 * Unix lines accept "ls -l" dates ("Sep 25 12:34", "Sep 25  2024") and ISO
 * dates ("2024-09-25 12:34"); a date without a year takes the latest year
 * that does not put it more than a day after `_now`. Symbolic link names are
 * split at " -> ". DOS lines accept two- and four-digit years and 12- or
 * 24-hour times. Only CR and LF are trimmed from the end: trailing spaces
 * belong to the name.
 *
 * @param[in]  _line   the line.
 * @param[in]  _length its length in bytes.
 * @param[in]  _format the dialect, or D_FTP_LISTING_AUTO to detect it.
 * @param[in]  _now    the current time, to date Unix entries without a year;
 *                     NULL leaves such dates unknown.
 * @param[out] _out    the entry; its spans point into `_line`.
 * @return D_FTP_LINE_ENTRY, D_FTP_LINE_SKIP, or D_FTP_LINE_MALFORMED
 *         (including for a NULL argument).
 */
enum d_ftp_line_result    d_ftp_listing_parse(const char*               _line,
                                              size_t                    _length,
                                              enum d_ftp_listing_format _format,
                                              const struct d_ftp_time*  _now,
                                              struct d_ftp_entry*       _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_LISTING_H
