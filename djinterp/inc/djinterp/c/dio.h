/*******************************************************************************
* djinterp [c]                                                             dio.h
*
* Cross-platform variants of certain `stdio.h` functions.
*   Portable, safer wrappers for standard I/O: the secure formatted-input and
* formatted-output variants, bounded line input, and the stream error state.
* Stream positioning is c/fs's: d_file_tell_stream and d_file_seek_stream, in
* fs/file_seek.h, which d_fgetpos and d_fsetpos only wrapped (decision 39 of
* the register). The checked scanf family is the framework's
* own and the same everywhere. The secure printf and gets variants map to
* Annex K or MSVC's implementations where D_ENV_C_HAS_SCANF_S is set, and to
* the standard functions otherwise; see each contract for where the two
* differ.
*
*
* path:      /inc/djinterp/c/dio.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.19
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FORMATTED INPUT
    ---------------
    1.  The scanf family
2.  FORMATTED OUTPUT
    ----------------
    1.  The printf family
3.  CHARACTER AND STRING I/O
    ------------------------
    1.  Line and string I/O
4.  ERROR HANDLING
    --------------
    1.  Stream error state
*/

#ifndef DJINTERP_C_DIO_H
#define DJINTERP_C_DIO_H 1

// std
#include <stdarg.h>        // va_list
#include <stdio.h>         // FILE, size_t
// djinterp
#include "./djinterp.h"    // framework root
// re_std
#include "../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FORMATTED INPUT
//==============================================================================


// 1.1    The scanf family
//------------------------------------------------------------------------------
/**
 * @brief Reads formatted data from a string (sscanf equivalent).
 *
 * @param[in]  _buffer  the string to read.
 * @param[in]  _format  the format.
 * @param[out] ...      the objects receiving the fields.
 * @return the number of fields converted and assigned, or EOF on an input
 *         failure before the first conversion.
 */
int     d_sscanf(const char* _buffer,
                 const char* _format,
                 ...);
/**
 * @brief Reads formatted data from a string, with the C11 Annex K checks
 *        (sscanf_s equivalent).
 *
 * @note The framework's own implementation, the same on every platform: each
 *       directive goes alone to the C library's vsscanf, so the conversions'
 *       syntax and locale are the library's, and the checks are this
 *       function's. Each %c, %s and %[ that is not suppressed takes two
 *       arguments: its buffer, then the buffer's size in elements as a size_t
 *       (Annex K's rsize_t). Microsoft's own sscanf_s takes an unsigned int
 *       there; a call written for it passes a size_t here instead.
 * @note A field its buffer cannot hold, terminator included, is a matching
 *       failure, never a truncation, and a %s or %[ buffer of nonzero size is
 *       left holding the empty string. A %c that meets the end of input before
 *       its width is an input failure. A suppressed conversion counts as a
 *       conversion before an input failure, as the standard has it: "5"
 *       against "%*d%d" returns 0, where glibc's sscanf returns EOF. A wide
 *       conversion's width counts what the library counts: characters on
 *       glibc, bytes on musl.
 * @warning A null _buffer, _format or target, and a conversion specification
 *          the standard leaves undefined, are runtime-constraint violations:
 *          the call returns EOF and reads no further. No constraint handler
 *          is called.
 *
 * @param[in]  _buffer  the string to read.
 * @param[in]  _format  the format.
 * @param[out] ...      the objects receiving the fields, each %c, %s and %[
 *                      followed by its buffer's size in elements, a size_t.
 * @return the number of fields assigned; EOF on a runtime-constraint
 *         violation, or on an input failure before the first conversion.
 */
int     d_sscanf_s(const char* _buffer,
                   const char* _format,
                   ...);
/**
 * @brief Reads formatted data from a string with a va_list (vsscanf
 *        equivalent).
 *
 * @param[in] _buffer  the string to read.
 * @param[in] _format  the format.
 * @param[in] _argptr  the objects receiving the fields.
 * @return the number of fields converted and assigned, or EOF on an input
 *         failure before the first conversion.
 */
int     d_vsscanf(const char* _buffer,
                  const char* _format,
                  va_list     _argptr);
/**
 * @brief Reads formatted data from a string with a va_list, with the C11
 *        Annex K checks (vsscanf_s equivalent).
 *
 * @note Everything noted at d_sscanf_s() applies.
 *
 * @param[in] _buffer  the string to read.
 * @param[in] _format  the format.
 * @param[in] _argptr  the objects receiving the fields, with buffer sizes as
 *                     for d_sscanf_s().
 * @return the number of fields assigned; EOF on a runtime-constraint
 *         violation, or on an input failure before the first conversion.
 */
int     d_vsscanf_s(const char* _buffer,
                    const char* _format,
                    va_list     _argptr);
/**
 * @brief Reads formatted data from a stream (fscanf equivalent).
 *
 * @param[in,out] _stream  the stream to read.
 * @param[in]     _format  the format.
 * @param[out]    ...      the objects receiving the fields.
 * @return the number of fields converted and assigned, or EOF on an input
 *         failure before the first conversion.
 */
int     d_fscanf(FILE*       _stream,
                 const char* _format,
                 ...);
/**
 * @brief Reads formatted data from a stream, with the C11 Annex K checks
 *        (fscanf_s equivalent).
 *
 * @note Everything noted at d_sscanf_s() applies, over vfscanf. A stream is
 *       read once, so a %s or %[ is read with its width capped at its
 *       buffer's size less one, then checked for going on: a field that does
 *       has had that many characters consumed when the matching failure is
 *       reported. Where the library counts a wide conversion's width in
 *       bytes, as musl does, the cap is conservative: a multibyte field that
 *       would fit its buffer can fail.
 *
 * @param[in,out] _stream  the stream to read.
 * @param[in]     _format  the format.
 * @param[out]    ...      the objects receiving the fields, with buffer sizes
 *                         as for d_sscanf_s().
 * @return the number of fields assigned; EOF on a runtime-constraint
 *         violation, or on an input failure before the first conversion.
 */
