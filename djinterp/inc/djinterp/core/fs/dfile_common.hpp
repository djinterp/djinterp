/******************************************************************************
* djinterp [core]                                             dfile_common.hpp
*
* Shared foundation for the djinterp C++ file I/O modules.
*   This header is the C++ counterpart to dfile_common.h. It pulls in the C
* foundation (types, constants, feature and platform macros) and layers the
* pieces every C++ file module shares on top: the djinterp::file namespace,
* strongly-typed enums for the flag sets the C API takes as plain ints, and a
* handful of readability aliases. Nothing here allocates or wraps a resource;
* the RAII handle classes and free functions live in the per-use-case modules.
*   The C++ layer is a thin, zero-overhead veneer: every wrapper forwards to
* the extern "C" implementation declared by the C headers and is marked
* D_INLINE, so the generated code is identical to calling the C function
* directly while gaining namespaces, RAII, and type-safe flags.
*
* 
* path:      /inc/djinterp/cpp/io/file/dfile_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ALIASES
      -------
      1.  offset        (alias for the C d_off_t file-offset type)

II.   STRONGLY-TYPED FLAGS
      --------------------
      1.  seek_origin   (SEEK_* wrapper)
      2.  lock_flag     (D_LOCK_* wrapper, combinable)
      3.  access_flag   (F_OK/R_OK/W_OK/X_OK wrapper, combinable)
      4.  entry_type    (DT_* wrapper for directory entries)

III.  FLAG OPERATORS
      --------------
      1.  operator| / operator& / to_int helpers for the combinable flags
*/

#ifndef DJINTERP_FILE_COMMON_
#define DJINTERP_FILE_COMMON_ 1

#include "../../../c/io/file/dfile_common.h"


NS_DJINTERP
D_NAMESPACE(file)

// I. aliases

// offset
//   type: signed 64-bit file offset; the C++ spelling of d_off_t.
using offset = d_off_t;


// II. strongly-typed flags

// seek_origin
//   type: reference point for a seek, wrapping the SEEK_* constants.
enum class seek_origin : int
{
    set     = SEEK_SET,     // from the beginning of the file
    current = SEEK_CUR,     // from the current position
    end     = SEEK_END      // from the end of the file
};

// lock_flag
//   type: advisory-lock operation, wrapping the D_LOCK_* constants. The
// shared/exclusive/unlock modes may be combined with nonblock via operator|.
enum class lock_flag : int
{
    shared    = D_LOCK_SH,  // shared (read) lock
    exclusive = D_LOCK_EX,  // exclusive (write) lock
    nonblock  = D_LOCK_NB,  // fail instead of blocking
    unlock    = D_LOCK_UN   // release the lock
};

// access_flag
//   type: accessibility check, wrapping the F_OK/R_OK/W_OK/X_OK constants.
// The permission bits may be combined with operator|.
enum class access_flag : int
{
    exists  = F_OK,         // path exists
    read    = R_OK,         // readable
    write   = W_OK,         // writable
    execute = X_OK          // executable
};

// entry_type
//   type: directory-entry file type, wrapping the DT_* constants as reported
// by d_dirent_t.d_type.
enum class entry_type : uint8_t
{
    unknown      = DT_UNKNOWN,
    regular      = DT_REG,
    directory    = DT_DIR,
    symlink      = DT_LNK,
    char_device  = DT_CHR,
    block_device = DT_BLK,
    fifo         = DT_FIFO,
    socket       = DT_SOCK
};


// III. flag operators

// to_int
//   converts a strongly-typed flag back to the int the C API expects.
D_CONSTEXPR_INLINE
int
to_int(lock_flag _flag)
{
    return static_cast<int>(_flag);
}

D_CONSTEXPR_INLINE
int
to_int(access_flag _flag)
{
    return static_cast<int>(_flag);
}

D_CONSTEXPR_INLINE
int
to_int(seek_origin _origin)
{
    return static_cast<int>(_origin);
}

// operator| / operator&
//   combine and test the bitmask flag sets (lock_flag, access_flag) while
// keeping the strong type.
D_CONSTEXPR_INLINE
lock_flag
operator|(lock_flag _lhs, lock_flag _rhs)
{
    return static_cast<lock_flag>(static_cast<int>(_lhs) |
                                  static_cast<int>(_rhs));
}

D_CONSTEXPR_INLINE
lock_flag
operator&(lock_flag _lhs, lock_flag _rhs)
{
    return static_cast<lock_flag>(static_cast<int>(_lhs) &
                                  static_cast<int>(_rhs));
}

D_CONSTEXPR_INLINE
access_flag
operator|(access_flag _lhs, access_flag _rhs)
{
    return static_cast<access_flag>(static_cast<int>(_lhs) |
                                    static_cast<int>(_rhs));
}

D_CONSTEXPR_INLINE
access_flag
operator&(access_flag _lhs, access_flag _rhs)
{
    return static_cast<access_flag>(static_cast<int>(_lhs) &
                                    static_cast<int>(_rhs));
}

NS_END  // file
NS_END  // djinterp


#endif  // DJINTERP_FILE_COMMON_
