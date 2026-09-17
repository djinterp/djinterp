/******************************************************************************
* djinterp [jit]                                                         jit.c
*
* djinterp portable JIT core -- implementation (jit.h).
*   Executable-memory lifecycle, the emit primitives, and diagnostics. The
* memory backend is chosen at compile time from env/jit/env_jit.h: this file
* carries a POSIX (mmap/mprotect) path, a Windows (VirtualAlloc/VirtualProtect)
* path, and an Apple MAP_JIT path, selected by D_ENV_JIT_BACKEND.
*
* path:      /inc/djinterp/jit/jit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit.h"

// std / platform -- the backend headers for the selected memory API
#include <stdio.h>
#if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC)
    // windows
    #include <windows.h>
#elif ( (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_POSIX_MMAP) ||               \
        (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT) )
    // posix
    #include <sys/mman.h>
    #include <unistd.h>
    #if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT)
        // apple -- per-thread W^X toggle for MAP_JIT regions
        #include <pthread.h>
        #include <libkern/OSCacheControl.h>
    #endif
#endif


// ===========================================================================
// I.   INTERNAL HELPERS
// ===========================================================================

// d_jit_internal_page_size
//   function (internal): the OS page/allocation granularity, or 4096 as a
// safe fallback when no backend is compiled in.
static size_t d_jit_internal_page_size(void)
{
#if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (size_t)si.dwPageSize;
#elif ( (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_POSIX_MMAP) ||               \
        (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT) )
    long v = sysconf(_SC_PAGESIZE);
    return (v > 0) ? (size_t)v : (size_t)4096;
#else
    return (size_t)4096;
#endif
}

// d_jit_internal_round_up
//   function (internal): round _n up to a multiple of _page (a power of two).
static size_t d_jit_internal_round_up(size_t _n, size_t _page)
{
    if (_n == 0) { _n = 1; }
    return (_n + _page - 1) & ~(_page - 1);
}


// ===========================================================================
// II.  CODE-BUFFER LIFECYCLE
// ===========================================================================

int d_jit_buffer_init(d_jit_buffer* _buf, size_t _capacity)
{
    size_t page;
    if (_buf == NULL) { return -1; }
    _buf->code = NULL; _buf->size = 0; _buf->capacity = 0; _buf->finalized = 0;

#if !D_ENV_JIT_CAN_ALLOCATE_EXEC
    /* platform cannot hand out executable memory (e.g. JIT prohibited) */
    (void)_capacity;
    return -1;
#else
    page = d_jit_internal_page_size();
    _capacity = d_jit_internal_round_up(_capacity, page);

#if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC)
    _buf->code = (unsigned char*)VirtualAlloc(
        NULL, _capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (_buf->code == NULL) { return -1; }

#elif (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT)
    _buf->code = (unsigned char*)mmap(
        NULL, _capacity, PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
    if (_buf->code == MAP_FAILED) { _buf->code = NULL; return -1; }
    /* MAP_JIT regions start execute-protected; open the write window */
    pthread_jit_write_protect_np(0);

#else  /* D_ENV_JIT_BACKEND_POSIX_MMAP */
    _buf->code = (unsigned char*)mmap(
        NULL, _capacity, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (_buf->code == MAP_FAILED) { _buf->code = NULL; return -1; }
#endif

    _buf->capacity = _capacity;
    return 0;
#endif  /* D_ENV_JIT_CAN_ALLOCATE_EXEC */
}

int d_jit_emit(d_jit_buffer* _buf, const void* _bytes, size_t _n)
{
    if (_buf == NULL || _buf->code == NULL || _buf->finalized) { return -1; }
    if (_n > _buf->capacity - _buf->size)               { return -1; }
    if (_n != 0) {
        const unsigned char* src = (const unsigned char*)_bytes;
        size_t i;
        for (i = 0; i < _n; ++i) { _buf->code[_buf->size + i] = src[i]; }
        _buf->size += _n;
    }
    return 0;
}

int d_jit_emit_u8(d_jit_buffer* _buf, uint8_t _v)
{
    return d_jit_emit(_buf, &_v, 1);
}

int d_jit_emit_u16(d_jit_buffer* _buf, uint16_t _v)
{
    unsigned char b[2];
    int i;
    for (i = 0; i < 2; ++i) { b[i] = (unsigned char)((_v >> (8 * i)) & 0xFF); }
    return d_jit_emit(_buf, b, 2);
}

int d_jit_emit_u32(d_jit_buffer* _buf, uint32_t _v)
{
    unsigned char b[4];
    int i;
    for (i = 0; i < 4; ++i) { b[i] = (unsigned char)((_v >> (8 * i)) & 0xFF); }
    return d_jit_emit(_buf, b, 4);
}

int d_jit_emit_u64(d_jit_buffer* _buf, uint64_t _v)
{
    unsigned char b[8];
    int i;
    for (i = 0; i < 8; ++i) { b[i] = (unsigned char)((_v >> (8 * i)) & 0xFF); }
    return d_jit_emit(_buf, b, 8);
}

void* d_jit_buffer_finalize(d_jit_buffer* _buf)
{
    if (_buf == NULL || _buf->code == NULL) { return NULL; }
    if (_buf->finalized) { return _buf->code; }

#if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC)
    {
        DWORD old;
        if (!VirtualProtect(_buf->code, _buf->capacity,
                            PAGE_EXECUTE_READ, &old)) {
            return NULL;
        }
    }
#elif (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT)
    /* close the write window, then make the new code visible to I-fetch */
    pthread_jit_write_protect_np(1);
    sys_icache_invalidate(_buf->code, _buf->size);
#elif (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_POSIX_MMAP)
    if (mprotect(_buf->code, _buf->capacity, PROT_READ | PROT_EXEC) != 0) {
        return NULL;
    }
#else
    return NULL;  /* no backend compiled in */
#endif

    D_JIT_CLEAR_CACHE(_buf->code, _buf->code + _buf->size);
    _buf->finalized = 1;
    return _buf->code;
}

void d_jit_buffer_release(d_jit_buffer* _buf)
{
    if (_buf == NULL || _buf->code == NULL) { return; }
#if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC)
    VirtualFree(_buf->code, 0, MEM_RELEASE);
#elif ( (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_POSIX_MMAP) ||               \
        (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT) )
    munmap(_buf->code, _buf->capacity);
#endif
    _buf->code = NULL; _buf->size = 0; _buf->capacity = 0; _buf->finalized = 0;
}


// ===========================================================================
// III. DIAGNOSTICS
// ===========================================================================

const char* d_jit_backend_name(void)
{
    return D_ENV_JIT_BACKEND_NAME;
}

void d_jit_print_info(void)
{
    printf("djinterp JIT environment\n");
    printf("  backend            : %s\n", D_ENV_JIT_BACKEND_NAME);
    printf("  available          : %d\n", (int)D_ENV_JIT_AVAILABLE);
    printf("  can allocate exec  : %d\n", (int)D_ENV_JIT_CAN_ALLOCATE_EXEC);
    printf("  jit prohibited     : %d\n", (int)D_ENV_JIT_PROHIBITED);
    printf("  requires W^X       : %d\n", (int)D_ENV_JIT_REQUIRES_WX);
    printf("  needs icache flush : %d\n", (int)D_ENV_JIT_NEEDS_ICACHE_FLUSH);
    printf("  has encoder        : %d\n", (int)D_ENV_JIT_HAS_ENCODER);
}
