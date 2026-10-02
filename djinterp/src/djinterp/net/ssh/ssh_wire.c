/*******************************************************************************
* djinterp [net]                                                      ssh_wire.c
*
*   Definitions for ssh_wire.h. Every read checks the length it is about
* to consume against what remains before consuming it, and every write
* reserves what it needs before touching the buffer, so neither can
* overrun, and a failure leaves nothing half-done.
*
*
* path:      /src/djinterp/net/ssh/ssh_wire.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // SIZE_MAX, UINT32_MAX, uint*_t
#include <stdlib.h>   // realloc, free
#include <string.h>   // memcmp, memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


// d_ssh_internal_empty
//   the view an empty or failed read stores, so that no view is ever NULL.
D_STATIC const unsigned char d_ssh_internal_empty[1] = { 0 };

/*
d_ssh_internal_fits32
  File-local: whether a size fits a uint32 length field. Widening first keeps
the comparison meaningful, and warning-free, where size_t is 32 bits.
*/
D_STATIC bool
d_ssh_internal_fits32(
    size_t _size
)
{
    const uint64_t size = (uint64_t)_size;

    return (size <= (uint64_t)UINT32_MAX);
}

/*
d_ssh_internal_take
  File-local: consumes `_count` bytes and returns where they start -- the
empty view for zero bytes of an empty buffer -- or returns NULL, marking the
reader failed, if it had already failed or fewer bytes remain.
*/
D_STATIC const unsigned char*
d_ssh_internal_take(
    struct d_ssh_reader* _reader,
    size_t               _count
)
{
    if ( (!_reader) ||
         (_reader->failed) )
    {
        return NULL;
    }

    // position never exceeds size, so the subtraction cannot wrap
    if (_count > (_reader->size - _reader->position))
    {
        _reader->failed = true;

        return NULL;
    }

    const unsigned char* const start =
        (_reader->data) ? (_reader->data + _reader->position)
                        : d_ssh_internal_empty;

    _reader->position += _count;

    return start;
}

/*
d_ssh_internal_reader_fail
  File-local: marks a reader failed, if there is one, and returns false.
*/
D_STATIC bool
d_ssh_internal_reader_fail(
    struct d_ssh_reader* _reader
)
{
    if (_reader)
    {
        _reader->failed = true;
    }

    return false;
}

/*
d_ssh_reader_init
  A NULL buffer with a nonzero size cannot be read, so such a reader starts
out failed instead of dereferencing it later.
*/
void
d_ssh_reader_init(
    struct d_ssh_reader* _reader,
    const void*          _data,
    size_t               _size
)
{
    if (!_reader)
    {
        return;
    }

    _reader->data     = (const unsigned char*)_data;
    _reader->size     = (_data) ? _size : 0;
    _reader->position = 0;
    _reader->failed   = ( (!_data) &&
                          (_size > 0) );

    return;
}

/*
d_ssh_read_byte
  The output is zeroed before anything can fail, so every path leaves it
defined.
*/
bool
d_ssh_read_byte(
    struct d_ssh_reader* _reader,
    uint8_t*             _value
)
{
    if (!_value)
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    *_value = 0;

    const unsigned char* const bytes = d_ssh_internal_take(_reader, 1);

    if (!bytes)
    {
        return false;
    }

    *_value = bytes[0];

    return true;
}

/*
d_ssh_read_boolean
  RFC 4251 reads any nonzero byte as true; only 1 is ever written.
*/
bool
d_ssh_read_boolean(
    struct d_ssh_reader* _reader,
    bool*                _value
)
{
    if (!_value)
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    uint8_t    byte = 0;
    const bool read = d_ssh_read_byte(_reader, &byte);

    *_value = (byte != 0);

    return read;
}

/*
d_ssh_read_uint32
  Big-endian, as every SSH integer is.
*/
bool
d_ssh_read_uint32(
    struct d_ssh_reader* _reader,
    uint32_t*            _value
)
{
    if (!_value)
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    *_value = 0;

    const unsigned char* const bytes = d_ssh_internal_take(_reader, 4);

    if (!bytes)
    {
        return false;
    }

    *_value = d_ssh_internal_load32(bytes);

    return true;
}

