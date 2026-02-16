/******************************************************************************
* djinterp [core]                                                  string_fn.h
*
* Cross-platform variants of certain `string.h` functions.
* 
*
* path:      \inc\string_fn.h                                  
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.26
******************************************************************************/
/*
TABLE OF CONTENTS
=================
i.    Safe string copy & string concatenation
      ---------------------------------------
      a.  d_strcpy_s
      b.  d_strncpy_s
      c.  d_strcat_s
      d.  d_strncat_s
      
ii.   String duplication
      ------------------
      a.  d_strdup
      b.  d_strndup
      
iii.  Case-insensitive comparison
      ---------------------------
      a.  d_strcasecmp
      b.  d_strncasecmp
      
iv.   Thread-safe tokenization
      ------------------------
      a.  d_strtok_r
      
v.    String length with limit
      ------------------------
      a.  d_strnlen
      
vi.   Case-insensitive subtring search
      ---------------------------------
      a.  d_strcasestr
      
vii.  String case conversion
      ----------------------
      a.  d_strlwr
      b.  d_strupr

viii. String reversal
      ---------------
      a.  d_strrev

ix.   Character search that returns end pointer
      -----------------------------------------
      a.  d_strchrnul
      
x.    Thread-safe error string
      ------------------------
      a.  d_strerror_r
*/

#ifndef DJINTERP_STRING_FN_
#define DJINTERP_STRING_FN_ 1

#include <ctype.h>          // for `tolower`
#include <stddef.h>         // for `size_t`
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include ".\djinterp.h"
#include ".\dmemory.h"


// i.   Safe string copying & concatenation
int      d_strcpy_s(char* restrict _destination, size_t _dest_size, const char* restrict _src);
int      d_strncpy_s(char* restrict _destination, size_t _dest_size, const char* restrict _src, size_t _count);
int      d_strcat_s(char* restrict _destination, size_t _dest_size, const char* restrict _src);
int      d_strncat_s(char* restrict _destination, size_t _dest_size, const char* restrict _src, size_t _count);

// ii.   String duplication
char*    d_strdup(const char* _str);
char*    d_strndup(const char* _str, size_t _n);

// iii.  Case-insensitive comparison
int      d_strcasecmp(const char* _str2, const char* _s2);
int      d_strncasecmp(const char* _str2, const char* _s2, size_t _n);

// iv.   Thread-safe tokenization
char*    d_strtok_r(char* restrict _str, const char* restrict _delim, char** restrict _saveptr);

// v.    String length with limit
size_t   d_strnlen(const char* _str, size_t _maxlen);

// vi.   Case-insensitive substring search
char*    d_strcasestr(const char* _haystack, const char* _needle);

// vii.  String case conversion
char*    d_strlwr(char* _str);
char*    d_strupr(char* _str);

// viii. String reversal
char*    d_strrev(char* _str);

// ix.   Character search that returns end pointer
char*    d_strchrnul(const char* _str, int _c);

// x.    Thread-safe error string
int      d_strerror_r(int _errnum, char* _buf, size_t _buflen);


#endif    // DJINTERP_STRING_FN_
