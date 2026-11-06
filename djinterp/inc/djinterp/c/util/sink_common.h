/*******************************************************************************
* djinterp [c]                                                     sink_common.h
*
*   A context-carrying byte consumer, and the two spans that go with it.
* Compiled by BOTH C and C++.
*
*   WHY THIS IS ITS OWN HEADER:
*   These types were born inside compress_common.h, because compression is
* where the need first appeared.  They are not compression concepts.  A sink is
* "somewhere bytes go"; a span is "borrowed bytes"; both are wanted by anything
* that PRODUCES output without owning a buffer -- the archive writers, the test
* framework's parity emitter, and eventually the document renderers.
*
*   Leaving them in compress_common.h would have forced the test framework to
* include the compression facade to emit a line of text, which is the kind of
* dependency that reads as an accident and then calcifies.  So they moved here,
* and compress_common.h includes this.
*
*   WHAT A SINK IS FOR:
*   Tier 0 has no growable byte buffer -- dj_string lives above it -- and
* putting an allocator in tier 0 would make every module inherit one.  A sink
* inverts the problem: the producer streams, and the CONSUMER decides where the
* bytes live.  A counting sink measures, a bounded sink fills a caller array,
* and a C++ adapter appends to a std::string.  Nothing in a producer changes
* when a new destination appears.
*   ostream.h builds on it: every sink is a stream's transport, and every
* stream lowers back to a sink, so a producer that takes a sink reaches the
* console, a file or a dj_string -- the last written in place, through the
* string's own capacity.
*
*   WHY NOT fn_write:
*   djinterp.h's fn_write is `size_t (*)(char* const, size_t)` -- no context
* pointer, so it cannot drive a growable buffer or a file handle without global
* mutable state.  struct dj_pack_sink carries a context and is otherwise the
* same idea.
*
*
* path:      /inc/djinterp/c/util/sink_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BORROWED SPANS
    --------------
    1.  The spans
         1.  dj_pack_text
         2.  dj_pack_bytes
         3.  D_PACK_TEXT_NONE, D_PACK_BYTES_NONE
    2.  Making and comparing spans
2.  THE SINK
    --------
    1.  The sink
         1.  dj_pack_sink_write_fn
         2.  dj_pack_sink
    2.  Measuring and filling
         1.  dj_pack_counting_sink
         2.  dj_pack_buffer_sink
    3.  Making sinks, and writing to one
*/

#ifndef DJINTERP_C_UTIL_SINK_COMMON_H
#define DJINTERP_C_UTIL_SINK_COMMON_H 1

// std
#include <stddef.h>  // size_t, offsetof
// djinterp
#include "../djinterp.h"  // framework root
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // fixed-width types, for users

#if !defined(D_EXTERN_C_BEGIN)
    #error "sink_common.h needs D_EXTERN_C_BEGIN; \
define it or set D_CFG_DEFINE_EXTERN_C to 1"
#endif

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  BORROWED SPANS
//==============================================================================
//   Neither owns anything, both carry a length, and both are trivially
// copyable and identical under C and C++ -- which is what lets a C++
// std::string reach a shared core without that core knowing std::string exists.
//
//   The length is authoritative, not the NUL.  A dj_pack_text may point at a
// buffer that is not NUL-terminated, and may contain an interior NUL.


// 1.1    The spans
//------------------------------------------------------------------------------
// 1.1.1
// dj_pack_text
//   struct: a borrowed character span -- pointer plus length in bytes.
// Encoding is not implied; a consumer that cares records the intent separately.
struct dj_pack_text
{
    const char* data;
    size_t      length;
};

// 1.1.2
// dj_pack_bytes
//   struct: a borrowed byte span -- pointer plus size.
struct dj_pack_bytes
{
    const void* data;
    size_t      size;
};

// 1.1.3
// D_PACK_TEXT_NONE, D_PACK_BYTES_NONE
//   macro: the empty spans, as brace initialisers.
#define D_PACK_TEXT_NONE    { (const char*)0, (size_t)0 }
#define D_PACK_BYTES_NONE   { (const void*)0, (size_t)0 }

D_STATIC_ASSERT(sizeof(struct dj_pack_text) == sizeof(struct dj_pack_bytes),
                "dj_pack_text / dj_pack_bytes: spans must share a shape");
