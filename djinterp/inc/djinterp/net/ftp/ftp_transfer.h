/*******************************************************************************
* djinterp [net]                                                  ftp_transfer.h
*
* FTP transfer parameters and ASCII line-ending conversion.
*   TYPE, STRU, and MODE arguments (RFC 959 3.1 and 3.4) in both directions,
* and the incremental CR LF conversion an ASCII-type transfer needs between
* local text and the network's NVT form.
*
*
* path:      /inc/djinterp/net/ftp/ftp_transfer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Buffer sizes
         1.  D_FTP_TYPE_ARGUMENT_SIZE
2.  TYPES
    -----
    1.  Transfer parameters
         1.  d_ftp_data_type
         2.  d_ftp_format_control
         3.  d_ftp_type
         4.  d_ftp_structure
         5.  d_ftp_transfer_mode
    2.  ASCII conversion
         1.  d_ftp_ascii_state
3.  TRANSFER PARAMETERS
    -------------------
    1.  Representation type
    2.  Parameter codes
4.  ASCII TRANSFERS
    ---------------
    1.  Line-ending conversion
*/

#ifndef DJINTERP_NET_FTP_FTP_TRANSFER_H
#define DJINTERP_NET_FTP_FTP_TRANSFER_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_error, d_ftp_buffer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_transfer.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Buffer sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_TYPE_ARGUMENT_SIZE
//   constant: bytes for the longest TYPE argument, "L 255", and its
// terminator.
#define D_FTP_TYPE_ARGUMENT_SIZE 6


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Transfer parameters
//------------------------------------------------------------------------------
// 2.1.1
// d_ftp_data_type
//   enum: the representation type named by TYPE; the values are the codes.
enum d_ftp_data_type
{
    D_FTP_TYPE_ASCII  = 'A',  // NVT-ASCII text, CR LF line endings
    D_FTP_TYPE_EBCDIC = 'E',  // EBCDIC text
    D_FTP_TYPE_IMAGE  = 'I',  // binary, byte for byte
    D_FTP_TYPE_LOCAL  = 'L'   // binary with a stated logical byte size
};

// 2.1.2
// d_ftp_format_control
//   enum: the format control that may follow the ASCII and EBCDIC types.
enum d_ftp_format_control
{
    D_FTP_FORMAT_NONE             = 0,    // omitted: the server's default
    D_FTP_FORMAT_NON_PRINT        = 'N',  // no vertical format control
    D_FTP_FORMAT_TELNET           = 'T',  // Telnet format effectors
    D_FTP_FORMAT_CARRIAGE_CONTROL = 'C'   // ASA (FORTRAN) carriage control
};

// 2.1.3
// d_ftp_type
//   struct: a complete TYPE argument.
struct d_ftp_type
{
    enum d_ftp_data_type      data_type;  // A, E, I, or L
    enum d_ftp_format_control format;     // A and E only; NONE omits it
    unsigned                  byte_size;  // L only: bits per byte, 1-255
};

// 2.1.4
// d_ftp_structure
//   enum: the file structure named by STRU; the values are the codes.
enum d_ftp_structure
{
    D_FTP_STRUCTURE_FILE   = 'F',  // a continuous byte sequence
    D_FTP_STRUCTURE_RECORD = 'R',  // a sequence of records
    D_FTP_STRUCTURE_PAGE   = 'P'   // indexed pages (TOPS-20)
};

// 2.1.5
// d_ftp_transfer_mode
//   enum: the transmission mode named by MODE; the values are the codes.
enum d_ftp_transfer_mode
{
    D_FTP_MODE_STREAM     = 'S',  // bytes as they are; EOF by closing
    D_FTP_MODE_BLOCK      = 'B',  // headed blocks, with restart markers
    D_FTP_MODE_COMPRESSED = 'C',  // run-length compressed blocks
    D_FTP_MODE_DEFLATE    = 'Z'   // zlib deflate (draft, widely deployed)
};

// 2.2    ASCII conversion
//------------------------------------------------------------------------------
// 2.2.1
// d_ftp_ascii_state
//   struct: the state an ASCII-mode conversion carries between chunks, so a
// CR LF split across two reads is still recognized. Prepare it with
// d_ftp_ascii_init().
struct d_ftp_ascii_state
{
    bool pending_cr;  // the previous chunk ended in CR
};


