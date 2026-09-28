/*******************************************************************************
* djinterp [net]                                                      ssh_wire.h
*
* SSH wire encoding (RFC 4251 section 5): readers and writers for the
* byte, boolean, uint32, uint64, string, mpint, and name-list types that
* SFTP, the agent protocol, and key blobs are written in.
*   Readers are bounds-checked and writers grow as needed. Both fail
* stickily, so a whole packet can be parsed or built and checked once.
*
*
* path:      /inc/djinterp/net/ssh/ssh_wire.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Cursors
         1.  d_ssh_reader
         2.  d_ssh_writer
2.  ENCODING
    --------
    1.  Reading
    2.  Writing
    3.  Name-lists
*/

#ifndef DJINTERP_NET_SSH_SSH_WIRE_H
#define DJINTERP_NET_SSH_SSH_WIRE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint8_t, uint32_t, uint64_t
// djinterp
#include "../../c/djinterp.h"  // framework root


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Cursors
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_reader
//   struct: a bounds-checked cursor over a received buffer. Failure is
// sticky: once a read fails, every later read fails too and stores zero, so a
// parser can read a whole packet and test d_ssh_reader_done once. The buffer
// is borrowed and must outlive the reader and every view read from it.
struct d_ssh_reader
{
    const unsigned char* data;      // the buffer; borrowed
    size_t               size;      // its length in bytes
    size_t               position;  // bytes consumed
    bool                 failed;    // a read failed; sticky
};

// 1.1.2
// d_ssh_writer
//   struct: a growable buffer that encoded values are appended to. Failure
// is sticky in the same way: after an allocation or validation failure every
// later write fails, so a builder can test the last result, or `failed`, once.
struct d_ssh_writer
{
    unsigned char* data;      // the encoded bytes; owned
    size_t         size;      // bytes written
    size_t         capacity;  // bytes allocated
    bool           failed;    // a write failed; sticky
};


//==============================================================================
// 2.  ENCODING
//==============================================================================
// RFC 4251 section 5: integers are big-endian; a string is a uint32 length and
// that many bytes; an mpint is a string holding a minimal two's-complement
// integer; a name-list is a string of comma-separated, non-empty,
// printable-ASCII names.


// 2.1    Reading
//------------------------------------------------------------------------------
/**
 * @brief Starts a reader over a received buffer.
 *
 * @param[out] _reader  the reader to initialize.
 * @param[in]  _data    the buffer, borrowed; may be `NULL` when `_size` is 0.
 * @param[in]  _size    its length in bytes.
 */
void
d_ssh_reader_init(struct d_ssh_reader* _reader,
                  const void*          _data,
                  size_t               _size);

// d_ssh_read_*
//   Each reads one value at the cursor and returns true; or returns false,
// stores zero (or an empty view), and marks the reader failed when the reader
// had already failed, an argument is NULL, the buffer ends first, or the value
// is malformed. A boolean is any nonzero byte. A string is a view into the
// buffer, not a copy. An mpint is its two's-complement bytes as sent, with
// `_negative` set from the top bit; one with a superfluous leading 0x00 or
// 0xFF byte is malformed, and zero is the empty view. A name-list is refused
// if it holds an empty name or a byte outside printable US-ASCII.
D_NODISCARD bool
d_ssh_read_byte(struct d_ssh_reader* _reader,
                uint8_t*             _value);
D_NODISCARD bool
d_ssh_read_boolean(struct d_ssh_reader* _reader,
                   bool*                _value);
D_NODISCARD bool
d_ssh_read_uint32(struct d_ssh_reader* _reader,
                  uint32_t*            _value);
D_NODISCARD bool
d_ssh_read_uint64(struct d_ssh_reader* _reader,
                  uint64_t*            _value);
D_NODISCARD bool
d_ssh_read_string(struct d_ssh_reader*  _reader,
                  const unsigned char** _data,
                  size_t*               _size);
