/*******************************************************************************
* djinterp [c]                                                       string_fn.h
*
* Cross-platform variants of certain `string.h` functions.
*   Provides fundamental string operations on raw `const char*` buffers with
* explicit lengths, suitable for use both standalone and as the underlying
* implementation layer for higher-level string types such as `d_string`.
*
*
* path:      /inc/djinterp/c/string_fn.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Search results
         1.  D_STRING_NPOS
2.  BOUNDED COPYING AND DUPLICATION
    -------------------------------
    1.  Safe copy and concatenation
    2.  Duplication
3.  COMPARISON
    ----------
    1.  Case-insensitive comparison
    2.  Length-aware comparison
    3.  Equality
4.  SEARCH
    ------
    1.  C-string search
    2.  Prefix, suffix, and containment
    3.  Index-returning search
5.  TOKENIZATION AND LENGTH
    -----------------------
    1.  Tokenization
    2.  Bounded length
6.  TRANSFORMATION
    --------------
    1.  Case conversion
    2.  Reversal
    3.  Character replacement
7.  INSPECTION
    ----------
    1.  Validation
    2.  Counting
    3.  Hashing
8.  ERROR STRINGS
    -------------
    1.  Error descriptions
*/

#ifndef DJINTERP_C_STRING_FN_H
#define DJINTERP_C_STRING_FN_H 1

// std
#include <stdbool.h>     // bool
#include <stddef.h>      // size_t
// djinterp
#include "./djinterp.h"  // framework root


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Search results
//------------------------------------------------------------------------------
// 1.1.1
// D_STRING_NPOS
//   constant: the "not found" result of the index-returning searches. It is
// (d_index)-1, that is -1, since d_index is signed.
#ifndef D_STRING_NPOS
    #define D_STRING_NPOS ((d_index)-1)
#endif  // D_STRING_NPOS


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN


//==============================================================================
// 2.  BOUNDED COPYING AND DUPLICATION
//==============================================================================


// 2.1    Safe copy and concatenation
//------------------------------------------------------------------------------
/**
 * @brief Copies a string into a bounded buffer (C11 strcpy_s equivalent).
 *
 * @param[out] _destination  the buffer to copy into.
 * @param[in]  _dest_size    the size of `_destination` in bytes, terminator
 *                           included.
 * @param[in]  _source       the string to copy.
 * @pre    `_destination` and `_source` do not overlap.
 * @retval 0       success.
 * @retval EINVAL  `_destination` or `_source` is `NULL`; a usable
 *                 `_destination` is emptied.
 * @retval ERANGE  `_dest_size` is `0`, or the result does not fit; in the
 *                 latter case `_destination` is emptied rather than truncated.
 */
int              d_strcpy_s(char* D_RESTRICT       _destination,
                            size_t                 _dest_size,
                            const char* D_RESTRICT _source);
/**
 * @brief Copies at most `_count` characters of a string into a bounded buffer
 *        (C11 strncpy_s equivalent).
 *
 * @param[out] _destination  the buffer to copy into.
 * @param[in]  _dest_size    the size of `_destination` in bytes, terminator
 *                           included.
 * @param[in]  _source       the string to copy; it need not be terminated
 *                           within `_count` characters.
 * @param[in]  _count        the most characters to copy.
 * @pre    `_destination` and `_source` do not overlap.
 * @post   on success `_destination` is terminated.
 * @retval 0       success.
 * @retval EINVAL  `_destination` or `_source` is `NULL`; a usable
 *                 `_destination` is emptied.
 * @retval ERANGE  `_dest_size` is `0`, or the result does not fit; in the
 *                 latter case `_destination` is emptied rather than truncated.
 */
int              d_strncpy_s(char* D_RESTRICT       _destination,
                             size_t                 _dest_size,
                             const char* D_RESTRICT _source,
                             size_t                 _count);
/**
 * @brief Appends a string within a bounded buffer (C11 strcat_s equivalent).
 *
 * @param[in,out] _destination  the buffer holding the string to extend.
 * @param[in]     _dest_size    the size of `_destination` in bytes, terminator
 *                              included.
 * @param[in]     _source       the string to append.
 * @pre    `_destination` and `_source` do not overlap.
 * @retval 0       success.
 * @retval EINVAL  `_destination` or `_source` is `NULL`; a usable
 *                 `_destination` is emptied.
 * @retval ERANGE  `_dest_size` is `0`, `_destination` holds no terminator
 *                 within `_dest_size`, or the result does not fit; in the
 *                 latter two cases `_destination` is emptied.
 */
