/*******************************************************************************
* djinterp [net]                                                  ftp_internal.h
*
* Private helpers shared by the net/ftp/ sources.
*   Locale-independent ASCII classification, bounded number parsing, span
* trimming, all-or-nothing buffer appends, the Telnet byte filter, and civil
* calendar arithmetic. Not installed, and not part of the public interface.
*
*
* path:      /src/djinterp/net/ftp/ftp_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Telnet
         1.  d_ftp_internal_telnet
         2.  d_ftp_internal_telnet_state
2.  HELPERS
    -------
    1.  Characters
    2.  Comparison
    3.  Spans
    4.  Numbers
    5.  Buffers
    6.  Telnet
    7.  Calendar
*/

#ifndef DJINTERP_NET_FTP_FTP_INTERNAL_H
#define DJINTERP_NET_FTP_FTP_INTERNAL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t, uint64_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_internal.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Telnet
//------------------------------------------------------------------------------
// 1.1.1
// d_ftp_internal_telnet
//   enum: the Telnet bytes (RFC 854) that the control connection may carry.
enum d_ftp_internal_telnet
{
    D_FTP_INTERNAL_TELNET_WILL = 251,  // negotiation: an option follows
    D_FTP_INTERNAL_TELNET_WONT = 252,  // negotiation: an option follows
    D_FTP_INTERNAL_TELNET_DO   = 253,  // negotiation: an option follows
    D_FTP_INTERNAL_TELNET_DONT = 254,  // negotiation: an option follows
    D_FTP_INTERNAL_TELNET_IAC  = 255   // "interpret as command"
};

