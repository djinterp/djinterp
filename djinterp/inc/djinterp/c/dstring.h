/*******************************************************************************
* djinterp [c]                                                         dstring.h
*
* Safe string type containing a textual value and its length.
*   Distinguished from `text_buffer.h` in that `d_string` is optimized for
* strings that may occasionally be resized but are not expected to undergo
* frequent modifications. Provides cross-platform string operations mirroring
* `string_fn.h` but operating on `d_string` types.
*
* path:      /inc/djinterp/c/dstring.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.09.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  String type
         1.  d_string
2.  LIFECYCLE
    ---------
    1.  Creation
    2.  Capacity management
    3.  Destruction
3.  ACCESS
    ------
    1.  Basic accessors
    2.  Character access
4.  COPYING AND DUPLICATION
    -----------------------
    1.  Safe copy
    2.  Safe concatenation
    3.  Duplication
5.  COMPARISON
    ----------
    1.  Case-sensitive comparison
    2.  Case-insensitive comparison
    3.  Equality
6.  SEARCH
    ------
    1.  Character search
    2.  Substring search
    3.  Case-insensitive search
    4.  Containment
    5.  Spans
7.  MODIFICATION
    ------------
    1.  Assignment
    2.  Append
    3.  Prepend
    4.  Insert
    5.  Erase and clear
    6.  Replace
8.  TRANSFORMATION
    --------------
    1.  Case conversion
    2.  Reversal
    3.  Trimming
    4.  Tokenization
    5.  Joining
9.  UTILITIES
    ---------
    1.  Validation
    2.  Counting and hashing
    3.  Error strings
    4.  Formatted strings
*/

#ifndef DJINTERP_C_DSTRING_H
#define DJINTERP_C_DSTRING_H 1

// std
#include <stdarg.h>       // va_list
#include <stdbool.h>      // bool
#include <stddef.h>       // size_t
// djinterp
#include "./djinterp.h"  // framework root


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    String type
//------------------------------------------------------------------------------
// 1.1.1
// d_string
//   struct: a safe string type containing a textual value and its length.
// The text is always null-terminated for compatibility with C string
// functions. Unlike text_buffer, `d_string` is intended for strings that may
// occasionally be resized but do not undergo frequent modifications.
struct d_string
{
    size_t size;      // length of string (excluding null terminator)
    char*  text;      // null-terminated string data
    size_t capacity;  // allocated capacity (including space for null)
};


//==============================================================================
// 2.  LIFECYCLE
//==============================================================================


// 2.1    Creation
//------------------------------------------------------------------------------
/**
 * @brief Creates an empty string with the default capacity.
 *
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if allocation failed.
 */
struct d_string* d_string_new(void);
/**
 * @brief Creates an empty string with the given initial capacity.
 *
 * @note a `_capacity` of `0` is raised to `1`, the room the terminator needs.
 *
 * @param[in] _capacity  initial capacity in bytes (including space for null
 *                       terminator).
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if allocation failed.
 */
struct d_string* d_string_new_with_capacity(size_t _capacity);
/**
 * @brief Creates a string from a null-terminated C string.
 *
 * @param[in] _cstr  null-terminated source string to copy.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_cstr` is `NULL` or allocation failed.
 */
struct d_string* d_string_new_from_cstr(const char* _cstr);
/**
 * @brief Creates a string from at most `_length` characters of a C string.
 *
 * @param[in] _cstr    the source string to copy from.
 * @param[in] _length  maximum number of characters to copy.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_cstr` is `NULL` or allocation failed.
 */
struct d_string* d_string_new_from_cstr_n(const char* _cstr,
                                          size_t      _length);
/**
 * @brief Creates a string from `_length` bytes of a buffer that need not be
 *        null-terminated.
 *
 * @param[in] _buffer  the source buffer to copy from.
 * @param[in] _length  number of bytes to copy.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_buffer` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_new_from_buffer(const char* _buffer,
                                          size_t      _length);
/**
 * @brief Creates a deep copy of a string.
 *
 * @param[in] _other  the string to copy.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_other` is `NULL` or allocation failed.
 */
struct d_string* d_string_new_copy(const struct d_string* _other);
/**
 * @brief Creates a string of `_length` copies of one character.
 *
 * @param[in] _length     number of times to repeat the character.
 * @param[in] _fill_char  character to fill with.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if allocation failed.
 */
struct d_string* d_string_new_fill(size_t _length,
                                   char   _fill_char);
/**
 * @brief Creates a string from printf-style formatting.
 *
 * @param[in] _format  printf-style format string.
 * @param[in] ...      the format arguments.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_format` is `NULL`, formatting fails,
 *         or allocation fails.
 */
struct d_string* d_string_new_formatted(const char* _format,
                                        ...);

// 2.2    Capacity management
//------------------------------------------------------------------------------
/**
 * @brief Ensures the capacity is at least `_capacity`; never shrinks.
 *
 * @param[in,out] _string    the string to modify.
 * @param[in]     _capacity  minimum capacity to reserve.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` if the capacity is now at least `_capacity`, or `false` if
 *         `_string` is `NULL` or allocation failed.
 */
bool             d_string_reserve(struct d_string* _string,
                                  size_t           _capacity);
/**
 * @brief Reduces the capacity to the current size plus the terminator.
 *
 * @param[in,out] _string  the string to shrink.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success or when there is nothing to shrink, or `false` if
 *         `_string` is `NULL` or allocation failed.
 */
bool             d_string_shrink_to_fit(struct d_string* _string);
/**
 * @brief Returns the allocated capacity, terminator included.
 *
 * @param[in] _string  the string to query.
 * @return the capacity, or `0` if `_string` is `NULL`.
 */
size_t           d_string_capacity(const struct d_string* _string);
/**
 * @brief Sets the size, truncating or padding as needed.
 *
 * @note padding characters are '\0', so a grown string contains embedded null
 *       characters until they are overwritten.
 *
 * @param[in,out] _string    the string to resize.
 * @param[in]     _new_size  new size for the `d_string`.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if `_string` is `NULL` or allocation
 *         failed.
 */
