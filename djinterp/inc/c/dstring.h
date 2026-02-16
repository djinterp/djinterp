/******************************************************************************
* djinterp [core]                                                    dstring.h
*
*   Safe string type containing a textual value and its length. Distinguished
* from `text_buffer.h` in that `d_string` is optimized for strings that may 
* occasionally be resized but are not expected to undergo frequent 
* modifications. 
*   Provides cross-platform string operations mirroring `string_fn.h` but 
* operating on d_string types.
*
* 
* path:      \inc\dstring.h                                  
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#ifndef DJINTERP_STRING_
#define DJINTERP_STRING_ 1

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include ".\djinterp.h"
#include ".\string_fn.h"


// d_string
//   struct: a safe string type containing a textual value and its length.
// The text is always null-terminated for compatibility with C string 
// functions. Unlike text_buffer, d_string is intended for strings that may
// occasionally be resized but do not undergo frequent modifications.
struct d_string
{
    size_t size;        // length of string (excluding null terminator)
    char*  text;        // null-terminated string data
    size_t capacity;    // allocated capacity (including space for null)
};


// creation functions
struct d_string* d_string_new(void);
struct d_string* d_string_new_with_capacity(size_t _capacity);
struct d_string* d_string_new_from_cstr(const char* _cstr);
struct d_string* d_string_new_from_cstr_n(const char* _cstr, size_t _length);
struct d_string* d_string_new_from_buffer(const char* _buffer, size_t _length);
struct d_string* d_string_new_copy(const struct d_string* _other);
struct d_string* d_string_new_fill(size_t _length, char _fill_char);
struct d_string* d_string_new_formatted(const char* _format, ...);

// capacity management functions
bool             d_string_reserve(struct d_string* _str, size_t _capacity);
bool             d_string_shrink_to_fit(struct d_string* _str);
size_t           d_string_capacity(const struct d_string* _str);
bool             d_string_resize(struct d_string* _str, size_t _new_size);

// Access functions
//   basic accessors
size_t           d_string_length(const struct d_string* _str);
size_t           d_string_size(const struct d_string* _str);
const char*      d_string_cstr(const struct d_string* _str);
char*            d_string_data(struct d_string* _str);
bool             d_string_is_empty(const struct d_string* _str);
//   character access
char             d_string_char_at(const struct d_string* _str, d_index _index);
bool             d_string_set_char(struct d_string* _str, d_index _index, char _c);
char             d_string_front(const struct d_string* _str);
char             d_string_back(const struct d_string* _str);

// Safe copy functions (C11 `strcpy_s` equivalents)
int              d_string_copy_s(struct d_string* restrict _dest, const struct d_string* restrict _src);
int              d_string_copy_cstr_s(struct d_string* restrict _dest, const char* restrict _src);
int              d_string_ncopy_s(struct d_string* restrict _dest, const struct d_string* restrict _src, size_t _count);
int              d_string_ncopy_cstr_s(struct d_string* restrict _dest, const char* restrict _src, size_t _count);
int              d_string_to_buffer_s(char* restrict _dest, size_t _dest_size, const struct d_string* restrict _src);

// Concatenation functions (C11 `strcat_s` equivalents)
int              d_string_cat_s(struct d_string* restrict _dest, const struct d_string* restrict _src);
int              d_string_cat_cstr_s(struct d_string* restrict _dest, const char* restrict _src);
int              d_string_ncat_s(struct d_string* restrict _dest, const struct d_string* restrict _src, size_t _count);
int              d_string_ncat_cstr_s(struct d_string* restrict _dest, const char* restrict _src, size_t _count);

// Duplication functions (POSIX `strdup` equivalents)
struct d_string* d_string_dup(const struct d_string* _str);
struct d_string* d_string_ndup(const struct d_string* _str, size_t _n);
struct d_string* d_string_substr(const struct d_string* _str, d_index _start, size_t _length);

// Comparison functions
//   case-sensitive comparison
int              d_string_cmp(const struct d_string* _s1, const struct d_string* _s2);
int              d_string_cmp_cstr(const struct d_string* _s1, const char* _s2);
int              d_string_ncmp(const struct d_string* _s1, const struct d_string* _s2, size_t _n);
int              d_string_ncmp_cstr(const struct d_string* _s1, const char* _s2, size_t _n);
//   case-insensitive comparison (POSIX strcasecmp equivalents)
int              d_string_casecmp(const struct d_string* _s1, const struct d_string* _s2);
int              d_string_casecmp_cstr(const struct d_string* _s1, const char* _s2);
int              d_string_ncasecmp(const struct d_string* _s1, const struct d_string* _s2, size_t _n);
int              d_string_ncasecmp_cstr(const struct d_string* _s1, const char* _s2, size_t _n);
//   equality checks (convenience wrappers)
bool             d_string_equals(const struct d_string* _s1, const struct d_string* _s2);
bool             d_string_equals_cstr(const struct d_string* _s1, const char* _s2);
bool             d_string_equals_ignore_case(const struct d_string* _s1, const struct d_string* _s2);
bool             d_string_equals_cstr_ignore_case(const struct d_string* _s1, const char* _s2);

// Search functions
//  character search
d_index          d_string_find_char(const struct d_string* _str, char _c);
d_index          d_string_find_char_from(const struct d_string* _str, char _c, d_index _start);
d_index          d_string_rfind_char(const struct d_string* _str, char _c);
char*            d_string_chr(const struct d_string* _str, int _c);
char*            d_string_rchr(const struct d_string* _str, int _c);
char*            d_string_chrnul(const struct d_string* _str, int _c);
//   substring search
d_index          d_string_find(const struct d_string* _str, const struct d_string* _substr);
d_index          d_string_find_cstr(const struct d_string* _str, const char* _substr);
d_index          d_string_find_from(const struct d_string* _str, const struct d_string* _substr, d_index _start);
d_index          d_string_find_cstr_from(const struct d_string* _str, const char* _substr, d_index _start);
d_index          d_string_rfind(const struct d_string* _str, const struct d_string* _substr);
d_index          d_string_rfind_cstr(const struct d_string* _str, const char* _substr);
char*            d_string_str(const struct d_string* _haystack, const char* _needle);
//   case-insensitive search
d_index          d_string_casefind(const struct d_string* _str, const struct d_string* _substr);
d_index          d_string_casefind_cstr(const struct d_string* _str, const char* _substr);
char*            d_string_casestr(const struct d_string* _haystack, const char* _needle);
//   containment checks
bool             d_string_contains(const struct d_string* _str, const struct d_string* _substr);
bool             d_string_contains_cstr(const struct d_string* _str, const char* _substr);
bool             d_string_contains_char(const struct d_string* _str, char _c);
bool             d_string_starts_with(const struct d_string* _str, const struct d_string* _prefix);
bool             d_string_starts_with_cstr(const struct d_string* _str, const char* _prefix);
bool             d_string_ends_with(const struct d_string* _str, const struct d_string* _suffix);
bool             d_string_ends_with_cstr(const struct d_string* _str, const char* _suffix);
//   span functions
size_t           d_string_spn(const struct d_string* _str, const char* _accept);
size_t           d_string_cspn(const struct d_string* _str, const char* _reject);
char*            d_string_pbrk(const struct d_string* _str, const char* _accept);

// Modification functions (in-place)
// assignment
bool             d_string_assign(struct d_string* _str, const struct d_string* _other);
bool             d_string_assign_cstr(struct d_string* _str, const char* _cstr);
bool             d_string_assign_buffer(struct d_string* _str, const char* _buffer, size_t _length);
bool             d_string_assign_char(struct d_string* _str, size_t _count, char _c);
// append
bool             d_string_append(struct d_string* _str, const struct d_string* _other);
bool             d_string_append_cstr(struct d_string* _str, const char* _cstr);
bool             d_string_append_buffer(struct d_string* _str, const char* _buffer, size_t _length);
bool             d_string_append_char(struct d_string* _str, char _c);
bool             d_string_append_formatted(struct d_string* _str, const char* _format, ...);
// prepend
bool             d_string_prepend(struct d_string* _str, const struct d_string* _other);
bool             d_string_prepend_cstr(struct d_string* _str, const char* _cstr);
bool             d_string_prepend_char(struct d_string* _str, char _c);
// insert
bool             d_string_insert(struct d_string* _str, d_index _index, const struct d_string* _other);
bool             d_string_insert_cstr(struct d_string* _str, d_index _index, const char* _cstr);
bool             d_string_insert_char(struct d_string* _str, d_index _index, char _c);
// erase and clear
bool             d_string_erase(struct d_string* _str, d_index _index, size_t _count);
bool             d_string_erase_char(struct d_string* _str, d_index _index);
void             d_string_clear(struct d_string* _str);
// replace
bool             d_string_replace(struct d_string* _str, d_index _index, size_t _count, const struct d_string* _replacement);
bool             d_string_replace_cstr(struct d_string* _str, d_index _index, size_t _count, const char* _replacement);
bool             d_string_replace_all(struct d_string* _str, const struct d_string* _old, const struct d_string* _new);
bool             d_string_replace_all_cstr(struct d_string* _str, const char* _old, const char* _new);
bool             d_string_replace_char(struct d_string* _str, char _old_char, char _new_char);

// Case conversion functions (Windows `_strlwr`/`_strupr` equivalents)
bool             d_string_to_lower(struct d_string* _str);
bool             d_string_to_upper(struct d_string* _str);

// non-modifying versions that return new strings
struct d_string* d_string_lower(const struct d_string* _str);
struct d_string* d_string_upper(const struct d_string* _str);

// Reversal function (Windows `_strrev` equivalent)
bool             d_string_reverse(struct d_string* _str);
struct d_string* d_string_reversed(const struct d_string* _str);

// Trimming functions
//   in-place trimming
bool             d_string_trim(struct d_string* _str);
bool             d_string_trim_left(struct d_string* _str);
bool             d_string_trim_right(struct d_string* _str);
bool             d_string_trim_chars(struct d_string* _str, const char* _chars);
//   non-modifying versions
struct d_string* d_string_trimmed(const struct d_string* _str);
struct d_string* d_string_trimmed_left(const struct d_string* _str);
struct d_string* d_string_trimmed_right(const struct d_string* _str);

// Tokenization functions (POSIX `strtok_r` equivalent)
char*            d_string_tokenize(struct d_string* _str, const char* _delim, char** _saveptr);
size_t           d_string_split(const struct d_string* _str, const char* _delim, struct d_string*** _tokens);
void             d_string_split_free(struct d_string** _tokens, size_t _count);

// Join functions
struct d_string* d_string_join(const struct d_string* const* _strings, size_t _count, const char* _delimiter);
struct d_string* d_string_join_cstr(const char* const* _strings, size_t _count, const char* _delimiter);
struct d_string* d_string_concat(size_t _count, ...);

// Utility functions
//   validation
bool             d_string_is_valid(const struct d_string* _str);
bool             d_string_is_ascii(const struct d_string* _str);
bool             d_string_is_numeric(const struct d_string* _str);
bool             d_string_is_alpha(const struct d_string* _str);
bool             d_string_is_alnum(const struct d_string* _str);
bool             d_string_is_whitespace(const struct d_string* _str);
//   counting
size_t           d_string_count_char(const struct d_string* _str, char _c);
size_t           d_string_count_substr(const struct d_string* _str, const char* _substr);
//   hash
size_t           d_string_hash(const struct d_string* _str);

// Thread-safe error string (POSIX `strerror_r` equivalent)
struct d_string* d_string_error(int _errnum);
int              d_string_error_r(int _errnum, struct d_string* _str);

// Formatted string Functions
struct d_string* d_string_printf(const char* _format, ...);
struct d_string* d_string_vprintf(const char* _format, va_list _args);
int              d_string_sprintf(struct d_string* _str, const char* _format, ...);

// Destruction
void             d_string_free(struct d_string* _str);
void             d_string_free_contents(struct d_string* _str);


#endif  // DJINTERP_STRING_