//==============================================================================
// 3.  TRANSFER PARAMETERS
//==============================================================================


// 3.1    Representation type
//------------------------------------------------------------------------------
/**
 * @brief Appends a TYPE argument, such as "I", "A N", or "L 8".
 *
 * @param[in]     _type the type; L needs a byte size of 1 to 255.
 * @param[in,out] _out  the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_type_format(const struct d_ftp_type* _type,
                                   struct d_ftp_buffer*     _out);
/**
 * @brief Parses a TYPE argument, case-insensitively.
 *
 * An A or E type without a format control reads as NON_PRINT, the RFC 959
 * default; types other than L read with a byte size of 8.
 *
 * @param[in]  _text   the argument.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the parsed type.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, D_FTP_ERROR_MALFORMED, or
 *         D_FTP_ERROR_UNSUPPORTED for an unknown type code (reply 504).
 */
enum d_ftp_error d_ftp_type_parse(const char*        _text,
                                  size_t             _length,
                                  struct d_ftp_type* _out);

// 3.2    Parameter codes
//------------------------------------------------------------------------------
// Single-letter STRU and MODE arguments, read case-insensitively. Each
// returns false, leaving `_out` untouched, for an unknown code.
bool d_ftp_structure_from_code(char                  _code,
                               enum d_ftp_structure* _out);
bool d_ftp_mode_from_code(char                      _code,
                          enum d_ftp_transfer_mode* _out);


//==============================================================================
// 4.  ASCII TRANSFERS
//==============================================================================
// TYPE A transfers carry text with CR LF line endings (RFC 959 3.1.1.1). The
// converters are incremental: each converts as much of its input as fits,
// reports how much it consumed, and carries a split CR LF across calls in
// its state.


// 4.1    Line-ending conversion
//------------------------------------------------------------------------------
/**
 * @brief Prepares conversion state for a new transfer.
 *
 * @param[out] _state the state.
 */
void             d_ftp_ascii_init(struct d_ftp_ascii_state* _state);
/**
 * @brief Converts local text for sending: each LF not already preceded by
 *        CR becomes CR LF.
 *
 * @param[in,out] _state        the transfer's state.
 * @param[in]     _data         the local text; NULL only if `_length` is 0.
 * @param[in]     _length       its length in bytes.
 * @param[out]    _out_used     input bytes converted.
 * @param[in,out] _out          the buffer to append to.
 * @return D_FTP_OK, or D_FTP_ERROR_INVALID_ARGUMENT. A full buffer is not an
 *         error: `_out_used` falls short of `_length`.
 */
enum d_ftp_error d_ftp_ascii_to_network(struct d_ftp_ascii_state* _state,
                                        const char*               _data,
                                        size_t                    _length,
                                        size_t*                   _out_used,
                                        struct d_ftp_buffer*      _out);
/**
 * @brief Converts received text to local form: CR LF becomes LF, and the
 *        Telnet CR NUL becomes a bare CR.
 *
 * @param[in,out] _state        the transfer's state.
 * @param[in]     _data         the received text; NULL only if `_length`
 *                              is 0.
 * @param[in]     _length       its length in bytes.
 * @param[out]    _out_used     input bytes converted.
 * @param[in,out] _out          the buffer to append to.
 * @return D_FTP_OK, or D_FTP_ERROR_INVALID_ARGUMENT. A full buffer is not an
 *         error: `_out_used` falls short of `_length`.
 */
enum d_ftp_error d_ftp_ascii_from_network(struct d_ftp_ascii_state* _state,
                                          const char*               _data,
                                          size_t                    _length,
                                          size_t*                   _out_used,
                                          struct d_ftp_buffer*      _out);
/**
 * @brief Ends a received transfer, emitting the CR that a final chunk may
 *        have left pending.
 *
 * @param[in,out] _state the transfer's state.
 * @param[in,out] _out   the buffer to append to.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or
 *         D_FTP_ERROR_BUFFER_TOO_SMALL.
 */
enum d_ftp_error d_ftp_ascii_finish(struct d_ftp_ascii_state* _state,
                                    struct d_ftp_buffer*      _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_TRANSFER_H