bool             d_string_resize(struct d_string* _string,
                                 size_t           _new_size);

// 2.3    Destruction
//------------------------------------------------------------------------------
/**
 * @brief Frees a string and its buffer.
 *
 * @param[in] _string  the string to free; may be `NULL`.
 * @post   `_string` and every pointer obtained from it are invalid.
 */
void             d_string_free(struct d_string* _string);
/**
 * @brief Frees a string's buffer but not the string itself.
 *
 * @param[in,out] _string  the string whose buffer to free; may be `NULL`.
 * @post   `_string` is empty with no buffer, and pointers previously obtained
 *         into its buffer are invalid.
 */
void             d_string_free_contents(struct d_string* _string);


//==============================================================================
// 3.  ACCESS
//==============================================================================


// 3.1    Basic accessors
//------------------------------------------------------------------------------
/**
 * @brief Returns the length, excluding the terminator.
 *
 * @param[in] _string  the string to query.
 * @return the length, or `0` if `_string` is `NULL`.
 */
size_t           d_string_length(const struct d_string* _string);
/**
 * @brief Alias for d_string_length().
 *
 * @param[in] _string  the string to query.
 * @return the length, or `0` if `_string` is `NULL`.
 */
size_t           d_string_size(const struct d_string* _string);
/**
 * @brief Returns the null-terminated text.
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _string  the string to access.
 * @return the text, or `NULL` if `_string` is `NULL`.
 */
const char*      d_string_cstr(const struct d_string* _string);
/**
 * @brief Returns the text for modification.
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _string  the string to access.
 * @return the text, or `NULL` if `_string` is `NULL`.
 */
char*            d_string_data(struct d_string* _string);
/**
 * @brief Tests whether a string is empty.
 *
 * @param[in] _string  the string to check.
 * @return `true` if `_string` is `NULL` or empty, `false` otherwise.
 */
bool             d_string_is_empty(const struct d_string* _string);

// 3.2    Character access
//------------------------------------------------------------------------------
/**
 * @brief Returns the character at an index.
 *
 * @param[in] _string  the string to access.
 * @param[in] _index   index of character (negative indices count from end).
 * @return the character, or '\0' if `_string` is `NULL` or `_index` is out of
 *         range.
 */
char             d_string_char_at(const struct d_string* _string,
                                  d_index                _index);
/**
 * @brief Replaces the character at an index.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _index   index of character (negative indices count from end).
 * @param[in]     _c       character to set.
 * @return `true` on success, or `false` if `_string` is `NULL` or `_index` is
 *         out of range.
 */
bool             d_string_set_char(struct d_string* _string,
                                   d_index          _index,
                                   char             _c);
/**
 * @brief Returns the first character.
 *
 * @param[in] _string  the string to access.
 * @return the first character, or '\0' if `_string` is `NULL` or empty.
 */
char             d_string_front(const struct d_string* _string);
/**
 * @brief Returns the last character.
 *
 * @param[in] _string  the string to access.
 * @return the last character, or '\0' if `_string` is `NULL` or empty.
 */
char             d_string_back(const struct d_string* _string);


//==============================================================================
// 4.  COPYING AND DUPLICATION
//==============================================================================
// Bounded copies modeled on C11 Annex K, and POSIX-style duplication.


// 4.1    Safe copy
//------------------------------------------------------------------------------
/**
 * @brief Copies one string over another (C11 `strcpy_s` equivalent).
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source `d_string`.
 * @pre    `_source` is not `_destination` itself.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_copy_s(struct d_string*       _destination,
                                 const struct d_string* _source);
/**
 * @brief Copies a C string over a string.
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source C string.
 * @pre    `_source` does not point into `_destination`'s own buffer, which
 *         growing may free.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_copy_cstr_s(struct d_string* _destination,
                                      const char*      _source);
/**
 * @brief Copies at most `_count` characters of one string over another.
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source `d_string`.
 * @param[in]     _count        maximum number of characters to copy.
 * @pre    `_source` is not `_destination` itself.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_ncopy_s(struct d_string*       _destination,
                                  const struct d_string* _source,
                                  size_t                 _count);
/**
 * @brief Copies at most `_count` characters of a C string over a string.
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source C string.
 * @param[in]     _count        maximum number of characters to copy.
 * @pre    `_source` does not point into `_destination`'s own buffer, which
 *         growing may free.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_ncopy_cstr_s(struct d_string* _destination,
                                       const char*      _source,
                                       size_t           _count);
/**
 * @brief Copies a string into a caller-supplied character buffer.
 *
 * @param[out] _destination       the destination buffer.
 * @param[in]  _destination_size  the size of `_destination` in bytes,
 *                                terminator included.
 * @param[in]  _source            the source `d_string`.
 * @retval 0       success.
 * @retval EINVAL  either pointer is `NULL`.
 * @retval ERANGE  `_destination_size` cannot hold the text and its terminator;
 *                 unless it is `0`, `_destination` is left holding an empty
 *                 string.
 */
int              d_string_to_buffer_s(char* D_RESTRICT       _destination,
                                      size_t                 _destination_size,
                                      const struct d_string* _source);

// 4.2    Safe concatenation
//------------------------------------------------------------------------------
/**
 * @brief Appends one string to another (C11 `strcat_s` equivalent).
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source string to append.
 * @pre    `_source` is not `_destination` itself.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_cat_s(struct d_string* D_RESTRICT       _destination,
                                const struct d_string* D_RESTRICT _source);
/**
 * @brief Appends a C string to a string.
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source C string to append.
 * @pre    `_source` does not point into `_destination`'s own buffer, which
 *         growing may free.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_cat_cstr_s(struct d_string* D_RESTRICT _destination,
                                     const char* D_RESTRICT      _source);
/**
 * @brief Appends at most `_count` characters of one string to another.
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source string to append.
 * @param[in]     _count        maximum number of characters to append.
 * @pre    `_source` is not `_destination` itself.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_ncat_s(struct d_string* D_RESTRICT       _destination,
                                 const struct d_string* D_RESTRICT _source,
                                 size_t                            _count);
/**
 * @brief Appends at most `_count` characters of a C string to a string.
 *
 * @param[in,out] _destination  the destination `d_string`.
 * @param[in]     _source       the source C string to append.
 * @param[in]     _count        maximum number of characters to append.
 * @pre    `_source` does not point into `_destination`'s own buffer, which
 *         growing may free.
 * @post   pointers previously obtained into `_destination`'s buffer may be
 *         invalidated.
 * @retval 0       success.
 * @retval EINVAL  either argument is `NULL`.
 * @retval ERANGE  the destination could not be grown (allocation failed).
 */