D_STATIC_ASSERT(offsetof(struct dj_pack_text, data) == 0,
                "dj_pack_text: field drift at data");
D_STATIC_ASSERT(offsetof(struct dj_pack_bytes, data) == 0,
                "dj_pack_bytes: field drift at data");

// 1.2    Making and comparing spans
//------------------------------------------------------------------------------
// none allocates or fails: a null pointer is the empty span, and two empty
// spans are equal whatever their pointers
struct dj_pack_text  d_pack_text_from_cstr(const char* _cstr);
struct dj_pack_text  d_pack_text_from_span(const char* _data,
                                           size_t      _length);
struct dj_pack_bytes d_pack_bytes_from_span(const void* _data,
                                            size_t      _size);
int                  d_pack_text_equal(struct dj_pack_text _a,
                                       struct dj_pack_text _b);
int                  d_pack_text_is_empty(struct dj_pack_text _text);


//==============================================================================
// 2.  THE SINK
//==============================================================================
//   A context-carrying byte consumer.  It is what lets a growable consumer
// avoid the measure pass; a dj_string is reached through a stream over it
// (c/util/ostream/ostream.h), lowered to a sink with d_ostream_as_sink.
//
//   `write` returns the number of bytes accepted.  A short write is a sink
// failure and aborts the transform with D_PACK_STATUS_SINK_ERROR: partial
// acceptance has no meaning for a codec that cannot rewind.


// 2.1    The sink
//------------------------------------------------------------------------------
// 2.1.1
// dj_pack_sink_write_fn
//   function pointer: accepts _size bytes from _data on behalf of _context,
// returning the number of bytes taken.  Any value other than _size ends the
// transform.
typedef size_t (*dj_pack_sink_write_fn)(void*       _context,
                                        const void* _data,
                                        size_t      _size);

// 2.1.2
// dj_pack_sink
//   struct: a byte consumer plus its context.  Passed by value; holds no
// ownership of _context.
struct dj_pack_sink
{
    dj_pack_sink_write_fn write;
    void*                 context;
};

// 2.2    Measuring and filling
//------------------------------------------------------------------------------
// 2.2.1
// dj_pack_counting_sink
//   struct: the context of a sink that accepts everything and records only
// the total.  This is how the measure pass is implemented, so measuring and
// producing run the same code path and cannot disagree about the size.
struct dj_pack_counting_sink
{
    size_t total;
};

// 2.2.2
// dj_pack_buffer_sink
//   struct: the context of a sink that writes into a fixed caller-owned
// buffer.  `overflow` is raised as soon as a write would exceed `capacity`,
// and `needed` continues to accumulate the full requirement, which is what
// lets the buffer form report the size to grow to on failure.
struct dj_pack_buffer_sink
{
    unsigned char* buffer;
    size_t         capacity;
    size_t         written;
    size_t         needed;
    int            overflow;
};

// 2.3    Making sinks, and writing to one
//------------------------------------------------------------------------------
// a sink over a counter or a buffer borrows it, which must outlive the sink;
// the initialisers zero the counts and set no other state
struct dj_pack_sink d_pack_sink_from_counter(
                        struct dj_pack_counting_sink* _counter);
struct dj_pack_sink d_pack_sink_from_buffer(
                        struct dj_pack_buffer_sink* _buffer);
void                d_pack_counting_sink_init(
                        struct dj_pack_counting_sink* _counter);
void                d_pack_buffer_sink_init(
                        struct dj_pack_buffer_sink* _sink,
                        void*                       _buffer,
                        size_t                      _capacity);
/**
 * @brief Writes every byte to a sink, or reports that it did not take them.
 *
 * @param[in] _sink  the destination.
 * @param[in] _data  the bytes; may be NULL when _size is 0.
 * @param[in] _size  how many.
 * @return 1 when the sink accepted every byte, 0 otherwise: a short write,
 *         or a sink with no write function, whatever the size. A zero-size
 *         write to a sink with one succeeds without calling it.
 */
int                 d_sink_emit(struct dj_pack_sink _sink,
                                const void*         _data,
                                size_t              _size);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_SINK_COMMON_H