/*
d_ssh_read_uint64
  Two big-endian halves, high first.
*/
bool
d_ssh_read_uint64(
    struct d_ssh_reader* _reader,
    uint64_t*            _value
)
{
    if (!_value)
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    *_value = 0;

    const unsigned char* const bytes = d_ssh_internal_take(_reader, 8);

    if (!bytes)
    {
        return false;
    }

    *_value = ( ((uint64_t)d_ssh_internal_load32(bytes) << 32) |
                ((uint64_t)d_ssh_internal_load32(bytes + 4)) );

    return true;
}

/*
d_ssh_read_string
  The length is checked against what remains before any byte is consumed, so
a hostile length can neither overrun the buffer nor make anything allocate.
*/
bool
d_ssh_read_string(
    struct d_ssh_reader*  _reader,
    const unsigned char** _data,
    size_t*               _size
)
{
    if (_data)
    {
        *_data = d_ssh_internal_empty;
    }

    if (_size)
    {
        *_size = 0;
    }

    if ( (!_data) ||
         (!_size) )
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    uint32_t length = 0;

    if (!d_ssh_read_uint32(_reader, &length))
    {
        return false;
    }

    const unsigned char* const bytes = d_ssh_internal_take(_reader,
                                                           (size_t)length);

    if (!bytes)
    {
        return false;
    }

    *_data = bytes;
    *_size = (size_t)length;

    return true;
}

/*
d_ssh_read_mpint
  RFC 4251 allows exactly one encoding per value: a leading 0x00 byte only
when the next byte's top bit is set (the value would otherwise read as
negative), a leading 0xFF only when the next byte's top bit is clear, and no
bytes at all for zero.
*/
bool
d_ssh_read_mpint(
    struct d_ssh_reader*  _reader,
    const unsigned char** _bytes,
    size_t*               _size,
    bool*                 _negative
)
{
    if (_negative)
    {
        *_negative = false;
    }

    if ( (!_bytes)  ||
         (!_size)   ||
         (!_negative) )
    {
        return d_ssh_read_string(_reader, _bytes, _size) &&
               d_ssh_internal_reader_fail(_reader);
    }

    const unsigned char* bytes = NULL;
    size_t               size  = 0;

    if (!d_ssh_read_string(_reader, &bytes, &size))
    {
        return false;
    }

    // a superfluous sign byte, or a lone 0x00 for zero, is not minimal
    if ( (size > 0) &&
         ( ( (bytes[0] == 0x00) &&
             ( (size == 1) ||
               ((bytes[1] & 0x80) == 0) ) ) ||
           ( (bytes[0] == 0xFF) &&
             (size > 1)         &&
             ((bytes[1] & 0x80) != 0) ) ) )
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    *_bytes    = bytes;
    *_size     = size;
    *_negative = ( (size > 0) &&
                   ((bytes[0] & 0x80) != 0) );

    return true;
}

/*
d_ssh_read_name_list
  A view, like a string: the list is not NUL-terminated, so it comes with its
length.
*/
bool
d_ssh_read_name_list(
    struct d_ssh_reader* _reader,
    const char**         _list,
    size_t*              _length
)
{
    if (_list)
    {
        *_list = (const char*)d_ssh_internal_empty;
    }

    if (_length)
    {
        *_length = 0;
    }

    if ( (!_list) ||
         (!_length) )
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    const unsigned char* bytes = NULL;
    size_t               size  = 0;

    if (!d_ssh_read_string(_reader, &bytes, &size))
    {
        return false;
    }

    if (!d_ssh_internal_name_list_valid((const char*)bytes, size))
    {
        return d_ssh_internal_reader_fail(_reader);
    }

    *_list   = (const char*)bytes;
    *_length = size;

    return true;
}

/*
d_ssh_reader_remaining
  A failed reader has nothing left to offer.
*/
size_t
d_ssh_reader_remaining(
    const struct d_ssh_reader* _reader
)
{
    if ( (!_reader) ||
         (_reader->failed) )
    {
        return 0;
    }

    return _reader->size - _reader->position;
}

/*
d_ssh_reader_done
  True only for a packet parsed completely: trailing bytes are as suspect as
missing ones.
*/
bool
d_ssh_reader_done(
    const struct d_ssh_reader* _reader
)
{
    return ( (_reader)          &&
             (!_reader->failed) &&
             (_reader->position == _reader->size) );
}