int              d_strcat_s(char* D_RESTRICT       _destination,
                            size_t                 _dest_size,
                            const char* D_RESTRICT _source);
/**
 * @brief Appends at most `_count` characters of a string within a bounded
 *        buffer (C11 strncat_s equivalent).
 *
 * @param[in,out] _destination  the buffer holding the string to extend.
 * @param[in]     _dest_size    the size of `_destination` in bytes, terminator
 *                              included.
 * @param[in]     _source       the string to append; it need not be terminated
 *                              within `_count` characters.
 * @param[in]     _count        the most characters to append.
 * @pre    `_destination` and `_source` do not overlap.
 * @retval 0       success.
 * @retval EINVAL  `_destination` or `_source` is `NULL`; a usable
 *                 `_destination` is emptied.
 * @retval ERANGE  `_dest_size` is `0`, `_destination` holds no terminator
 *                 within `_dest_size`, or the result does not fit; in the
 *                 latter two cases `_destination` is emptied.
 */
int              d_strncat_s(char* D_RESTRICT       _destination,
                             size_t                 _dest_size,
                             const char* D_RESTRICT _source,
                             size_t                 _count);

// 2.2    Duplication
//------------------------------------------------------------------------------
/**
 * @brief Duplicates a string (POSIX strdup equivalent).
 *
 * @param[in] _str  the string to duplicate.
 * @post   on success the caller owns the result and releases it with free().
 * @return the copy, or `NULL` if `_str` is `NULL` or allocation failed.
 */
char*            d_strdup(const char* _str);
/**
 * @brief Duplicates at most `_n` characters of a string (POSIX strndup
 *        equivalent).
 *
 * @param[in] _str  the string to duplicate; it need not be terminated within
 *                  `_n` characters.
 * @param[in] _n    the most characters to copy.
 * @post   on success the caller owns the result and releases it with free().
 * @return the terminated copy, or `NULL` if `_str` is `NULL` or allocation
 *         failed.
 */
char*            d_strndup(const char* _str,
                           size_t      _n);


//==============================================================================
// 3.  COMPARISON
//==============================================================================


// 3.1    Case-insensitive comparison
//------------------------------------------------------------------------------
/**
 * @brief Compares two strings, ignoring case (POSIX strcasecmp equivalent).
 *
 * @param[in] _s1  the first string; may be `NULL`.
 * @param[in] _s2  the second string; may be `NULL`.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal.
 */
int              d_strcasecmp(const char* _s1,
                              const char* _s2);
/**
 * @brief Compares at most `_n` characters of two strings, ignoring case (POSIX
 *        strncasecmp equivalent).
 *
 * @param[in] _s1  the first string; may be `NULL`.
 * @param[in] _s2  the second string; may be `NULL`.
 * @param[in] _n   the most characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_strncasecmp(const char* _s1,
                               const char* _s2,
                               size_t      _n);

// 3.2    Length-aware comparison
//------------------------------------------------------------------------------
/**
 * @brief Compares two strings of known length; embedded null characters take
 *        part.
 *
 * @param[in] _s1      the first string; may be `NULL`.
 * @param[in] _s1_len  the length of `_s1`, terminator excluded.
 * @param[in] _s2      the second string; may be `NULL`.
 * @param[in] _s2_len  the length of `_s2`, terminator excluded.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal. A string that is a prefix of
 *         the other orders first.
 */
int              d_strcmp_n(const char* _s1,
                            size_t      _s1_len,
                            const char* _s2,
                            size_t      _s2_len);
/**
 * @brief Compares at most `_n` characters of two strings of known length.
 *
 * @param[in] _s1      the first string; may be `NULL`.
 * @param[in] _s1_len  the length of `_s1`, terminator excluded.
 * @param[in] _s2      the second string; may be `NULL`.
 * @param[in] _s2_len  the length of `_s2`, terminator excluded.
 * @param[in] _n       the most characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_strncmp_n(const char* _s1,
                             size_t      _s1_len,
                             const char* _s2,
                             size_t      _s2_len,
                             size_t      _n);
/**
 * @brief Compares two strings of known length, ignoring case.
 *
 * @param[in] _s1      the first string; may be `NULL`.
 * @param[in] _s1_len  the length of `_s1`, terminator excluded.
 * @param[in] _s2      the second string; may be `NULL`.
 * @param[in] _s2_len  the length of `_s2`, terminator excluded.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal.
 */