D_NODISCARD bool
d_ssh_read_mpint(struct d_ssh_reader*  _reader,
                 const unsigned char** _bytes,
                 size_t*               _size,
                 bool*                 _negative);
D_NODISCARD bool
d_ssh_read_name_list(struct d_ssh_reader* _reader,
                     const char**         _list,
                     size_t*              _length);

// reader queries
//   d_ssh_reader_remaining is the number of unread bytes (0 once failed);
// d_ssh_reader_done is true when nothing failed and every byte was read.
D_NODISCARD size_t
d_ssh_reader_remaining(const struct d_ssh_reader* _reader);
D_NODISCARD bool
d_ssh_reader_done(const struct d_ssh_reader* _reader);

// 2.2    Writing
//------------------------------------------------------------------------------
/**
 * @brief Starts an empty writer; allocates nothing until the first write.
 *
 * @param[out] _writer  the writer to initialize.
 */
void
d_ssh_writer_init(struct d_ssh_writer* _writer);
/**
 * @brief Frees a writer's buffer and leaves it empty and usable again.
 *
 * @param[in,out] _writer  the writer, or `NULL`.
 */
void
d_ssh_writer_free(struct d_ssh_writer* _writer);

// d_ssh_write_*
//   Each appends one value and returns true; or returns false, appending
// nothing, and marks the writer failed when the writer had already failed,
// an argument is invalid, or memory runs out. d_ssh_write_mpint takes a
// non-negative big-endian magnitude, strips its leading zeros, and prepends a
// 0x00 byte when the top bit is set. d_ssh_write_name_list takes a
// NUL-terminated list and refuses what d_ssh_read_name_list refuses.
// d_ssh_write_raw appends bytes with no length prefix.
D_NODISCARD bool
d_ssh_write_byte(struct d_ssh_writer* _writer,
                 uint8_t              _value);
D_NODISCARD bool
d_ssh_write_boolean(struct d_ssh_writer* _writer,
                    bool                 _value);
D_NODISCARD bool
d_ssh_write_uint32(struct d_ssh_writer* _writer,
                   uint32_t             _value);
D_NODISCARD bool
d_ssh_write_uint64(struct d_ssh_writer* _writer,
                   uint64_t             _value);
D_NODISCARD bool
d_ssh_write_string(struct d_ssh_writer* _writer,
                   const void*          _data,
                   size_t               _size);
D_NODISCARD bool
d_ssh_write_cstring(struct d_ssh_writer* _writer,
                    const char*          _text);
D_NODISCARD bool
d_ssh_write_mpint(struct d_ssh_writer* _writer,
                  const unsigned char* _magnitude,
                  size_t               _size);
D_NODISCARD bool
d_ssh_write_name_list(struct d_ssh_writer* _writer,
                      const char*          _list);
D_NODISCARD bool
d_ssh_write_raw(struct d_ssh_writer* _writer,
                const void*          _data,
                size_t               _size);
/**
 * @brief Overwrites four already-written bytes with a uint32, such as a
 *        packet length reserved before its body was known.
 *
 * @param[in,out] _writer  the writer.
 * @param[in]     _offset  where the four bytes start.
 * @param[in]     _value   the value to store.
 * @return `true`; or `false` if the writer has failed or the four bytes were
 *         never written. A failed patch does not mark the writer failed.
 */
D_NODISCARD bool
d_ssh_writer_patch_uint32(struct d_ssh_writer* _writer,
                          size_t               _offset,
                          uint32_t             _value);

// 2.3    Name-lists
//------------------------------------------------------------------------------
/**
 * @brief Reports whether a name-list contains a name, compared exactly.
 *
 * @param[in] _list    the list; need not be NUL-terminated.
 * @param[in] _length  its length in bytes.
 * @param[in] _name    the NUL-terminated name to look for.
 * @return `true` if one of the comma-separated names equals `_name`;
 *         `false` otherwise or if an argument is `NULL`.
 */
D_NODISCARD bool
d_ssh_name_list_contains(const char* _list,
                         size_t      _length,
                         const char* _name);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_WIRE_H