int              d_string_ncat_cstr_s(struct d_string* D_RESTRICT _destination,
                                      const char* D_RESTRICT      _source,
                                      size_t                      _count);

// 4.3    Duplication
//------------------------------------------------------------------------------
/**
 * @brief Duplicates a string (POSIX `strdup` equivalent).
 *
 * @param[in] _string  the string to duplicate.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_dup(const struct d_string* _string);
/**
 * @brief Duplicates at most `_n` characters of a string.
 *
 * @param[in] _string  the string to duplicate.
 * @param[in] _n       maximum number of characters to copy.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_ndup(const struct d_string* _string,
                               size_t                 _n);
/**
 * @brief Copies up to `_length` characters starting at an index.
 *
 * @note `_length` is clamped to the characters available.
 *
 * @param[in] _string  the source `d_string`.
 * @param[in] _start   starting index (negative counts from end).
 * @param[in] _length  number of characters to extract.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL`, `_start` is out of
 *         range, or allocation failed.
 */
struct d_string* d_string_substr(const struct d_string* _string,
                                 d_index                _start,
                                 size_t                 _length);


//==============================================================================
// 5.  COMPARISON
//==============================================================================


// 5.1    Case-sensitive comparison
//------------------------------------------------------------------------------
/**
 * @brief Compares two strings lexicographically, lengths included.
 *
 * @param[in] _s1  the first string being compared; may be `NULL`.
 * @param[in] _s2  the second string being compared; may be `NULL`.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal.
 */
int              d_string_compare(const struct d_string* _s1,
                                  const struct d_string* _s2);
/**
 * @brief Compares a string with a C string lexicographically, lengths included.
 *
 * @param[in] _s1  the string being compared; may be `NULL`.
 * @param[in] _s2  the C string being compared; may be `NULL`.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal.
 */
int              d_string_compare_cstr(const struct d_string* _s1,
                                       const char*            _s2);
/**
 * @brief Compares at most `_n` characters of two strings.
 *
 * @param[in] _s1  the first string being compared; may be `NULL`.
 * @param[in] _s2  the second string being compared; may be `NULL`.
 * @param[in] _n   the maximum number of characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_string_ncmp(const struct d_string* _s1,
                               const struct d_string* _s2,
                               size_t                 _n);
/**
 * @brief Compares at most `_n` characters of a string and a C string.
 *
 * @note unlike d_string_ncmp(), the comparison stops at the first null
 *       character of either string.
 *
 * @param[in] _s1  the string being compared; may be `NULL`.
 * @param[in] _s2  the C string being compared; may be `NULL`.
 * @param[in] _n   the maximum number of characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_string_ncmp_cstr(const struct d_string* _s1,
                                    const char*            _s2,
                                    size_t                 _n);

// 5.2    Case-insensitive comparison
//------------------------------------------------------------------------------
/**
 * @brief Compares two strings, ignoring case (POSIX `strcasecmp` equivalent).
 *
 * @param[in] _s1  the first string being compared; may be `NULL`.
 * @param[in] _s2  the second string being compared; may be `NULL`.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal.
 */
int              d_string_casecmp(const struct d_string* _s1,
                                  const struct d_string* _s2);
/**
 * @brief Compares a string with a C string, ignoring case.
 *
 * @param[in] _s1  the string being compared; may be `NULL`.
 * @param[in] _s2  the C string being compared; may be `NULL`.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal.
 */
int              d_string_casecmp_cstr(const struct d_string* _s1,
                                       const char*            _s2);
/**
 * @brief Compares at most `_n` characters of two strings, ignoring case.
 *
 * @param[in] _s1  the first string being compared; may be `NULL`.
 * @param[in] _s2  the second string being compared; may be `NULL`.
 * @param[in] _n   the maximum number of characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_string_ncasecmp(const struct d_string* _s1,
                                   const struct d_string* _s2,
                                   size_t                 _n);
/**
 * @brief Compares at most `_n` characters of a string and a C string, ignoring
 *        case.
 *
 * @param[in] _s1  the string being compared; may be `NULL`.
 * @param[in] _s2  the C string being compared; may be `NULL`.
 * @param[in] _n   the maximum number of characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_string_ncasecmp_cstr(const struct d_string* _s1,
                                        const char*            _s2,
                                        size_t                 _n);

// 5.3    Equality
//------------------------------------------------------------------------------
/**
 * @brief Tests two strings for equality.
 *
 * @param[in] _s1  the first string being compared; may be `NULL`.
 * @param[in] _s2  the second string being compared; may be `NULL`.
 * @return `true` if the strings are equal or both `NULL`, `false` otherwise.
 */
bool             d_string_equals(const struct d_string* _s1,
                                 const struct d_string* _s2);
/**
 * @brief Tests a string and a C string for equality.
 *
 * @param[in] _s1  the string being compared; may be `NULL`.
 * @param[in] _s2  the C string being compared; may be `NULL`.
 * @return `true` if the strings are equal or both `NULL`, `false` otherwise.
 */
bool             d_string_equals_cstr(const struct d_string* _s1,
                                      const char*            _s2);
/**
 * @brief Tests two strings for equality, ignoring case.
 *
 * @param[in] _s1  the first string being compared; may be `NULL`.
 * @param[in] _s2  the second string being compared; may be `NULL`.
 * @return `true` if the strings are equal ignoring case or both `NULL`, `false`
 *         otherwise.
 */
bool             d_string_equals_ignore_case(const struct d_string* _s1,
                                             const struct d_string* _s2);