/*
d_ssh_internal_writer_fail
  File-local: marks a writer failed, if there is one, and returns false.
*/
D_STATIC bool
d_ssh_internal_writer_fail(
    struct d_ssh_writer* _writer
)
{
    if (_writer)
    {
        _writer->failed = true;
    }

    return false;
}

/*
d_ssh_internal_reserve
  File-local: makes room for `_extra` more bytes. The capacity doubles, so a
run of small writes costs amortized constant time; near the limit it settles
for exactly enough. Failure marks the writer.
*/
D_STATIC bool
d_ssh_internal_reserve(
    struct d_ssh_writer* _writer,
    size_t               _extra
)
{
    if ( (!_writer) ||
         (_writer->failed) )
    {
        return false;
    }

    // the total must be representable before anything is allocated
    if (_extra > (SIZE_MAX - _writer->size))
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    const size_t needed = _writer->size + _extra;

    if (needed <= _writer->capacity)
    {
        return true;
    }

    size_t capacity = (_writer->capacity > 0) ? _writer->capacity : 64;

    // double until it fits
    while (capacity < needed)
    {
        capacity = (capacity > (SIZE_MAX / 2)) ? needed : (capacity * 2);
    }

    unsigned char* const data = realloc(_writer->data, capacity);

    if (!data)
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    _writer->data     = data;
    _writer->capacity = capacity;

    return true;
}

/*
d_ssh_internal_append
  File-local: appends bytes after reserving room for them.
*/
D_STATIC bool
d_ssh_internal_append(
    struct d_ssh_writer* _writer,
    const void*          _data,
    size_t               _size
)
{
    if (!d_ssh_internal_reserve(_writer, _size))
    {
        return false;
    }

    // an empty append may carry a NULL pointer, which memcpy forbids
    if (_size > 0)
    {
        memcpy(_writer->data + _writer->size, _data, _size);
        _writer->size += _size;
    }

    return true;
}

/*
d_ssh_writer_init
  Nothing is allocated until the first write, so an unused writer costs
nothing to discard.
*/
void
d_ssh_writer_init(
    struct d_ssh_writer* _writer
)
{
    if (!_writer)
    {
        return;
    }

    _writer->data     = NULL;
    _writer->size     = 0;
    _writer->capacity = 0;
    _writer->failed   = false;

    return;
}

/*
d_ssh_writer_free
  Reinitializes after freeing, so the writer can be reused at once.
*/
void
d_ssh_writer_free(
    struct d_ssh_writer* _writer
)
{
    if (!_writer)
    {
        return;
    }

    free(_writer->data);
    d_ssh_writer_init(_writer);

    return;
}

/*
d_ssh_write_byte
  The narrowest append.
*/
bool
d_ssh_write_byte(
    struct d_ssh_writer* _writer,
    uint8_t              _value
)
{
    const unsigned char byte = (unsigned char)_value;

    return d_ssh_internal_append(_writer, &byte, 1);
}

/*
d_ssh_write_boolean
  Writes 1 for true: readers accept any nonzero byte, but 1 is canonical.
*/
bool
d_ssh_write_boolean(
    struct d_ssh_writer* _writer,
    bool                 _value
)
{
    return d_ssh_write_byte(_writer, (uint8_t)((_value) ? 1 : 0));
}

/*
d_ssh_write_uint32
  Big-endian.
*/
bool
d_ssh_write_uint32(
    struct d_ssh_writer* _writer,
    uint32_t             _value
)
{
    unsigned char bytes[4];

    d_ssh_internal_store32(bytes, _value);

    return d_ssh_internal_append(_writer, bytes, sizeof(bytes));
}

/*
d_ssh_write_uint64
  Big-endian, high half first.
*/
bool
d_ssh_write_uint64(
    struct d_ssh_writer* _writer,
    uint64_t             _value
)
{
    unsigned char bytes[8];

    d_ssh_internal_store64(bytes, _value);

    return d_ssh_internal_append(_writer, bytes, sizeof(bytes));
}

