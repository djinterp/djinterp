/******************************************************************************
* djinterp [utility]                                            sink_common.h
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
*   The C fork has no growable byte buffer -- no d_string, no d_vector -- and
* putting an allocator in tier 0 would make every module inherit one.  A sink
* inverts the problem: the producer streams, and the CONSUMER decides where the
* bytes live.  A counting sink measures, a bounded sink fills a caller array, a
* C++ adapter appends to a std::string, and a future d_string is one more sink.
* Nothing in a producer changes when a new destination appears.
*
*   WHY NOT fn_write:
*   djinterp.h's fn_write is `size_t (*)(char* const, size_t)` -- no context
* pointer, so it cannot drive a growable buffer or a file handle without global
* mutable state.  struct d_pack_sink carries a context and is otherwise the
* same idea.
*
*
* path:      /inc/djinterp/core/util/sink_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

#ifndef DJINTERP_UTIL_SINK_COMMON_
#define DJINTERP_UTIL_SINK_COMMON_ 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../../c/djinterp.h"       // D_STATIC_ASSERT, D_EXTERN_C_*

#if !defined(D_EXTERN_C_BEGIN)
    #error "sink_common.h needs D_EXTERN_C_BEGIN; \
define it or set D_CFG_DEFINE_EXTERN_C to 1"
#endif

D_EXTERN_C_BEGIN


// =============================================================================
// I.   BORROWED SPANS
// =============================================================================
//   Neither owns anything, both carry a length, and both are trivially
// copyable and identical under C and C++ -- which is what lets a C++
// std::string reach a shared core without that core knowing std::string exists.
//
//   The length is authoritative, not the NUL.  A d_pack_text may point at a
// buffer that is not NUL-terminated, and may contain an interior NUL.

// d_pack_text
//   struct: a borrowed character span -- pointer plus length in bytes.
// Encoding is not implied; a consumer that cares records the intent separately.
struct d_pack_text
{
    const char* data;
    size_t      length;
};

// d_pack_bytes
//   struct: a borrowed byte span -- pointer plus size.
struct d_pack_bytes
{
    const void* data;
    size_t      size;
};

// D_PACK_TEXT_NONE / D_PACK_BYTES_NONE
//   macro: the empty spans, as brace initialisers.
#define D_PACK_TEXT_NONE    { (const char*)0, (size_t)0 }
#define D_PACK_BYTES_NONE   { (const void*)0, (size_t)0 }

D_STATIC_ASSERT(sizeof(struct d_pack_text) == sizeof(struct d_pack_bytes),
                "d_pack_text / d_pack_bytes: spans must share a shape");
D_STATIC_ASSERT(offsetof(struct d_pack_text, data) == 0,
                "d_pack_text: field drift at data");
D_STATIC_ASSERT(offsetof(struct d_pack_bytes, data) == 0,
                "d_pack_bytes: field drift at data");

struct d_pack_text  d_pack_text_from_cstr(const char* _cstr);
struct d_pack_text  d_pack_text_from_span(const char* _data,
                                          size_t      _length);
struct d_pack_bytes d_pack_bytes_from_span(const void* _data,
                                           size_t      _size);
int                 d_pack_text_equal(struct d_pack_text _a,
                                      struct d_pack_text _b);
int                 d_pack_text_is_empty(struct d_pack_text _text);


// =============================================================================
// II.  THE SINK
// =============================================================================
//   A context-carrying byte consumer.  It is what lets a growable consumer
// avoid the measure pass, and it is the single point at which d_string will
// attach when it exists.
//
//   `write` returns the number of bytes accepted.  A short write is a sink
// failure and aborts the transform with D_PACK_STATUS_SINK_ERROR: partial
// acceptance has no meaning for a codec that cannot rewind.

// d_pack_sink_write_fn
//   function pointer: accepts _size bytes from _data on behalf of _context,
// returning the number of bytes taken.  Any value other than _size ends the
// transform.
typedef size_t (*d_pack_sink_write_fn)(void*       _context,
                                       const void* _data,
                                       size_t      _size);

// d_pack_sink
//   struct: a byte consumer plus its context.  Passed by value; holds no
// ownership of _context.
struct d_pack_sink
{
    d_pack_sink_write_fn    write;
    void*                   context;
};

// d_pack_counting_sink
//   struct: the context of a sink that accepts everything and records only
// the total.  This is how the measure pass is implemented, so measuring and
// producing run the same code path and cannot disagree about the size.
struct d_pack_counting_sink
{
    size_t  total;
};

// d_pack_buffer_sink
//   struct: the context of a sink that writes into a fixed caller-owned
// buffer.  `overflow` is raised as soon as a write would exceed `capacity`,
// and `needed` continues to accumulate the full requirement, which is what
// lets the buffer form report the size to grow to on failure.
struct d_pack_buffer_sink
{
    unsigned char*  buffer;
    size_t          capacity;
    size_t          written;
    size_t          needed;
    int             overflow;
};

struct d_pack_sink d_pack_sink_from_counter(
                    struct d_pack_counting_sink* _counter);
struct d_pack_sink d_pack_sink_from_buffer(struct d_pack_buffer_sink* _buffer);
void               d_pack_counting_sink_init(
                    struct d_pack_counting_sink* _counter);
void               d_pack_buffer_sink_init(
                    struct d_pack_buffer_sink* _sink,
                    void*                      _buffer,
                    size_t                     _capacity);
int                d_sink_emit(struct d_pack_sink _sink,
                               const void*        _data,
                               size_t             _size);



D_EXTERN_C_END


#endif  // DJINTERP_UTIL_SINK_COMMON_