int     d_fscanf_s(FILE*       _stream,
                   const char* _format,
                   ...);


//==============================================================================
// 2.  FORMATTED OUTPUT
//==============================================================================


// 2.1    The printf family
//------------------------------------------------------------------------------
/**
 * @brief Writes formatted data to a bounded buffer (sprintf_s equivalent).
 *
 * @param[out] _buffer  the buffer to write.
 * @param[in]  _size    the size of `_buffer` in bytes, terminator included.
 * @param[in]  _format  the format.
 * @param[in]  ...      the values to format.
 * @return as d_vsprintf_s().
 */
int     d_sprintf_s(char*       _buffer,
                    size_t      _size,
                    const char* _format,
                    ...);
/**
 * @brief Writes formatted data to a bounded buffer with a va_list (vsprintf_s
 *        equivalent).
 *
 * @note where D_ENV_C_HAS_SCANF_S is not set this is vsnprintf, which
 *       truncates the output where vsprintf_s would report the overflow.
 *
 * @param[out] _buffer  the buffer to write.
 * @param[in]  _size    the size of `_buffer` in bytes, terminator included.
 * @param[in]  _format  the format.
 * @param[in]  _argptr  the values to format.
 * @return with vsprintf_s, the number of characters written, excluding the
 *         terminator, or a negative value on failure; with the vsnprintf
 *         fallback, the length the full output would have had.
 */
int     d_vsprintf_s(char*       _buffer,
                     size_t      _size,
                     const char* _format,
                     va_list     _argptr);
/**
 * @brief Writes formatted data to a bounded buffer (snprintf equivalent).
 *
 * @param[out] _buffer  the buffer to write.
 * @param[in]  _size    the size of `_buffer` in bytes, terminator included.
 * @param[in]  _format  the format.
 * @param[in]  ...      the values to format.
 * @return as d_vsnprintf().
 */
int     d_snprintf(char*       _buffer,
                   size_t      _size,
                   const char* _format,
                   ...);
/**
 * @brief Writes formatted data to a bounded buffer with a va_list (vsnprintf
 *        equivalent).
 *
 * @note under MSVC before Visual Studio 2015 this is _vsnprintf, which returns
 *       -1 and leaves the buffer unterminated when the output does not fit.
 *
 * @param[out] _buffer  the buffer to write.
 * @param[in]  _size    the size of `_buffer` in bytes, terminator included.
 * @param[in]  _format  the format.
 * @param[in]  _argptr  the values to format.
 * @return the length the full output would have had, excluding the terminator,
 *         or a negative value on an encoding error.
 */
int     d_vsnprintf(char*       _buffer,
                    size_t      _size,
                    const char* _format,
                    va_list     _argptr);


//==============================================================================
// 3.  CHARACTER AND STRING I/O
//==============================================================================


// 3.1    Line and string I/O
//------------------------------------------------------------------------------
/**
 * @brief Reads a line from stdin into a bounded buffer (gets_s equivalent).
 *
 * @note where D_ENV_C_HAS_SCANF_S is not set this reads with fgets: a line
 *       longer than the buffer is then returned in parts rather than failing,
 *       the rest remaining in stdin.
 *
 * @param[out] _buffer  receives the line, without its newline.
 * @param[in]  _size    the size of `_buffer` in bytes.
 * @return `_buffer`, or `NULL` for a `NULL` `_buffer`, a zero `_size`, end of
 *         file, or a read error.
 */
char*   d_gets_s(char*  _buffer,
                 size_t _size);
/**
 * @brief Writes a string to a stream (fputs equivalent).
 *
 * @param[in]     _str     the string to write.
 * @param[in,out] _stream  the stream.
 * @return a non-negative value on success, or EOF on error.
 */
int     d_fputs(const char* _str,
                FILE*       _stream);
/**
 * @brief Reads a line from a stream (fgets equivalent).
 *
 * @param[out]    _str     receives the line, newline included.
 * @param[in]     _num     the size of `_str` in bytes.
 * @param[in,out] _stream  the stream.
 * @return `_str`, or `NULL` at end of file or on a read error.
 */
char*   d_fgets(char* _str,
                int   _num,
                FILE* _stream);


//==============================================================================
// 4.  ERROR HANDLING
//==============================================================================


// 4.1    Stream error state
//------------------------------------------------------------------------------
/**
 * @brief Prints a prefix and the description of errno to stderr (perror
 *        equivalent).
 *
 * @param[in] _s  the prefix; may be `NULL` or empty for none.
 */
void    d_perror(const char* _s);
/**
 * @brief Tests a stream's end-of-file indicator (feof equivalent).
 *
 * @param[in] _stream  the stream.
 * @return nonzero if the indicator is set.
 */
int     d_feof(FILE* _stream);
/**
 * @brief Tests a stream's error indicator (ferror equivalent).
 *
 * @param[in] _stream  the stream.
 * @return nonzero if the indicator is set.
 */
int     d_ferror(FILE* _stream);
/**
 * @brief Clears a stream's end-of-file and error indicators (clearerr
 *        equivalent).
 *
 * @param[in,out] _stream  the stream.
 */
void    d_clearerr(FILE* _stream);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DIO_H