/**
 * @brief Tests a string and a C string for equality, ignoring case.
 *
 * @param[in] _s1  the string being compared; may be `NULL`.
 * @param[in] _s2  the C string being compared; may be `NULL`.
 * @return `true` if the strings are equal ignoring case or both `NULL`, `false`
 *         otherwise.
 */
bool             d_string_equals_cstr_ignore_case(const struct d_string* _s1,
                                                  const char*            _s2);


//==============================================================================
// 6.  SEARCH
//==============================================================================


// 6.1    Character search
//------------------------------------------------------------------------------
/**
 * @brief Finds the first occurrence of a character.
 *
 * @note only the text before the first null character is searched.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       the character to find.
 * @return the index of the first occurrence, or `-1` if there is none or
 *         `_string` is `NULL`.
 */
d_index          d_string_find_char(const struct d_string* _string,
                                    char                   _c);
/**
 * @brief Finds the first occurrence of a character at or after an index.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       character to find.
 * @param[in] _start   starting index.
 * @return the index of the first occurrence at or after `_start`, or `-1` if
 *         there is none, `_start` is out of range, or `_string` is `NULL`.
 */
d_index          d_string_find_char_from(const struct d_string* _string,
                                         char                   _c,
                                         d_index                _start);
/**
 * @brief Finds the last occurrence of a character.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       character to find.
 * @return the index of the last occurrence, or `-1` if there is none or
 *         `_string` is `NULL`.
 */
d_index          d_string_rfind_char(const struct d_string* _string,
                                     char                   _c);
/**
 * @brief Finds the first occurrence of a character (`strchr` equivalent).
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       character to find.
 * @return a pointer to the first occurrence, or `NULL` if there is none or
 *         `_string` is `NULL`.
 */
char*            d_string_chr(const struct d_string* _string,
                              int                    _c);
/**
 * @brief Finds the last occurrence of a character (`strrchr` equivalent).
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       character to find.
 * @return a pointer to the last occurrence, or `NULL` if there is none or
 *         `_string` is `NULL`.
 */
char*            d_string_rchr(const struct d_string* _string,
                               int                    _c);
/**
 * @brief Finds a character, or the terminator if it is absent.
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       character to find.
 * @return a pointer to the first occurrence or to the terminator, or `NULL`
 *         only if `_string` is `NULL`.
 */
char*            d_string_chrnul(const struct d_string* _string,
                                 int                    _c);

// 6.2    Substring search
//------------------------------------------------------------------------------
/**
 * @brief Finds the first occurrence of a substring.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the string to search for.
 * @return the index of the first occurrence, `0` for an empty needle, or `-1`
 *         if there is none or either argument is `NULL`.
 */
d_index          d_string_find(const struct d_string* _haystack,
                               const struct d_string* _needle);
/**
 * @brief Finds the first occurrence of a C string.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the C string to search for.
 * @return the index of the first occurrence, `0` for an empty needle, or `-1`
 *         if there is none or either argument is `NULL`.
 */
d_index          d_string_find_cstr(const struct d_string* _haystack,
                                    const char*            _needle);
/**
 * @brief Finds the first occurrence of a substring at or after an index.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the string to search for.
 * @param[in] _start     starting index.
 * @return the index of the first occurrence at or after `_start`, or `-1` if
 *         there is none, `_start` is out of range, or either string is `NULL`.
 */
d_index          d_string_find_from(const struct d_string* _haystack,
                                    const struct d_string* _needle,
                                    d_index                _start);
/**
 * @brief Finds the first occurrence of a C string at or after an index.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the C string to search for.
 * @param[in] _start     starting index.
 * @return the index of the first occurrence at or after `_start`, or `-1` if
 *         there is none, `_start` is out of range, or either string is `NULL`.
 */
d_index          d_string_find_cstr_from(const struct d_string* _haystack,
                                         const char*            _needle,
                                         d_index                _start);
/**
 * @brief Finds the last occurrence of a substring.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the string to search for.
 * @return the index of the last occurrence, the length for an empty needle, or
 *         `-1` if there is none or either argument is `NULL`.
 */
d_index          d_string_rfind(const struct d_string* _haystack,
                                const struct d_string* _needle);
/**
 * @brief Finds the last occurrence of a C string.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the C string to search for.
 * @return the index of the last occurrence, the length for an empty needle, or
 *         `-1` if there is none or either argument is `NULL`.
 */
d_index          d_string_rfind_cstr(const struct d_string* _haystack,
                                     const char*            _needle);
/**
 * @brief Finds a C string (`strstr` equivalent).
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the C string to search for.
 * @return a pointer to the first occurrence, or `NULL` if there is none or
 *         either argument is `NULL`.
 */
char*            d_string_str(const struct d_string* _haystack,
                              const char*            _needle);

// 6.3    Case-insensitive search
//------------------------------------------------------------------------------
/**
 * @brief Finds the first occurrence of a substring, ignoring case.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the string to search for.
 * @return the index of the first occurrence, or `-1` if there is none or either
 *         argument is `NULL`.
 */
d_index          d_string_casefind(const struct d_string* _haystack,
                                   const struct d_string* _needle);
/**
 * @brief Finds the first occurrence of a C string, ignoring case.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the C string to search for.
 * @return the index of the first occurrence, or `-1` if there is none or either
 *         argument is `NULL`.
 */
d_index          d_string_casefind_cstr(const struct d_string* _haystack,
                                        const char*            _needle);
/**
 * @brief Finds a C string, ignoring case.
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _haystack  the string to search in.
 * @param[in] _needle    the C string to search for.
 * @return a pointer to the first occurrence, or `NULL` if there is none or
 *         either argument is `NULL`.
 */
char*            d_string_casestr(const struct d_string* _haystack,
                                  const char*            _needle);

// 6.4    Containment
//------------------------------------------------------------------------------
/**
 * @brief Tests whether a string contains a substring.
 *
 * @param[in] _string  the string to search in.
 * @param[in] _substr  the string to search for.
 * @return `true` if `_substr` occurs in `_string`, `false` otherwise or if
 *         either argument is `NULL`.
 */
bool             d_string_contains(const struct d_string* _string,
                                   const struct d_string* _substr);