/*
d_ssh_write_string
  Room for the length and the bytes is reserved together, so a failure leaves
neither behind.
*/
bool
d_ssh_write_string(
    struct d_ssh_writer* _writer,
    const void*          _data,
    size_t               _size
)
{
    // the bytes must exist, and their count must fit the uint32 prefix
    if ( ( (!_data) &&
           (_size > 0) )                          ||
         (!d_ssh_internal_fits32(_size))          ||
         (!d_ssh_internal_reserve(_writer, _size + 4)) )
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    unsigned char length[4];

    d_ssh_internal_store32(length, (uint32_t)_size);

    return ( (d_ssh_internal_append(_writer, length, sizeof(length))) &&
             (d_ssh_internal_append(_writer, _data, _size)) );
}

/*
d_ssh_write_cstring
  A string whose length is the text's.
*/
bool
d_ssh_write_cstring(
    struct d_ssh_writer* _writer,
    const char*          _text
)
{
    if (!_text)
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    return d_ssh_write_string(_writer, _text, strlen(_text));
}

/*
d_ssh_write_mpint
  Leading zeros are stripped so equal values encode identically, and a 0x00
byte is prepended when the top bit is set, which would otherwise make the
magnitude read as negative.
*/
bool
d_ssh_write_mpint(
    struct d_ssh_writer* _writer,
    const unsigned char* _magnitude,
    size_t               _size
)
{
    if ( (!_magnitude) &&
         (_size > 0) )
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    const unsigned char* bytes = _magnitude;
    size_t               size  = _size;

    // zero is the empty string
    while ( (size > 0) &&
            (bytes[0] == 0) )
    {
        bytes++;
        size--;
    }

    const size_t pad = ( (size > 0) &&
                         ((bytes[0] & 0x80) != 0) ) ? 1 : 0;

    if ( (size > (SIZE_MAX - 5))                          ||
         (!d_ssh_internal_fits32(size + pad))             ||
         (!d_ssh_internal_reserve(_writer, size + pad + 4)) )
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    const unsigned char zero = 0;
    unsigned char       length[4];

    d_ssh_internal_store32(length, (uint32_t)(size + pad));

    return ( (d_ssh_internal_append(_writer, length, sizeof(length))) &&
             (d_ssh_internal_append(_writer, &zero, pad))             &&
             (d_ssh_internal_append(_writer, bytes, size)) );
}

/*
d_ssh_write_name_list
  Validated before anything is written: a malformed list here would be a
protocol violation on the wire.
*/
bool
d_ssh_write_name_list(
    struct d_ssh_writer* _writer,
    const char*          _list
)
{
    if ( (!_list) ||
         (!d_ssh_internal_name_list_valid(_list, strlen(_list))) )
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    return d_ssh_write_cstring(_writer, _list);
}

/*
d_ssh_write_raw
  No length prefix: for values already encoded, or framing done by hand.
*/
bool
d_ssh_write_raw(
    struct d_ssh_writer* _writer,
    const void*          _data,
    size_t               _size
)
{
    if ( (!_data) &&
         (_size > 0) )
    {
        return d_ssh_internal_writer_fail(_writer);
    }

    return d_ssh_internal_append(_writer, _data, _size);
}

/*
d_ssh_writer_patch_uint32
  Refuses to write past what was written, so a patch cannot grow the buffer
or touch bytes the caller never supplied.
*/
bool
d_ssh_writer_patch_uint32(
    struct d_ssh_writer* _writer,
    size_t               _offset,
    uint32_t             _value
)
{
    // the four bytes must already have been written
    if ( (!_writer)                     ||
         (_writer->failed)              ||
         (_offset > _writer->size)      ||
         ((_writer->size - _offset) < 4) )
    {
        return false;
    }

    d_ssh_internal_store32(_writer->data + _offset, _value);

    return true;
}

/*
d_ssh_name_list_contains
  Compares whole names, so "publickey" does not match inside
"publickey-hostbound@openssh.com".
*/
bool
d_ssh_name_list_contains(
    const char* _list,
    size_t      _length,
    const char* _name
)
{
    if ( (!_list) ||
         (!_name) )
    {
        return false;
    }

    const size_t name_length = strlen(_name);

    // an empty name is never a member
    if (name_length == 0)
    {
        return false;
    }

    size_t start = 0;

    // compare each comma-delimited name in turn
    while (start <= _length)
    {
        size_t end = start;

        while ( (end < _length) &&
                (_list[end] != ',') )
        {
            end++;
        }

        if ( ((end - start) == name_length) &&
             (memcmp(_list + start, _name, name_length) == 0) )
        {
            return true;
        }

        start = end + 1;
    }

    return false;
}