int              d_strcasecmp_n(const char* _s1,
                                size_t      _s1_len,
                                const char* _s2,
                                size_t      _s2_len);
/**
 * @brief Compares at most `_n` characters of two strings of known length,
 *        ignoring case.
 *
 * @param[in] _s1      the first string; may be `NULL`.
 * @param[in] _s1_len  the length of `_s1`, terminator excluded.
 * @param[in] _s2      the second string; may be `NULL`.
 * @param[in] _s2_len  the length of `_s2`, terminator excluded.
 * @param[in] _n       the most characters to compare.
 * @return a value less than, equal to, or greater than zero as `_s1` orders
 *         before, equal to, or after `_s2`; a `NULL` string orders before any
 *         other, and two `NULL`s compare equal; always `0` when `_n` is `0`.
 */
int              d_strncasecmp_n(const char* _s1,
                                 size_t      _s1_len,
                                 const char* _s2,
                                 size_t      _s2_len,
                                 size_t      _n);

// 3.3    Equality
//------------------------------------------------------------------------------
/**
 * @brief Tests two strings of known length for equality.
 *
 * @param[in] _s1      the first string; may be `NULL`.
 * @param[in] _s1_len  the length of `_s1`, terminator excluded.
 * @param[in] _s2      the second string; may be `NULL`.
 * @param[in] _s2_len  the length of `_s2`, terminator excluded.
 * @return `true` if both are `NULL`, or both are non-`NULL` with equal lengths
 *         and contents; `false` otherwise.
 */
bool             d_strequals(const char* _s1,
                             size_t      _s1_len,
                             const char* _s2,
                             size_t      _s2_len);
/**
 * @brief Tests two strings of known length for equality, ignoring case.
 *
 * @param[in] _s1      the first string; may be `NULL`.
 * @param[in] _s1_len  the length of `_s1`, terminator excluded.
 * @param[in] _s2      the second string; may be `NULL`.
 * @param[in] _s2_len  the length of `_s2`, terminator excluded.
 * @return `true` if both are `NULL`, or both are non-`NULL` with equal lengths
 *         and case-insensitively equal contents; `false` otherwise.
 */
bool             d_strequals_nocase(const char* _s1,
                                    size_t      _s1_len,
                                    const char* _s2,
                                    size_t      _s2_len);


//==============================================================================
// 4.  SEARCH
//==============================================================================


// 4.1    C-string search
//------------------------------------------------------------------------------
/**
 * @brief Finds a substring, ignoring case (GNU strcasestr equivalent).
 *
 * @param[in] _haystack  the string to search.
 * @param[in] _needle    the string to find.
 * @return a pointer to the first occurrence, `_haystack` itself for an empty
 *         needle, or `NULL` if there is none or either argument is `NULL`.
 */
char*            d_strcasestr(const char* _haystack,
                              const char* _needle);
/**
 * @brief Finds a character, or the terminator if it is absent (GNU strchrnul
 *        equivalent).
 *
 * @param[in] _str  the string to search.
 * @param[in] _c    the character to find, converted to char.
 * @return a pointer to the first occurrence or to the terminator; `NULL` only
 *         if `_str` is `NULL`.
 */
char*            d_strchrnul(const char* _str,
                             int         _c);

// 4.2    Prefix, suffix, and containment
//------------------------------------------------------------------------------
/**
 * @brief Tests whether a string of known length begins with a prefix.
 *
 * @param[in] _str         the string to test.
 * @param[in] _str_len     the length of `_str`.
 * @param[in] _prefix      the prefix.
 * @param[in] _prefix_len  the length of `_prefix`; `0` always matches.
 * @return `true` if `_str` begins with `_prefix`; `false` otherwise or if
 *         either pointer is `NULL`.
 */
bool             d_strstartswith(const char* _str,
                                 size_t      _str_len,
                                 const char* _prefix,
                                 size_t      _prefix_len);
/**
 * @brief Tests whether a string of known length ends with a suffix.
 *
 * @param[in] _str         the string to test.
 * @param[in] _str_len     the length of `_str`.
 * @param[in] _suffix      the suffix.
 * @param[in] _suffix_len  the length of `_suffix`; `0` always matches.
 * @return `true` if `_str` ends with `_suffix`; `false` otherwise or if either
 *         pointer is `NULL`.
 */