/**
 * @brief Tests whether a string contains a C string.
 *
 * @param[in] _string  the string to search in.
 * @param[in] _substr  the C string to search for.
 * @return `true` if `_substr` occurs in `_string`, `false` otherwise or if
 *         either argument is `NULL`.
 */
bool             d_string_contains_cstr(const struct d_string* _string,
                                        const char*            _substr);
/**
 * @brief Tests whether a string contains a character.
 *
 * @param[in] _string  the string to search in.
 * @param[in] _c       character to search for.
 * @return `true` if `_c` occurs in `_string`, `false` otherwise or if `_string`
 *         is `NULL`.
 */
bool             d_string_contains_char(const struct d_string* _string,
                                        char                   _c);
/**
 * @brief Tests whether a string begins with a prefix.
 *
 * @param[in] _string  the string to check.
 * @param[in] _prefix  the string prefix.
 * @return `true` if `_string` begins with `_prefix`, `false` otherwise or if
 *         either argument is `NULL`.
 */
bool             d_string_starts_with(const struct d_string* _string,
                                      const struct d_string* _prefix);
/**
 * @brief Tests whether a string begins with a C string prefix.
 *
 * @param[in] _string  the string to check.
 * @param[in] _prefix  the C string prefix.
 * @return `true` if `_string` begins with `_prefix`, `false` otherwise or if
 *         either argument is `NULL`.
 */
bool             d_string_starts_with_cstr(const struct d_string* _string,
                                           const char*            _prefix);
/**
 * @brief Tests whether a string ends with a suffix.
 *
 * @param[in] _string  the string to check.
 * @param[in] _suffix  the string suffix.
 * @return `true` if `_string` ends with `_suffix`, `false` otherwise or if
 *         either argument is `NULL`.
 */
bool             d_string_ends_with(const struct d_string* _string,
                                    const struct d_string* _suffix);
/**
 * @brief Tests whether a string ends with a C string suffix.
 *
 * @param[in] _string  the string to check.
 * @param[in] _suffix  the C string suffix.
 * @return `true` if `_string` ends with `_suffix`, `false` otherwise or if
 *         either argument is `NULL`.
 */
bool             d_string_ends_with_cstr(const struct d_string* _string,
                                         const char*            _suffix);

// 6.5    Spans
//------------------------------------------------------------------------------
/**
 * @brief Measures the leading run of characters drawn from `_accept`.
 *
 * @param[in] _string  the string to scan.
 * @param[in] _accept  string of accepted characters.
 * @return the length of the run, or `0` if either argument is `NULL`.
 */
size_t           d_string_spn(const struct d_string* _string,
                              const char*            _accept);
/**
 * @brief Measures the leading run of characters not in `_reject`.
 *
 * @param[in] _string  the string to scan.
 * @param[in] _reject  string of rejected characters.
 * @return the length of the run, or `0` if either argument is `NULL`.
 */
size_t           d_string_cspn(const struct d_string* _string,
                               const char*            _reject);
/**
 * @brief Finds the first character drawn from `_accept`.
 *
 * @note the result points into the string's own buffer, and is invalidated by
 *       any call that grows, shrinks, or frees it.
 *
 * @param[in] _string  the string to search.
 * @param[in] _accept  string of characters to find.
 * @return a pointer to the first such character, or `NULL` if there is none or
 *         either argument is `NULL`.
 */
char*            d_string_pbrk(const struct d_string* _string,
                               const char*            _accept);


//==============================================================================
// 7.  MODIFICATION
//==============================================================================


// 7.1    Assignment
//------------------------------------------------------------------------------
/**
 * @brief Replaces a string's contents with a copy of another string.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _other   the source `d_string`.
 * @pre    `_other` is not `_string` itself.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL` or
 *         allocation failed.
 */
bool             d_string_assign(struct d_string*       _string,
                                 const struct d_string* _other);
/**
 * @brief Replaces a string's contents with a C string.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _cstr    the source C string.
 * @pre    `_cstr` does not point into `_string`'s own buffer, which growing may
 *         free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL` or
 *         allocation failed.
 */
bool             d_string_assign_cstr(struct d_string* _string,
                                      const char*      _cstr);
/**
 * @brief Replaces a string's contents with `_length` bytes of a buffer.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _buffer  the source buffer.
 * @param[in]     _length  number of bytes to copy.
 * @pre    `_buffer` does not point into `_string`'s own buffer, which growing
 *         may free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either pointer is `NULL` or
 *         allocation failed.
 */
bool             d_string_assign_buffer(struct d_string* _string,
                                        const char*      _buffer,
                                        size_t           _length);
/**
 * @brief Replaces a string's contents with `_count` copies of a character.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _count   number of times to repeat.
 * @param[in]     _c       character to assign.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if `_string` is `NULL` or allocation
 *         failed.
 */
bool             d_string_assign_char(struct d_string* _string,
                                      size_t           _count,
                                      char             _c);

// 7.2    Append
//------------------------------------------------------------------------------
/**
 * @brief Appends another string.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _other   the string to append.
 * @pre    `_other` is not `_string` itself.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL` or
 *         allocation failed.
 */
bool             d_string_append(struct d_string*       _string,
                                 const struct d_string* _other);
/**
 * @brief Appends a C string.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _cstr    the C string to append.
 * @pre    `_cstr` does not point into `_string`'s own buffer, which growing may
 *         free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL` or
 *         allocation failed.
 */
bool             d_string_append_cstr(struct d_string* _string,
                                      const char*      _cstr);
/**
 * @brief Appends `_length` bytes of a buffer.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _buffer  buffer to append.
 * @param[in]     _length  number of bytes to append.
 * @pre    `_buffer` does not point into `_string`'s own buffer, which growing
 *         may free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either pointer is `NULL` or
 *         allocation failed.
 */
bool             d_string_append_buffer(struct d_string* _string,
                                        const char*      _buffer,
                                        size_t           _length);
/**
 * @brief Appends one character.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _c       character to append.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if `_string` is `NULL` or allocation
 *         failed.
 */
bool             d_string_append_char(struct d_string* _string,
                                      char             _c);
