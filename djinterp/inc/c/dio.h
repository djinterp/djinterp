/******************************************************************************
* djinterp [core]                                                        dio.h
*
* Cross-platform variants of certain `stdio.h` functions.
* This header provides portable and secure implementations of standard I/O
* operations, focusing on safety wrappers for formatted input, large file
* support, and thread-safe operations. It ensures that secure variants like
* `sscanf_s` are available even on platforms that do not natively support
* C11 Annex K.
* 
*
* path:      \inc\dio.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.05.19
******************************************************************************/

#ifndef DJINTERP_IO_
#define DJINTERP_IO_

#include <stdarg.h>
#include <stdio.h>
#include ".\djinterp.h"
#include ".\dfile.h"
#include ".\djinterp.h"


// I.   formatted input (secure variants)
int     d_sscanf(const char* _buffer, const char* _format, ...);
int     d_sscanf_s(const char* _buffer, const char* _format, ...);
int     d_vsscanf(const char* _buffer, const char* _format, va_list _argptr);
int     d_vsscanf_s(const char* _buffer, const char* _format, va_list _argptr);
int     d_fscanf(FILE* _stream, const char* _format, ...);
int     d_fscanf_s(FILE* _stream, const char* _format, ...);

// II.  formatted output (secure variants)
int     d_sprintf_s(char* _buffer, size_t _size, const char* _format, ...);
int     d_vsprintf_s(char* _buffer, size_t _size, const char* _format, va_list _argptr);
int     d_snprintf(char* _buffer, size_t _size, const char* _format, ...);
int     d_vsnprintf(char* _buffer, size_t _size, const char* _format, va_list _argptr);

// III. character and string I/O
char*   d_gets_s(char* _buffer, size_t _size);
int     d_fputs(const char* _str, FILE* _stream);
char*   d_fgets(char* _str, int _num, FILE* _stream);

// IV.  large file stream positioning
int     d_fgetpos(FILE* _stream, d_off_t* _pos);
int     d_fsetpos(FILE* _stream, const d_off_t* _pos);
void    d_rewind(FILE* _stream);

// V.   error handling
void    d_perror(const char* _s);
int     d_feof(FILE* _stream);
int     d_ferror(FILE* _stream);
void    d_clearerr(FILE* _stream);


#endif  // DJINTERP_IO_