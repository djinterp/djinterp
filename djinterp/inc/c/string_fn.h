/******************************************************************************
* djinterp [core]                                                  string_fn.h
*
* Cross-platform variants of certain `string.h` functions.
*   Provides fundamental string operations on raw `const char*` buffers with
* explicit lengths, suitable for use both standalone and as the underlying
* implementation layer for higher-level string types such as `d_string`.
*
*
* path:      \inc\string_fn.h                                  
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
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

xi.   Length-aware comparison
      ----------------------
      a.  d_strcmp_n
      b.  d_strncmp_n
      c.  d_strcasecmp_n
      d.  d_strncasecmp_n
      e.  d_strequals
      f.  d_strequals_nocase

xii.  Validation
      ----------
      a.  d_str_is_valid
      b.  d_str_is_ascii
      c.  d_str_is_numeric
      d.  d_str_is_alpha
      e.  d_str_is_alnum
      f.  d_str_is_whitespace

xiii. Counting
      --------
      a.  d_strcount_char
      b.  d_strcount_substr

xiv.  Hash
      ----
      a.  d_strhash

xv.   Prefix, suffix, and containment
      --------------------------------
      a.  d_strstartswith
      b.  d_strendswith
      c.  d_strcontains
      d.  d_strcontains_char

xvi.  Index-returning search
      ----------------------
      a.  d_strchr_index
      b.  d_strchr_index_from
      c.  d_strrchr_index
      d.  d_strstr_index
      e.  d_strstr_index_from
      f.  d_strrstr_index
      g.  d_strcasestr_index

xvii. In-place character replacement
      ------------------------------
      a.  d_strreplace_char
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


// d_index
//   type: unsigned index type for string position results.
// D_STRING_NPOS
//   constant: sentinel value indicating "not found" in index-returning
// search functions. Equal to the maximum value of d_index.
#ifndef D_STRING_NPOS
    #define D_STRING_NPOS ((d_index)-1)
#endif


// i.    safe string copying & concatenation
int      d_strcpy_s(char* restrict _destination, size_t _dest_size, const char* restrict _src);
int      d_strncpy_s(char* restrict _destination, size_t _dest_size, const char* restrict _src, size_t _count);
int      d_strcat_s(char* restrict _destination, size_t _dest_size, const char* restrict _src);
int      d_strncat_s(char* restrict _destination, size_t _dest_size, const char* restrict _src, size_t _count);

// ii.   string duplication
char*    d_strdup(const char* _str);
char*    d_strndup(const char* _str, size_t _n);

// iii.  case-insensitive comparison
int      d_strcasecmp(const char* _str2, const char* _s2);
int      d_strncasecmp(const char* _str2, const char* _s2, size_t _n);

// iv.   thread-safe tokenization
char*    d_strtok_r(char* restrict _str, const char* restrict _delim, char** restrict _saveptr);

// v.    string length with limit
size_t   d_strnlen(const char* _str, size_t _maxlen);

// vi.   case-insensitive substring search
char*    d_strcasestr(const char* _haystack, const char* _needle);

// vii.  string case conversion
char*    d_strlwr(char* _str);
char*    d_strupr(char* _str);

// viii. string reversal
char*    d_strrev(char* _str);

// ix.   character search that returns end pointer
char*    d_strchrnul(const char* _str, int _c);

// x.    thread-safe error string
int      d_strerror_r(int _errnum, char* _buf, size_t _buflen);

// xi.   length-aware comparison
int      d_strcmp_n(const char* _s1, size_t _s1_len, const char* _s2, size_t _s2_len);
int      d_strncmp_n(const char* _s1, size_t _s1_len, const char* _s2, size_t _s2_len, size_t _n);
int      d_strcasecmp_n(const char* _s1, size_t _s1_len, const char* _s2, size_t _s2_len);
int      d_strncasecmp_n(const char* _s1, size_t _s1_len, const char* _s2, size_t _s2_len, size_t _n);
bool     d_strequals(const char* _s1, size_t _s1_len, const char* _s2, size_t _s2_len);
bool     d_strequals_nocase(const char* _s1, size_t _s1_len, const char* _s2, size_t _s2_len);

// xii.  validation
bool     d_str_is_valid(const char* _text, size_t _length);
bool     d_str_is_ascii(const char* _text, size_t _length);
bool     d_str_is_numeric(const char* _text, size_t _length);
bool     d_str_is_alpha(const char* _text, size_t _length);
bool     d_str_is_alnum(const char* _text, size_t _length);
bool     d_str_is_whitespace(const char* _text, size_t _length);

// xiii. counting
size_t   d_strcount_char(const char* _str, size_t _len, char _c);
size_t   d_strcount_substr(const char* _str, size_t _len, const char* _substr);

// xiv.  hash
size_t   d_strhash(const char* _str, size_t _len);

// xv.   prefix, suffix, and containment
bool     d_strstartswith(const char* _str, size_t _str_len, const char* _prefix, size_t _prefix_len);
bool     d_strendswith(const char* _str, size_t _str_len, const char* _suffix, size_t _suffix_len);
bool     d_strcontains(const char* _str, size_t _str_len, const char* _substr);
bool     d_strcontains_char(const char* _str, size_t _str_len, char _c);

// xvi.  index-returning search
d_index  d_strchr_index(const char* _str, size_t _len, char _c);
d_index  d_strchr_index_from(const char* _str, size_t _len, char _c, size_t _start);
d_index  d_strrchr_index(const char* _str, size_t _len, char _c);
d_index  d_strstr_index(const char* _str, size_t _str_len, const char* _substr, size_t _substr_len);
d_index  d_strstr_index_from(const char* _str, size_t _str_len, const char* _substr, size_t _substr_len, size_t _start);
d_index  d_strrstr_index(const char* _str, size_t _str_len, const char* _substr, size_t _substr_len);
d_index  d_strcasestr_index(const char* _str, size_t _str_len, const char* _substr, size_t _substr_len);

// xvii. in-place character replacement
size_t   d_strreplace_char(char* _str, size_t _len, char _old, char _new);


#endif    // DJINTERP_STRING_FN_