// 1.1.2
// d_ftp_internal_telnet_state
//   enum: the states of the incremental Telnet filter.
enum d_ftp_internal_telnet_state
{
    D_FTP_INTERNAL_TELNET_STATE_DATA = 0,  // reading data
    D_FTP_INTERNAL_TELNET_STATE_COMMAND,   // an IAC was just read
    D_FTP_INTERNAL_TELNET_STATE_OPTION     // a negotiation awaits its option
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


// 2.1    Characters
//------------------------------------------------------------------------------
// ASCII classification and case folding, independent of the locale and
// defined for every `char`. d_ftp_internal_hex_value() returns the value of
// a hexadecimal digit of either case, or -1 for any other byte.
bool d_ftp_internal_is_digit(char _c);
bool d_ftp_internal_is_alpha(char _c);
bool d_ftp_internal_is_blank(char _c);
char d_ftp_internal_to_lower(char _c);
int  d_ftp_internal_hex_value(char _c);

// 2.2    Comparison
//------------------------------------------------------------------------------
// `_text` spans `_length` bytes and need not be terminated; `_word` is
// NUL-terminated. Both fold ASCII case; equality also requires equal length.
bool d_ftp_internal_equals_nocase(const char* _text,
                                  size_t      _length,
                                  const char* _word);
bool d_ftp_internal_starts_nocase(const char* _text,
                                  size_t      _length,
                                  const char* _word);

// 2.3    Spans
//------------------------------------------------------------------------------
// d_ftp_internal_trim() drops leading blanks and trailing blanks, CR, and LF;
// d_ftp_internal_trim_eol() drops only trailing CR and LF, since blanks may
// belong to a listed name. d_ftp_internal_has_line_break() reports whether
// any byte is CR or LF, the bytes that would end a command or reply line
// early; NULL text has none.
struct d_ftp_span d_ftp_internal_trim(const char* _text,
                                      size_t      _length);
struct d_ftp_span d_ftp_internal_trim_eol(const char* _text,
                                          size_t      _length);
bool              d_ftp_internal_has_line_break(const char* _text,
                                                size_t      _length);

// 2.4    Numbers
//------------------------------------------------------------------------------
// d_ftp_internal_is_number() reports whether a span is one or more decimal
// digits; d_ftp_internal_digits_value() reads digits the caller has already
// checked, four at most, so the value cannot overflow.
bool     d_ftp_internal_is_number(struct d_ftp_span _span);
unsigned d_ftp_internal_digits_value(const char* _text,
                                     size_t      _count);
/**
 * @brief Parses a run of decimal digits into a bounded value.
 *
 * @param[in]  _text      the digits; every byte must be one.
 * @param[in]  _length    their count; zero fails.
 * @param[in]  _max       the largest value accepted, up to UINT64_MAX.
 * @param[out] _out_value the value, written only on success.
 * @return true on success; false for an empty field, a non-digit, or a value
 *         above `_max`, which is refused before it could overflow.
 */
bool     d_ftp_internal_parse_uint(const char* _text,
                                   size_t      _length,
                                   uint64_t    _max,
                                   uint64_t*   _out_value);

// 2.5    Buffers
//------------------------------------------------------------------------------
// Every writer requires a buffer d_ftp_internal_buffer_ok() accepts: storage
// present, and the length inside it with room left for the terminator, which
// is always kept. d_ftp_internal_room() is the number of bytes such a buffer
// can still take; d_ftp_internal_rollback() truncates it to an earlier
// length, undoing a partial write.
bool             d_ftp_internal_buffer_ok(const struct d_ftp_buffer* _buffer);
size_t           d_ftp_internal_room(const struct d_ftp_buffer* _buffer);
void             d_ftp_internal_rollback(struct d_ftp_buffer* _out,
                                         size_t               _mark);
/**
 * @brief Appends bytes and re-terminates the buffer, all or nothing.
 *
 * @param[in,out] _out    the buffer.
 * @param[in]     _data   the bytes; may be NULL when `_length` is 0.
 * @param[in]     _length their count.
 * @return true if the bytes and the terminator fit; false, having appended
 *         nothing, otherwise.
 */
bool             d_ftp_internal_append(struct d_ftp_buffer* _out,
                                       const char*          _data,
                                       size_t               _length);
/**
 * @brief Appends one byte, all or nothing.
 *
 * @param[in,out] _out the buffer.
 * @param[in]     _c   the byte.
 * @return true if it fit; false, having appended nothing, otherwise.
 */
bool             d_ftp_internal_append_char(struct d_ftp_buffer* _out,
                                            char                 _c);
/**
 * @brief Appends a NUL-terminated string, all or nothing.
 *
 * @param[in,out] _out  the buffer.
 * @param[in]     _text the string.
 * @return true if it fit; false, having appended nothing, otherwise.
 */
bool             d_ftp_internal_append_text(struct d_ftp_buffer* _out,
                                            const char*          _text);
/**
 * @brief Appends a value in decimal, zero-padded to a minimum width.
 *
 * @param[in,out] _out   the buffer.
 * @param[in]     _value the value.
 * @param[in]     _width the minimum number of digits; 0 for no padding.
 * @return true if the whole field fit; false, having appended nothing,
 *         otherwise.
 */
bool             d_ftp_internal_append_uint(struct d_ftp_buffer* _out,
                                            uint64_t             _value,
                                            size_t               _width);
/**
 * @brief Appends text composed in a scratch buffer, all or nothing.
 *
 * @param[in,out] _out   the buffer.
 * @param[in]     _local the composed text.
 * @return D_FTP_OK, or D_FTP_ERROR_BUFFER_TOO_SMALL with `_out` unchanged.
 */
enum d_ftp_error d_ftp_internal_commit(struct d_ftp_buffer*       _out,
                                       const struct d_ftp_buffer* _local);

// 2.6    Telnet
//------------------------------------------------------------------------------
/**
 * @brief Advances the Telnet filter by one byte.
 *
 * IAC IAC yields one data byte 0xFF; IAC WILL, WONT, DO, or DONT swallows the
 * option byte after it; any other IAC command is two bytes long.
 *
 * @param[in,out] _state the filter state, which starts at
 *                       D_FTP_INTERNAL_TELNET_STATE_DATA.
 * @param[in]     _byte  the next byte received.
 * @return true if the byte is data; false if it belongs to a command.
 */
bool d_ftp_internal_telnet_accept(unsigned char* _state,
                                  unsigned char  _byte);

// 2.7    Calendar
//------------------------------------------------------------------------------
// Days from 1970-01-01 to a proleptic Gregorian date, negative before it, and
// back again; `_month` is 1 through 12 and `_day` 1 through 31.
int64_t d_ftp_internal_days_from_civil(int64_t  _year,
                                       unsigned _month,
                                       unsigned _day);
void    d_ftp_internal_civil_from_days(int64_t   _days,
                                       int64_t*  _out_year,
                                       unsigned* _out_month,
                                       unsigned* _out_day);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_INTERNAL_H