/**
 * @brief Appends printf-style formatted text.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _format  format string.
 * @param[in]     ...      the format arguments.
 * @pre    no format argument points into `_string`'s own buffer, which
 *         formatting may overwrite or free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either pointer is `NULL`, formatting
 *         fails, or allocation fails.
 */
bool             d_string_append_formatted(struct d_string* _string,
                                           const char*      _format,
                                           ...);

// 7.3    Prepend
//------------------------------------------------------------------------------
/**
 * @brief Prepends another string.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _other   the string to prepend.
 * @pre    `_other` is not `_string` itself.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL` or
 *         allocation failed.
 */
bool             d_string_prepend(struct d_string*       _string,
                                  const struct d_string* _other);
/**
 * @brief Prepends a C string.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _cstr    the C string to prepend.
 * @pre    `_cstr` does not point into `_string`'s own buffer, which growing may
 *         free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL` or
 *         allocation failed.
 */
bool             d_string_prepend_cstr(struct d_string* _string,
                                       const char*      _cstr);
/**
 * @brief Prepends one character.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _c       character to prepend.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if `_string` is `NULL` or allocation
 *         failed.
 */
bool             d_string_prepend_char(struct d_string* _string,
                                       char             _c);

// 7.4    Insert
//------------------------------------------------------------------------------
/**
 * @brief Inserts another string before an index.
 *
 * @note `_index` may equal the length, which appends.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _index   insertion point.
 * @param[in]     _other   the string to insert.
 * @pre    `_other` is not `_string` itself.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL`, `_index`
 *         is out of range, or allocation failed.
 */
bool             d_string_insert(struct d_string*       _string,
                                 d_index                _index,
                                 const struct d_string* _other);
/**
 * @brief Inserts a C string before an index.
 *
 * @note `_index` may equal the length, which appends.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _index   insertion point.
 * @param[in]     _cstr    the C string to insert.
 * @pre    `_cstr` does not point into `_string`'s own buffer, which growing may
 *         free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL`, `_index`
 *         is out of range, or allocation failed.
 */
bool             d_string_insert_cstr(struct d_string* _string,
                                      d_index          _index,
                                      const char*      _cstr);
/**
 * @brief Inserts one character before an index.
 *
 * @note `_index` may equal the length, which appends.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _index   insertion point.
 * @param[in]     _c       character to insert.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if `_string` is `NULL`, `_index` is out
 *         of range, or allocation failed.
 */
bool             d_string_insert_char(struct d_string* _string,
                                      d_index          _index,
                                      char             _c);

// 7.5    Erase and clear
//------------------------------------------------------------------------------
/**
 * @brief Removes up to `_count` characters starting at an index.
 *
 * @note `_count` is clamped to the characters available.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _index   starting index.
 * @param[in]     _count   number of characters to erase.
 * @return `true` on success, or `false` if `_string` is `NULL` or `_index` is
 *         out of range.
 */
bool             d_string_erase(struct d_string* _string,
                                d_index          _index,
                                size_t           _count);
/**
 * @brief Removes the character at an index.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _index   index of character to erase.
 * @return `true` on success, or `false` if `_string` is `NULL` or `_index` is
 *         out of range.
 */
bool             d_string_erase_char(struct d_string* _string,
                                     d_index          _index);
/**
 * @brief Empties a string, keeping its capacity.
 *
 * @param[in,out] _string  the string to clear.
 */
void             d_string_clear(struct d_string* _string);

// 7.6    Replace
//------------------------------------------------------------------------------
/**
 * @brief Replaces up to `_count` characters at an index with another string.
 *
 * @note `_count` is clamped to the characters available.
 *
 * @param[in,out] _string       the string to modify.
 * @param[in]     _index        starting index of replacement.
 * @param[in]     _count        number of characters to replace.
 * @param[in]     _replacement  the string to insert.
 * @pre    `_replacement` is not `_string` itself.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL`, `_index`
 *         is out of range, or allocation failed.
 */
bool             d_string_replace(struct d_string*       _string,
                                  d_index                _index,
                                  size_t                 _count,
                                  const struct d_string* _replacement);
/**
 * @brief Replaces up to `_count` characters at an index with a C string.
 *
 * @note `_count` is clamped to the characters available.
 *
 * @param[in,out] _string       the string to modify.
 * @param[in]     _index        starting index of replacement.
 * @param[in]     _count        number of characters to replace.
 * @param[in]     _replacement  the C string to insert.
 * @pre    `_replacement` does not point into `_string`'s own buffer, which
 *         growing may free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, or `false` if either argument is `NULL`, `_index`
 *         is out of range, or allocation failed.
 */
bool             d_string_replace_cstr(struct d_string* _string,
                                       d_index          _index,
                                       size_t           _count,
                                       const char*      _replacement);
/**
 * @brief Replaces every occurrence of one substring with another.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _old     the string to find and replace.
 * @param[in]     _new     the string replacement.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, including when nothing matched, or `false` if an
 *         argument is `NULL`, `_old` is empty, or allocation failed.
 */
bool             d_string_replace_all(struct d_string*       _string,
                                      const struct d_string* _old,
                                      const struct d_string* _new);
/**
 * @brief Replaces every occurrence of one C string with another.
 *
 * @param[in,out] _string  the string to modify.
 * @param[in]     _old     the C string to find and replace.
 * @param[in]     _new     the C string replacement.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `true` on success, including when nothing matched, or `false` if an
 *         argument is `NULL`, `_old` is empty, or allocation failed.
 */
bool             d_string_replace_all_cstr(struct d_string* _string,
                                           const char*      _old,
                                           const char*      _new);
/**
 * @brief Replaces every occurrence of one character with another.
 *
 * @param[in,out] _string    the string to modify.
 * @param[in]     _old_char  character to replace.
 * @param[in]     _new_char  replacement character.
 * @return `true` on success, or `false` if `_string` is `NULL`.
 */
bool             d_string_replace_char(struct d_string* _string,
                                       char             _old_char,
                                       char             _new_char);


//==============================================================================
// 8.  TRANSFORMATION
//==============================================================================
// Each in-place transform has a copying counterpart that leaves its argument
// untouched.


