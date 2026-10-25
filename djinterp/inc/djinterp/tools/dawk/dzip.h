/*******************************************************************************
* djinterp [tools]                                                        dzip.h
*
* ZIP archives, read and rewritten.
*   Enough of ZIP to change a few entries of a package and keep the rest as
* they were: an archive is opened, its entries listed and read, some of them
* replaced or added, and the whole saved under another name. An entry nobody
* replaced is copied as it was stored, compressed bytes and checksum and all,
* so saving changes nothing a reader could see but what was replaced; a
* replaced or added entry is stored uncompressed, which every reader takes.
*   Reading inflates DEFLATE itself, so the unit is ISO C and needs no zlib.
* Not read: ZIP64, encrypted entries, and methods other than store and
* DEFLATE, each of which is refused with a reason rather than misread. An
* archive is read whole into neither memory nor a temporary file; only the
* entries asked for are, one at a time.
*   A stand-in, local to dawk: the framework's archive tier is the place for
* ZIP, and when it can read and rewrite a package without a third-party
* backend, this unit gives way to it.
*
* path:      /inc/djinterp/tools/dawk/dzip.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Archive
         1.  d_zip
    2.  Constants
         1.  D_ZIP_NONE
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Entries
    3.  Changes
    4.  Checksums
*/

#ifndef DJINTERP_TOOLS_DAWK_DZIP_H
#define DJINTERP_TOOLS_DAWK_DZIP_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Archive
//------------------------------------------------------------------------------
// 1.1.1
// d_zip
//   struct: an archive opened for reading, or made empty, with the changes
//   made to it since.  Layout is private.
struct d_zip;

// 1.2    Constants
//------------------------------------------------------------------------------
// 1.2.1
// D_ZIP_NONE
//   constant: the index d_zip_find gives for a name no entry has.
#define D_ZIP_NONE ((size_t)-1)


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Every operation that can fail writes why into a caller's buffer of
// `_capacity` bytes, which may be NULL when the reason is not wanted.


// 2.1    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Opens an archive and reads its directory.
 *
 * @note The file stays open until the archive is freed, since saving copies
 *       its unchanged entries from it.
 *
 * @param[in]  _path      the archive.
 * @param[out] _problem   receives why it could not be opened.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller releases the archive with `d_zip_free`.
 * @return the archive, or `NULL` when the file cannot be read, is not a ZIP
 *         archive, or uses a feature that is not read.
 */
struct d_zip* d_zip_open(const char* _path,
                         char*       _problem,
                         size_t      _capacity);
/**
 * @brief Makes an archive with no entries, to be filled with `d_zip_add`.
 *
 * @post   The caller releases the archive with `d_zip_free`.
 * @return the archive, or `NULL` when memory ran out.
 */
struct d_zip* d_zip_new(void);
/**
 * @brief Releases an archive, closing its file and discarding unsaved
 *        changes.
 *
 * @param[in] _zip  the archive; may be `NULL`.
 * @post Every name and index obtained from it is invalid.
 */
void          d_zip_free(struct d_zip* _zip);

// 2.2    Entries
//------------------------------------------------------------------------------
// The entries keep the order the archive's directory gives them, with added
// ones after; a name is the archive's own, `/` between its parts.
size_t        d_zip_count(const struct d_zip* _zip);
const char*   d_zip_name(const struct d_zip* _zip,
                         size_t              _index);
size_t        d_zip_find(const struct d_zip* _zip,
                         const char*         _name);
/**
 * @brief Reads one entry, as its replacement if it has one.
 *
 * @param[in,out] _zip         the archive.
 * @param[in]     _index       the entry.
 * @param[out]    _out_bytes   the bytes, allocated, with a terminating NUL
 *                             past the end that `_out_length` does not
 *                             count, so text can be read as a string.
 * @param[out]    _out_length  how many bytes there are.
 * @param[out]    _problem     receives why it could not be read.
 * @param[in]     _capacity    the size of `_problem`.
 * @post   The caller releases the bytes with `free`.
 * @return `true` when the entry was read and its checksum agreed; `false`
 *         otherwise, with `*_out_bytes` `NULL`.
 */
bool          d_zip_read(struct d_zip*   _zip,
                         size_t          _index,
                         unsigned char** _out_bytes,
                         size_t*         _out_length,
                         char*           _problem,
                         size_t          _capacity);

// 2.3    Changes
//------------------------------------------------------------------------------
/**
 * @brief Replaces an entry's bytes; the archive keeps a copy.
 *
 * @param[in,out] _zip     the archive.
 * @param[in]     _index   the entry.
 * @param[in]     _bytes   the new contents; may be `NULL` when `_length` is 0.
 * @param[in]     _length  how many bytes there are.
 * @return `true` if the entry will be saved with these bytes; `false` if the
 *         index names no entry or memory ran out.
 */
bool          d_zip_replace(struct d_zip*        _zip,
                            size_t               _index,
                            const unsigned char* _bytes,
                            size_t               _length);
/**
 * @brief Adds an entry after the others; the archive keeps a copy.
 *
 * @param[in,out] _zip     the archive.
 * @param[in]     _name    the entry's name, unique in the archive.
 * @param[in]     _bytes   the contents; may be `NULL` when `_length` is 0.
 * @param[in]     _length  how many bytes there are.
 * @return `true` if the entry was added; `false` if an entry has the name
 *         already, the name is empty, or memory ran out.
 */
bool          d_zip_add(struct d_zip*        _zip,
                        const char*          _name,
                        const unsigned char* _bytes,
                        size_t               _length);
/**
 * @brief Writes the archive, its changes included, to a file.
 *
 * @note The archive goes to `_path` only once it is whole: it is written
 *       beside it first and then renamed, so a failure leaves whatever was
 *       there before.
 *
 * @param[in,out] _zip       the archive.
 * @param[in]     _path      the file to write.  The archive's own file is
 *                           refused: on Windows an open file cannot be
 *                           replaced, and a save that works on one system
 *                           only is no rule to build on.
 * @param[out]    _problem   receives why it could not be written.
 * @param[in]     _capacity  the size of `_problem`.
 * @return `true` if the archive was written; `false` otherwise.
 */
bool          d_zip_save(struct d_zip* _zip,
                         const char*   _path,
                         char*         _problem,
                         size_t        _capacity);

// 2.4    Checksums
//------------------------------------------------------------------------------
// The IEEE 802.3 CRC-32 a ZIP entry carries: d_zip_crc32(0, ...) starts one,
// and passing a result back as `_crc` continues it over more bytes.
unsigned long d_zip_crc32(unsigned long        _crc,
                          const unsigned char* _bytes,
                          size_t               _length);


#endif  // DJINTERP_TOOLS_DAWK_DZIP_H
