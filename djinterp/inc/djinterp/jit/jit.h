/******************************************************************************
* djinterp [jit]                                                         jit.h
*
* djinterp portable JIT core (executable-memory + code buffer):
*   The architecture-independent half of the djinterp JIT. It turns the
* compile-time answers from env/jit/env_jit.h into runtime machinery: a code
* buffer backed by W^X-toggleable executable memory, primitive byte/word emit
* operations, a finalize step that flips the pages to read-execute (and flushes
* the instruction cache where the target needs it), plus a few diagnostics.
*
*   The per-architecture instruction encoders (jit_x64.h / jit_x86.h) build on
* this: they emit their opcode bytes into a d_jit_buffer through the primitives
* declared here. This header holds nothing architecture-specific.
*
*   Backend: d_jit_buffer_init obtains memory via the backend env_jit selected
* (POSIX mmap+mprotect, Windows VirtualAlloc+VirtualProtect, or Apple MAP_JIT);
* callers never name a backend. If D_ENV_JIT_CAN_ALLOCATE_EXEC is 0, init fails
* cleanly.
*
*   Requires:  djinterp.h (qualifier kit + scalar types) and env/jit/env_jit.h
*              (backend, policy, and cache-coherency detection).
*
* path:      /inc/djinterp/jit/jit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_
#define DJINTERP_JIT_ 1

// std
#include <stddef.h>      // for size_t
#include <stdint.h>      // for uint8_t / uint32_t / uint64_t
// djinterp
#include "../c/djinterp.h"
#include "../env/jit/env_jit.h"


// ===========================================================================
// I.   INSTRUCTION-CACHE FLUSH
// ===========================================================================
//   Relocated here from env_jit.h: the cache-flush OPERATION belongs with the
// code that emits, while env_jit.h keeps only the DETECTION flags it derives
// from (D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE / D_ENV_JIT_NEEDS_ICACHE_FLUSH).

// D_JIT_CLEAR_CACHE
//   macro: portable instruction-cache flush over the byte range [begin, end).
// Expands to the compiler builtin when available and to a harmless no-op
// otherwise (including on MSVC, where the Windows API call belongs at the
// point of use). d_jit_buffer_finalize calls it for you; it is exposed for
// callers that manage their own executable memory.
#ifndef D_JIT_CLEAR_CACHE
    #if D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE
        #define D_JIT_CLEAR_CACHE(begin, end)                                \
            __builtin___clear_cache((char*)(begin), (char*)(end))
    #else
        #define D_JIT_CLEAR_CACHE(begin, end)                                \
            ( (void)(begin), (void)(end) )
    #endif
#endif


// ===========================================================================
// II.  FUNCTION-POINTER CAST
// ===========================================================================
// D_JIT_FUNC_PTR(fn_type, code_ptr)
//   macro: cast a finalized code pointer to a function-pointer type. ISO C
// only conditionally supports object/function pointer casts; POSIX requires
// them (dlsym depends on it), so the one necessary cast is isolated here
// rather than sprinkled through calling code.
#ifndef D_JIT_FUNC_PTR
    #define D_JIT_FUNC_PTR(fn_type, code_ptr)  ((fn_type)(code_ptr))
#endif


// ===========================================================================
// III. CODE BUFFER + RUNTIME API
// ===========================================================================

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// d_jit_buffer
//   type: a growable region of executable-capable memory plus a write cursor.
//   fields:
//     code      - base of the mapping; writable until finalize, then RX.
//     size      - bytes emitted so far (the write cursor).
//     capacity  - bytes reserved (rounded up to a page by init).
//     finalized - 0 while writable, 1 after d_jit_buffer_finalize.
typedef struct d_jit_buffer
{
    unsigned char* code;
    size_t         size;
    size_t         capacity;
    int            finalized;
} d_jit_buffer;

// d_jit_buffer_init
//   function: reserve at least _capacity bytes of writable, executable-capable
// memory (page-rounded) and initialise _buf to an empty, non-finalized buffer.
//   returns: 0 on success; -1 on failure (allocation refused, or the platform
// cannot allocate executable memory -- see D_ENV_JIT_CAN_ALLOCATE_EXEC).
D_NODISCARD int   d_jit_buffer_init(d_jit_buffer* _buf, size_t _capacity);

// d_jit_emit
//   function: append _n raw bytes to _buf, advancing the cursor. Checked
// against capacity; fails if the buffer is finalized.
//   returns: 0 on success; -1 on overflow or if already finalized.
D_NODISCARD int   d_jit_emit(d_jit_buffer* _buf,
                             const void* _bytes, size_t _n);

// d_jit_emit_u8
//   function: append a single byte. returns 0 / -1 as d_jit_emit.
D_NODISCARD int   d_jit_emit_u8(d_jit_buffer* _buf, uint8_t _b);

// d_jit_emit_u16
//   function: append a 16-bit value little-endian (x86 byte order).
D_NODISCARD int   d_jit_emit_u16(d_jit_buffer* _buf, uint16_t _v);

// d_jit_emit_u32
//   function: append a 32-bit value little-endian (disp32 / imm32).
D_NODISCARD int   d_jit_emit_u32(d_jit_buffer* _buf, uint32_t _v);

// d_jit_emit_u64
//   function: append a 64-bit value little-endian (imm64 for movabs).
D_NODISCARD int   d_jit_emit_u64(d_jit_buffer* _buf, uint64_t _v);

// d_jit_buffer_finalize
//   function: flip the buffer's pages from read-write to read-execute, flush
// the instruction cache over the emitted range, and mark it finalized. After
// this the code may be called (cast it with D_JIT_FUNC_PTR); no further
// emit is permitted.
//   returns: a pointer to the executable code (== _buf->code) on success, or
// NULL if protection failed.
D_NODISCARD void* d_jit_buffer_finalize(d_jit_buffer* _buf);

// d_jit_buffer_release
//   function: release the buffer's memory (munmap / VirtualFree) and zero its
// fields. Safe on a zeroed or already-released buffer.
void              d_jit_buffer_release(d_jit_buffer* _buf);

// d_jit_backend_name
//   function: the name of the executable-memory backend in use (relocated from
// env_jit.h). returns a static string such as "mmap" or "VirtualAlloc".
const char*       d_jit_backend_name(void);

// d_jit_print_info
//   function: print the detected JIT environment (backend, availability, W^X
// policy, cache behaviour, encoder) to stdout. Diagnostics / build check.
void              d_jit_print_info(void);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_