// 8.1    Case conversion
//------------------------------------------------------------------------------
/**
 * @brief Converts a string to lowercase in place (`_strlwr` equivalent).
 *
 * @param[in,out] _string  the string to convert.
 * @return `true` on success, or `false` if `_string` or its text is `NULL`.
 */
bool             d_string_to_lower(struct d_string* _string);
/**
 * @brief Converts a string to uppercase in place (`_strupr` equivalent).
 *
 * @param[in,out] _string  the string to convert.
 * @return `true` on success, or `false` if `_string` or its text is `NULL`.
 */
bool             d_string_to_upper(struct d_string* _string);
/**
 * @brief Returns a lowercase copy of a string.
 *
 * @param[in] _string  the string to copy and convert.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_lower(const struct d_string* _string);
/**
 * @brief Returns an uppercase copy of a string.
 *
 * @param[in] _string  the string to copy and convert.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_upper(const struct d_string* _string);

// 8.2    Reversal
//------------------------------------------------------------------------------
/**
 * @brief Reverses a string in place (`_strrev` equivalent).
 *
 * @param[in,out] _string  the string to reverse.
 * @return `true` on success, or `false` if `_string` or its text is `NULL`.
 */
bool             d_string_reverse(struct d_string* _string);
/**
 * @brief Returns a reversed copy of a string.
 *
 * @param[in] _string  the string to copy and reverse.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_reversed(const struct d_string* _string);

// 8.3    Trimming
//------------------------------------------------------------------------------
/**
 * @brief Removes leading and trailing whitespace in place.
 *
 * @param[in,out] _string  the string to trim.
 * @return `true` on success, or `false` if `_string` is `NULL`.
 */
bool             d_string_trim(struct d_string* _string);
/**
 * @brief Removes leading whitespace in place.
 *
 * @param[in,out] _string  the string to trim.
 * @return `true` on success, or `false` if `_string` is `NULL`.
 */
bool             d_string_trim_left(struct d_string* _string);
/**
 * @brief Removes trailing whitespace in place.
 *
 * @param[in,out] _string  the string to trim.
 * @return `true` on success, or `false` if `_string` is `NULL`.
 */
bool             d_string_trim_right(struct d_string* _string);
/**
 * @brief Removes leading and trailing characters drawn from `_chars`, in place.
 *
 * @param[in,out] _string  the string to trim.
 * @param[in]     _chars   characters to trim.
 * @return `true` on success, or `false` if either argument is `NULL`.
 */
bool             d_string_trim_chars(struct d_string* _string,
                                     const char*      _chars);
/**
 * @brief Returns a copy with leading and trailing whitespace removed.
 *
 * @param[in] _string  the string to copy and trim.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_trimmed(const struct d_string* _string);
/**
 * @brief Returns a copy with leading whitespace removed.
 *
 * @param[in] _string  the string to copy and trim.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_trimmed_left(const struct d_string* _string);
/**
 * @brief Returns a copy with trailing whitespace removed.
 *
 * @param[in] _string  the string to copy and trim.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_string` is `NULL` or allocation
 *         failed.
 */
struct d_string* d_string_trimmed_right(const struct d_string* _string);

// 8.4    Tokenization
//------------------------------------------------------------------------------
/**
 * @brief Returns the next token (POSIX `strtok_r` equivalent).
 *
 * @warning tokenizes in place: delimiters in `_string`'s buffer are overwritten
 *          with '\0' while its size is left unchanged, so tokenize a copy if
 *          the string must stay intact.
 * @note    the result points into the string's own buffer, and is invalidated
 *          by any call that grows, shrinks, or frees it.
 *
 * @param[in,out] _string   the string to tokenize, or `NULL` to continue the
 *                          previous tokenization.
 * @param[in]     _delim    delimiter characters.
 * @param[in,out] _saveptr  save state pointer.
 * @return the next token, or `NULL` when there are no more tokens or `_delim`
 *         or `_saveptr` is `NULL`.
 */
char*            d_string_tokenize(struct d_string* _string,
                                   const char*      _delim,
                                   char**           _saveptr);
/**
 * @brief Splits a string into newly allocated tokens.
 *
 * @note runs of delimiters produce no empty tokens, and an empty `_string`
 *       yields a single empty token.
 *
 * @param[in]  _string  the string to split.
 * @param[in]  _delim   delimiter characters.
 * @param[out] _tokens  receives the token array.
 * @post   whenever `*_tokens` is set to a non-`NULL` array, the caller releases
 *         it with d_string_split_free(), even when the returned count is `0`.
 * @return the number of tokens stored in `*_tokens`, or `0` if there are none
 *         or the split failed.
 */
size_t           d_string_split(const struct d_string* _string,
                                const char*            _delim,
                                struct d_string***     _tokens);
/**
 * @brief Frees a token array produced by d_string_split().
 *
 * @param[in] _tokens  the array d_string_split() produced; may be `NULL`.
 * @param[in] _count   number of tokens in array.
 * @post   `_tokens` and every string in it are invalid.
 */
void             d_string_split_free(struct d_string** _tokens,
                                     size_t            _count);

// 8.5    Joining
//------------------------------------------------------------------------------
/**
 * @brief Joins an array of strings with a delimiter.
 *
 * @note `NULL` entries contribute nothing, but the delimiters around them are
 *       still written.
 *
 * @param[in] _strings    array of string pointers.
 * @param[in] _count      number of strings in array.
 * @param[in] _delimiter  delimiter to insert between strings.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the joined string, or `NULL` if `_strings` or `_delimiter` is `NULL`
 *         or allocation failed; a `_count` of `0` yields an empty string.
 */
struct d_string* d_string_join(const struct d_string* const* _strings,
                               size_t                        _count,
                               const char*                   _delimiter);
/**
 * @brief Joins an array of C strings with a delimiter.
 *
 * @note `NULL` entries contribute nothing, but the delimiters around them are
 *       still written.
 *
 * @param[in] _strings    array of C string pointers.
 * @param[in] _count      number of strings in array.
 * @param[in] _delimiter  delimiter to insert between strings.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the joined string, or `NULL` if `_strings` or `_delimiter` is `NULL`
 *         or allocation failed; a `_count` of `0` yields an empty string.
 */