bool             d_strendswith(const char* _str,
                               size_t      _str_len,
                               const char* _suffix,
                               size_t      _suffix_len);
/**
 * @brief Tests whether a string of known length contains a C string.
 *
 * @param[in] _str      the string to search.
 * @param[in] _str_len  the length of `_str`.
 * @param[in] _substr   the string to find.
 * @return `true` if `_substr` occurs in `_str`, including when it is empty;
 *         `false` otherwise or if either pointer is `NULL`.
 */
bool             d_strcontains(const char* _str,
                               size_t      _str_len,
                               const char* _substr);
/**
 * @brief Tests whether a string of known length contains a character.
 *
 * @param[in] _str      the string to search.
 * @param[in] _str_len  the length of `_str`.
 * @param[in] _c        the character to find.
 * @return `true` if `_c` occurs in `_str`; `false` otherwise or if `_str` is
 *         `NULL`.
 */
bool             d_strcontains_char(const char* _str,
                                    size_t      _str_len,
                                    char        _c);

// 4.3    Index-returning search
//------------------------------------------------------------------------------
/**
 * @brief Finds the first occurrence of a character in a buffer.
 *
 * @param[in] _str  the buffer to search.
 * @param[in] _len  the length of `_str`.
 * @param[in] _c    the character to find.
 * @return the index of the first occurrence, or D_STRING_NPOS if there is none
 *         or `_str` is `NULL`.
 */
d_index          d_strchr_index(const char* _str,
                                size_t      _len,
                                char        _c);
/**
 * @brief Finds the first occurrence of a character at or after an index.
 *
 * @param[in] _str    the buffer to search.
 * @param[in] _len    the length of `_str`.
 * @param[in] _c      the character to find.
 * @param[in] _start  the index to search from.
 * @return the index of the first occurrence, or D_STRING_NPOS if there is none,
 *         `_start` is not below `_len`, or `_str` is `NULL`.
 */
d_index          d_strchr_index_from(const char* _str,
                                     size_t      _len,
                                     char        _c,
                                     size_t      _start);
/**
 * @brief Finds the last occurrence of a character in a buffer.
 *
 * @param[in] _str  the buffer to search.
 * @param[in] _len  the length of `_str`.
 * @param[in] _c    the character to find.
 * @return the index of the last occurrence, or D_STRING_NPOS if there is none,
 *         `_len` is `0`, or `_str` is `NULL`.
 */
d_index          d_strrchr_index(const char* _str,
                                 size_t      _len,
                                 char        _c);
/**
 * @brief Finds the first occurrence of a substring in a buffer; embedded null
 *        characters take part.
 *
 * @param[in] _str         the buffer to search.
 * @param[in] _str_len     the length of `_str`.
 * @param[in] _substr      the buffer to find.
 * @param[in] _substr_len  the length of `_substr`.
 * @return the index of the first occurrence, `0` for an empty substring, or
 *         D_STRING_NPOS if there is none or either pointer is `NULL`.
 */
d_index          d_strstr_index(const char* _str,
                                size_t      _str_len,
                                const char* _substr,
                                size_t      _substr_len);
/**
 * @brief Finds the first occurrence of a substring at or after an index.
 *
 * @param[in] _str         the buffer to search.
 * @param[in] _str_len     the length of `_str`.
 * @param[in] _substr      the buffer to find.
 * @param[in] _substr_len  the length of `_substr`.
 * @param[in] _start       the index to search from.
 * @return the index of the first occurrence; `_start` itself for an empty
 *         substring when `_start` is at most `_str_len`; otherwise
 *         D_STRING_NPOS, as also when either pointer is `NULL`.
 */
d_index          d_strstr_index_from(const char* _str,
                                     size_t      _str_len,
                                     const char* _substr,
                                     size_t      _substr_len,
                                     size_t      _start);
/**
 * @brief Finds the last occurrence of a substring in a buffer.
 *
 * @param[in] _str         the buffer to search.
 * @param[in] _str_len     the length of `_str`.
 * @param[in] _substr      the buffer to find.
 * @param[in] _substr_len  the length of `_substr`.
 * @return the index of the last occurrence, `_str_len` for an empty substring,
 *         or D_STRING_NPOS if there is none or either pointer is `NULL`.
 */
