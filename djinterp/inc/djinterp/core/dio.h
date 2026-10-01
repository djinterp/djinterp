/*******************************************************************************
* djinterp [c]                                                             dio.h
*
* Cross-platform variants of certain `stdio.h` functions.
*   Portable, safer wrappers for standard I/O: the secure formatted-input and
* formatted-output variants, bounded line input, 64-bit stream positioning,
* and the stream error state. The secure variants map to Annex K or MSVC's
* implementations where D_STUDIO_HAS_SCANF_S is set, and to the standard
* functions otherwise; see each contract for where the two differ.
*
*
* path:      /inc/djinterp/c/dio.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.19
*                                                            revised: 2026.09.24
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
4.  STREAM POSITIONING
    ------------------
    1.  Large-file positioning
5.  ERROR HANDLING
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
#include "./fs/dfile.h"    // d_off_t


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
 * @brief Reads formatted data from a string (sscanf_s equivalent).
 *
 * @warning where D_STUDIO_HAS_SCANF_S is not set, this falls back to the plain
 *          function, which does not take the buffer-size argument the _s
 *          convention adds after each %s, %c, and %[ argument. A call written
 *          for the _s convention then misreads its arguments and can write
 *          through a size as if it were a pointer.
 *
 * @param[in]  _buffer  the string to read.
 * @param[in]  _format  the format.
 * @param[out] ...      the objects receiving the fields, each %s, %c, and %[
 *                      followed by its buffer size.
 * @return the number of fields converted and assigned, or EOF on an input
 *         failure before the first conversion.
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
 * @brief Reads formatted data from a string with a va_list (vsscanf_s
 *        equivalent).
 *
 * @warning the fallback caveat at d_sscanf_s() applies.
 *
 * @param[in] _buffer  the string to read.
 * @param[in] _format  the format.
 * @param[in] _argptr  the objects receiving the fields, with buffer sizes as
 *                     for d_sscanf_s().
 * @return the number of fields converted and assigned, or EOF on an input
 *         failure before the first conversion.
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
 * @brief Reads formatted data from a stream (fscanf_s equivalent).
 *
 * @warning the fallback caveat at d_sscanf_s() applies.
 *
 * @param[in,out] _stream  the stream to read.
 * @param[in]     _format  the format.
 * @param[out]    ...      the objects receiving the fields, with buffer sizes
 *                         as for d_sscanf_s().
 * @return the number of fields converted and assigned, or EOF on an input
 *         failure before the first conversion.
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
 * @note where D_STUDIO_HAS_SCANF_S is not set this is vsnprintf, which
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
 * @note where D_STUDIO_HAS_SCANF_S is not set this reads with fgets: a line
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
// 4.  STREAM POSITIONING
//==============================================================================


// 4.1    Large-file positioning
//------------------------------------------------------------------------------
//   d_file_rewind_stream lives in c/fs/file_seek.h, and returns int rather than
// void, because it can fail: seeking a pipe does. It was once declared here as
// well, which gave one symbol two incompatible declarations.
/**
 * @brief Reads a stream's position as a 64-bit offset.
 *
 * @note unlike fgetpos, the position is a plain d_off_t, so it can be stored
 *       and compared as a number.
 *
 * @param[in]  _stream  the stream.
 * @param[out] _pos     receives the offset from the start of the stream.
 * @return `0` on success, or `-1` for a `NULL` `_pos` or a failed read.
 */
int     d_fgetpos(FILE*    _stream,
                  d_off_t* _pos);
/**
 * @brief Moves a stream to a 64-bit offset from its start.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _pos     the offset from the start of the stream.
 * @return `0` on success, `-1` for a `NULL` `_pos`, or d_file_seek_stream()'s
 *         nonzero result on failure.
 */
int     d_fsetpos(FILE*          _stream,
                  const d_off_t* _pos);


//==============================================================================
// 5.  ERROR HANDLING
//==============================================================================


// 5.1    Stream error state
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


#endif  // DJINTERP_C_DIO_H
