/*******************************************************************************
* djinterp [net]                                                      ftp_fact.h
*
* FTP file facts: modification times and sizes (RFC 3659).
*   The YYYYMMDDHHMMSS[.sss] timestamps of MDTM, MFMT, and MLSx, with calendar
* validation and Unix-time conversion, and the decimal sizes SIZE reports.
*
*
* path:      /inc/djinterp/net/ftp/ftp_fact.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Buffer sizes
         1.  D_FTP_TIME_SIZE
2.  TYPES
    -----
    1.  Timestamps
         1.  d_ftp_time
3.  TIMESTAMPS AND SIZES
    --------------------
    1.  Timestamps
    2.  Sizes
*/

#ifndef DJINTERP_NET_FTP_FTP_FACT_H
#define DJINTERP_NET_FTP_FTP_FACT_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t, uint8_t, uint16_t, uint64_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_error, d_ftp_buffer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_fact.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Buffer sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_TIME_SIZE
//   constant: bytes for a timestamp with milliseconds, "YYYYMMDDHHMMSS.sss",
// and its terminator.
#define D_FTP_TIME_SIZE          19


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Timestamps
//------------------------------------------------------------------------------
// 2.1.1
// d_ftp_time
//   struct: a calendar time as FTP writes it (RFC 3659 time-val). MDTM and
// MLSx times are UTC; times read from LIST output are the server's local
// time, which it does not name.
struct d_ftp_time
{
    uint16_t year;         // 1-9999
    uint8_t  month;        // 1-12
    uint8_t  day;          // 1-31, as the month allows
    uint8_t  hour;         // 0-23
    uint8_t  minute;       // 0-59
    uint8_t  second;       // 0-60, allowing a leap second
    uint16_t millisecond;  // 0-999
};


//==============================================================================
// 3.  TIMESTAMPS AND SIZES
//==============================================================================


// 3.1    Timestamps
//------------------------------------------------------------------------------
/**
 * @brief Reports whether every field of a time is in range, the day checked
 *        against its month and year.
 *
 * @param[in] _time the time; NULL is invalid.
 * @return true if valid.
 */
bool             d_ftp_time_is_valid(const struct d_ftp_time* _time);
/**
 * @brief Parses an RFC 3659 time-val, "YYYYMMDDHHMMSS[.sss]", as MDTM and
 *        MLSx write it; surrounding blanks are ignored and fractions past
 *        milliseconds are truncated.
 *
 * @param[in]  _text   the text.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the time.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or D_FTP_ERROR_MALFORMED.
 */
enum d_ftp_error d_ftp_time_parse(const char*        _text,
                                  size_t             _length,
                                  struct d_ftp_time* _out);
/**
 * @brief Appends a time-val, with or without milliseconds.
 *
 * @param[in]     _time             the time; must be valid.
 * @param[in]     _with_millisecond true to append ".sss".
 * @param[in,out] _out              the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_time_format(const struct d_ftp_time* _time,
                                   bool                     _with_millisecond,
                                   struct d_ftp_buffer*     _out);
/**
 * @brief Converts a UTC time to seconds since 1970-01-01T00:00:00Z,
 *        milliseconds dropped.
 *
 * @param[in]  _time        the time; must be valid.
 * @param[out] _out_seconds the count; negative before 1970.
 * @return D_FTP_OK or D_FTP_ERROR_INVALID_ARGUMENT.
 */
enum d_ftp_error d_ftp_time_to_unix(const struct d_ftp_time* _time,
                                    int64_t*                 _out_seconds);
/**
 * @brief Converts seconds since 1970-01-01T00:00:00Z to a UTC time.
 *
 * @param[in]  _seconds the count.
 * @param[out] _out     the time, milliseconds zero.
 * @return D_FTP_OK, or D_FTP_ERROR_INVALID_ARGUMENT for a NULL `_out` or a
 *         date outside years 1 to 9999.
 */
enum d_ftp_error d_ftp_time_from_unix(int64_t            _seconds,
                                      struct d_ftp_time* _out);

// 3.2    Sizes
//------------------------------------------------------------------------------
/**
 * @brief Parses the text of a 213 reply to SIZE: a decimal byte count,
 *        surrounding blanks ignored.
 *
 * @param[in]  _text     the text.
 * @param[in]  _length   its length in bytes.
 * @param[out] _out_size the count.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or D_FTP_ERROR_MALFORMED,
 *         overflow included.
 */
enum d_ftp_error d_ftp_parse_size(const char* _text,
                                  size_t      _length,
                                  uint64_t*   _out_size);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_FACT_H