d_index          d_strrstr_index(const char* _str,
                                 size_t      _str_len,
                                 const char* _substr,
                                 size_t      _substr_len);
/**
 * @brief Finds the first occurrence of a substring in a buffer, ignoring case.
 *
 * @note the comparison stops at a null character, so a match can be reported
 *       between buffers that differ only after an embedded '\0'.
 *
 * @param[in] _str         the buffer to search.
 * @param[in] _str_len     the length of `_str`.
 * @param[in] _substr      the buffer to find.
 * @param[in] _substr_len  the length of `_substr`.
 * @return the index of the first occurrence, `0` for an empty substring, or
 *         D_STRING_NPOS if there is none or either pointer is `NULL`.
 */
d_index          d_strcasestr_index(const char* _str,
                                    size_t      _str_len,
                                    const char* _substr,
                                    size_t      _substr_len);


//==============================================================================
// 5.  TOKENIZATION AND LENGTH
//==============================================================================


// 5.1    Tokenization
//------------------------------------------------------------------------------
/**
 * @brief Returns the next token of a string (POSIX strtok_r equivalent).
 *
 * @warning tokenizes in place: each token's terminating delimiter is
 *          overwritten with '\0'.
 *
 * @param[in,out] _str      the string to tokenize, or `NULL` to continue the
 *                          previous tokenization.
 * @param[in]     _delim    the delimiter characters.
 * @param[in,out] _saveptr  the tokenizer's state between calls.
 * @return the next token, or `NULL` when there are no more tokens or `_delim`
 *         or `_saveptr` is `NULL`.
 */
char*            d_strtok_r(char* D_RESTRICT       _str,
                            const char* D_RESTRICT _delim,
                            char** D_RESTRICT      _saveptr);

// 5.2    Bounded length
//------------------------------------------------------------------------------
/**
 * @brief Measures a string, examining at most `_maxlen` characters (POSIX
 *        strnlen equivalent).
 *
 * @param[in] _str     the string to measure; it need not be terminated.
 * @param[in] _maxlen  the most characters to examine.
 * @return the length, at most `_maxlen`; `0` if `_str` is `NULL`.
 */
size_t           d_strnlen(const char* _str,
                           size_t      _maxlen);


//==============================================================================
// 6.  TRANSFORMATION
//==============================================================================


// 6.1    Case conversion
//------------------------------------------------------------------------------
/**
 * @brief Converts a string to lowercase in place (Windows _strlwr equivalent).
 *
 * @param[in,out] _str  the string to convert.
 * @return `_str`, or `NULL` if it is `NULL`.
 */
char*            d_strlwr(char* _str);
/**
 * @brief Converts a string to uppercase in place (Windows _strupr equivalent).
 *
 * @param[in,out] _str  the string to convert.
 * @return `_str`, or `NULL` if it is `NULL`.
 */
char*            d_strupr(char* _str);

// 6.2    Reversal
//------------------------------------------------------------------------------
/**
 * @brief Reverses a string in place (Windows _strrev equivalent).
 *
 * @param[in,out] _str  the string to reverse.
 * @return `_str`, or `NULL` if it is `NULL`.
 */
char*            d_strrev(char* _str);

// 6.3    Character replacement
//------------------------------------------------------------------------------
/**
 * @brief Replaces every occurrence of one character in a buffer with another,
 *        in place.
 *
 * @param[in,out] _str  the buffer to modify.
 * @param[in]     _len  the length of `_str`.
 * @param[in]     _old  the character to replace.
 * @param[in]     _new  the character to substitute.
 * @return the number of replacements, or `0` if `_str` is `NULL`.
 */
size_t           d_strreplace_char(char*  _str,
                                   size_t _len,
                                   char   _old,
                                   char   _new);


//==============================================================================
// 7.  INSPECTION
//==============================================================================


// 7.1    Validation
//------------------------------------------------------------------------------
/**
 * @brief Tests that a buffer holds no null character among its first `_length`
 *        bytes.
 *
 * @note the byte at `_length`, where a terminator would sit, is not examined.
 *
 * @param[in] _text    the buffer to check; may be `NULL`.
 * @param[in] _length  the number of characters to check.
 * @return `true` if `_text` is non-`NULL` and none of its first `_length` bytes
 *         is '\0'; `false` otherwise.
 */
bool             d_str_is_valid(const char* _text,
                                size_t      _length);
