/******************************************************************************
* djinterp [c]                                                       file_io.h
*
* Moving bytes -- through a descriptor, at an offset, or a whole file at once.
*   Read and write live together because they are one mechanism seen from two
* directions: the same chunking rule, the same short-transfer retry, the same
* open/size/close scaffolding behind the whole-file helpers. Splitting them
* would duplicate all of it to buy a separation nothing needs.
*   Descriptor LIFECYCLE is file_desc.h; this module only transfers through a
* descriptor the caller already holds. The whole-file helpers open by path and
* so depend on file_open.h.
*   Whether d_fwrite_all replaces a file atomically and whether it waits for
* durable storage are build-time decisions -- see cfg_file_io.h, and
* D_FILE_WRITE_IS_ATOMIC / D_FILE_WRITE_IS_DURABLE to query them.
*
* path:      /inc/djinterp/c/fs/file_io.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DESCRIPTOR READS
      ----------------
      1.  d_read             (POSIX read equivalent; may read short)
      2.  d_read_full        (read exactly _count bytes, or fewer at EOF)
      3.  d_pread            (positional read; atomic where supported)

II.   DESCRIPTOR WRITES
      -----------------
      1.  d_write            (POSIX write equivalent; may write short)
      2.  d_write_full       (write exactly _count bytes, or fail)
      3.  d_pwrite           (positional write; atomic where supported)

III.  WHOLE-FILE READS
      ----------------
      1.  d_fread_all        (read a path into a fresh allocation)
      2.  d_fread_all_stream (read an open stream into a fresh allocation)
      3.  d_fread_all_into   (read a path into a caller's buffer; no alloc)

IV.   WHOLE-FILE WRITES
      -----------------
      1.  d_fwrite_all       (create or replace a file with a buffer)
      2.  d_fappend_all      (append a buffer to a file, creating it if new)
*/

#ifndef DJINTERP_FILE_IO_
#define DJINTERP_FILE_IO_ 1

// djinterp
#include "./file_common.h"
#include "./file_open.h"
#include "../../config/c/fs/cfg_file_io.h"


D_EXTERN_C_BEGIN


// I.    Descriptor reads
ssize_t d_read(int    _fd,
               void*  _buf,
               size_t _count);
ssize_t d_read_full(int    _fd,
                    void*  _buf,
                    size_t _count);
ssize_t d_pread(int     _fd,
                void*   _buf,
                size_t  _count,
                d_off_t _offset);

// II.   Descriptor writes
ssize_t d_write(int         _fd,
                const void* _buf,
                size_t      _count);
ssize_t d_write_full(int         _fd,
                     const void* _buf,
                     size_t      _count);
ssize_t d_pwrite(int         _fd,
                 const void* _buf,
                 size_t      _count,
                 d_off_t     _offset);

// III.  Whole-file reads
void*   d_fread_all(const char* _path,
                    size_t*     _size);
void*   d_fread_all_stream(FILE*   _stream,
                           size_t* _size);
int     d_fread_all_into(const char* _path,
                         void*       _buf,
                         size_t      _bufsize,
                         size_t*     _size);

// IV.   Whole-file writes
int     d_fwrite_all(const char* _path,
                     const void* _data,
                     size_t      _size);
int     d_fappend_all(const char* _path,
                      const void* _data,
                      size_t      _size);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_IO_