struct d_string* d_string_join_cstr(const char* const* _strings,
                                    size_t             _count,
                                    const char*        _delimiter);
/**
 * @brief Concatenates a list of strings.
 *
 * @param[in] _count  number of strings to concatenate.
 * @param[in] ...     `_count` `const struct d_string*` arguments.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the concatenated string, or `NULL` if any argument is `NULL` or
 *         allocation failed; a `_count` of `0` yields an empty string.
 */
struct d_string* d_string_concat(size_t _count,
                                 ...);


//==============================================================================
// 9.  UTILITIES
//==============================================================================


// 9.1    Validation
//------------------------------------------------------------------------------
/**
 * @brief Tests a string's structural invariants.
 *
 * @param[in] _string  the string to check; may be `NULL`.
 * @return `true` if `_string` and its text are non-`NULL`, the text holds no
 *         '\0' before `size`, and a terminator sits at `size`; `false`
 *         otherwise.
 */
bool             d_string_is_valid(const struct d_string* _string);
/**
 * @brief Tests whether every character is 7-bit ASCII.
 *
 * @param[in] _string  the string to check; may be `NULL`.
 * @return `true` if every character is below 0x80, including for an empty
 *         string; `false` otherwise or if `_string` is `NULL`.
 */
bool             d_string_is_ascii(const struct d_string* _string);
/**
 * @brief Tests whether every character is a decimal digit.
 *
 * @param[in] _string  the string to check; may be `NULL`.
 * @return `true` if the string is non-empty and every character is a digit;
 *         `false` otherwise or if `_string` is `NULL`.
 */
bool             d_string_is_numeric(const struct d_string* _string);
/**
 * @brief Tests whether every character is alphabetic.
 *
 * @param[in] _string  the string to check; may be `NULL`.
 * @return `true` if the string is non-empty and every character is alphabetic;
 *         `false` otherwise or if `_string` is `NULL`.
 */
bool             d_string_is_alpha(const struct d_string* _string);
/**
 * @brief Tests whether every character is alphanumeric.
 *
 * @param[in] _string  the string to check; may be `NULL`.
 * @return `true` if the string is non-empty and every character is
 *         alphanumeric; `false` otherwise or if `_string` is `NULL`.
 */
bool             d_string_is_alnum(const struct d_string* _string);
/**
 * @brief Tests whether every character is whitespace.
 *
 * @param[in] _string  the string to check; may be `NULL`.
 * @return `true` if the string is non-empty and every character is whitespace;
 *         `false` otherwise or if `_string` is `NULL`.
 */
bool             d_string_is_whitespace(const struct d_string* _string);

// 9.2    Counting and hashing
//------------------------------------------------------------------------------
/**
 * @brief Counts occurrences of a character.
 *
 * @param[in] _string  the string to search.
 * @param[in] _c       character to count.
 * @return the number of occurrences, or `0` if `_string` or its text is `NULL`.
 */
size_t           d_string_count_char(const struct d_string* _string,
                                     char                   _c);
/**
 * @brief Counts non-overlapping occurrences of a C string.
 *
 * @param[in] _string  the string to search.
 * @param[in] _substr  substring to count.
 * @return the number of occurrences, or `0` if either argument is `NULL` or
 *         `_substr` is empty.
 */
size_t           d_string_count_substr(const struct d_string* _string,
                                       const char*            _substr);
/**
 * @brief Hashes a string with djb2.
 *
 * @param[in] _string  the string to hash.
 * @return the hash, or `0` if `_string` or its text is `NULL`.
 */
size_t           d_string_hash(const struct d_string* _string);

// 9.3    Error strings
//------------------------------------------------------------------------------
/**
 * @brief Describes an error number (POSIX `strerror` equivalent).
 *
 * @param[in] _errnum  error number.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the description, "Unknown error" if none is available, or `NULL` if
 *         allocation failed.
 */
struct d_string* d_string_error(int _errnum);
/**
 * @brief Describes an error number into an existing string (`strerror_r`
 *        equivalent).
 *
 * @param[in]     _errnum  error number.
 * @param[in,out] _string  receives the description, replacing its contents.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return `0` on success, `EINVAL` if `_string` is `NULL` or storing the text
 *         failed, or the nonzero code d_strerror_r() reported.
 */
int              d_string_error_r(int              _errnum,
                                  struct d_string* _string);

// 9.4    Formatted strings
//------------------------------------------------------------------------------
/**
 * @brief Creates a string from printf-style formatting.
 *
 * @param[in] _format  printf-style format string.
 * @param[in] ...      the format arguments.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_format` is `NULL`, formatting fails,
 *         or allocation fails.
 */
struct d_string* d_string_printf(const char* _format,
                                 ...);
/**
 * @brief Creates a string from printf-style formatting and a `va_list`.
 *
 * @note consumes `_args`; the caller must not reuse it, but still passes it to
 *       va_end().
 *
 * @param[in] _format  printf-style format string.
 * @param[in] _args    the format arguments.
 * @post   on success the caller owns the result and releases it with
 *         d_string_free().
 * @return the new string, or `NULL` if `_format` is `NULL`, formatting fails,
 *         or allocation fails.
 */
struct d_string* d_string_vprintf(const char* _format,
                                  va_list     _args);
/**
 * @brief Replaces a string's contents with printf-style formatted text.
 *
 * @param[in,out] _string  the string whose contents are replaced.
 * @param[in]     _format  printf-style format string.
 * @param[in]     ...      the format arguments.
 * @pre    no format argument points into `_string`'s own buffer, which
 *         formatting may overwrite or free.
 * @post   pointers previously obtained into `_string`'s buffer may be
 *         invalidated.
 * @return the number of characters written, or `-1` if either pointer is
 *         `NULL`, formatting fails, or allocation fails.
 */
int              d_string_sprintf(struct d_string* _string,
                                  const char*      _format,
                                  ...);


D_EXTERN_C_END


#endif  // DJINTERP_C_DSTRING_H