/**
 * @brief Tests whether every character is 7-bit ASCII.
 *
 * @param[in] _text    the buffer to check; may be `NULL`.
 * @param[in] _length  the number of characters to check.
 * @return `true` if every character is below 0x80, including for an empty
 *         buffer; `false` otherwise or if `_text` is `NULL`.
 */
bool             d_str_is_ascii(const char* _text,
                                size_t      _length);
/**
 * @brief Tests whether every character is a decimal digit.
 *
 * @param[in] _text    the buffer to check; may be `NULL`.
 * @param[in] _length  the number of characters to check.
 * @return `true` if the buffer is non-empty and every character is a decimal
 *         digit; `false` otherwise or if `_text` is `NULL`.
 */
bool             d_str_is_numeric(const char* _text,
                                  size_t      _length);
/**
 * @brief Tests whether every character is alphabetic.
 *
 * @note classified by isalpha, which follows the current C locale.
 *
 * @param[in] _text    the buffer to check; may be `NULL`.
 * @param[in] _length  the number of characters to check.
 * @return `true` if the buffer is non-empty and every character is alphabetic;
 *         `false` otherwise or if `_text` is `NULL`.
 */
bool             d_str_is_alpha(const char* _text,
                                size_t      _length);
/**
 * @brief Tests whether every character is alphanumeric.
 *
 * @note classified by isalnum, which follows the current C locale.
 *
 * @param[in] _text    the buffer to check; may be `NULL`.
 * @param[in] _length  the number of characters to check.
 * @return `true` if the buffer is non-empty and every character is
 *         alphanumeric; `false` otherwise or if `_text` is `NULL`.
 */
bool             d_str_is_alnum(const char* _text,
                                size_t      _length);
/**
 * @brief Tests whether every character is whitespace.
 *
 * @note classified by isspace, which follows the current C locale.
 *
 * @param[in] _text    the buffer to check; may be `NULL`.
 * @param[in] _length  the number of characters to check.
 * @return `true` if the buffer is non-empty and every character is whitespace;
 *         `false` otherwise or if `_text` is `NULL`.
 */
bool             d_str_is_whitespace(const char* _text,
                                     size_t      _length);

// 7.2    Counting
//------------------------------------------------------------------------------
/**
 * @brief Counts the occurrences of a character in a buffer.
 *
 * @param[in] _str  the buffer to search.
 * @param[in] _len  the length of `_str`.
 * @param[in] _c    the character to count.
 * @return the number of occurrences, or `0` if `_str` is `NULL`.
 */
size_t           d_strcount_char(const char* _str,
                                 size_t      _len,
                                 char        _c);
/**
 * @brief Counts the non-overlapping occurrences of a C string in a buffer.
 *
 * @param[in] _str     the buffer to search.
 * @param[in] _len     the length of `_str`.
 * @param[in] _substr  the string to count.
 * @return the number of occurrences, or `0` if either pointer is `NULL` or
 *         `_substr` is empty.
 */
size_t           d_strcount_substr(const char* _str,
                                   size_t      _len,
                                   const char* _substr);

// 7.3    Hashing
//------------------------------------------------------------------------------
/**
 * @brief Hashes a buffer with djb2.
 *
 * @param[in] _str  the buffer to hash.
 * @param[in] _len  the length of `_str`.
 * @return the hash, or `0` if `_str` is `NULL`.
 */
size_t           d_strhash(const char* _str,
                           size_t      _len);


//==============================================================================
// 8.  ERROR STRINGS
//==============================================================================


// 8.1    Error descriptions
//------------------------------------------------------------------------------
/**
 * @brief Describes an error number into a buffer (POSIX strerror_r
 *        counterpart).
 *
 * @note only 0, EINVAL, and ERANGE are described; every other number reads
 *       "Unknown error". This is a fixed table, not the platform's strerror_r.
 *
 * @param[in]  _errnum  the error number to describe.
 * @param[out] _buf     receives the description.
 * @param[in]  _buflen  the size of `_buf` in bytes.
 * @retval 0       success.
 * @retval EINVAL  `_buf` is `NULL` or `_buflen` is `0`.
 * @retval ERANGE  the description does not fit; `_buf` is left untouched.
 */
int              d_strerror_r(int    _errnum,
                              char*  _buf,
                              size_t _buflen);


D_EXTERN_C_END


#endif  // DJINTERP_C_STRING_FN_H
