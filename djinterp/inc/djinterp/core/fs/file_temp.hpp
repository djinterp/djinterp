/*******************************************************************************
* djinterp [core]                                                  file_temp.hpp
*
* Temporary-file support (roadmap Phase 8).
*   The atomic, safe part of temp handling -- creating an anonymous scratch
* file -- is a METHOD on file (file::open_temp), because a temp file IS a file
* and wants file's RAII. What remains here is the one query that returns a path
* rather than a handle: where the system keeps its temporary files.
*   WHY NOT A NAME GENERATOR. file_temp.h offers d_file_temp_name, and
* deliberately labels it racy: a name handed back is not a file, and between
* generating it and opening it another process can win the name. This header
* does not surface that -- the safe pattern is file::open_temp (choose the name
* and open it as one atomic step, with no name ever exposed), and
* temp_directory_path only tells you the DIRECTORY, for when you must place a
* named artifact there yourself and accept the responsibility that comes with
* a name.
*
*
* path:      /inc/djinterp/core/fs/file_temp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TEMPORARY FILES
    ---------------
    1.  Location
*/

#ifndef DJINTERP_FS_FILE_TEMP_HPP
#define DJINTERP_FS_FILE_TEMP_HPP 1

// djinterp
#include "file_path.hpp"           // path
#include "file_common.hpp"         // error, the D_* kit
#include "../../c/fs/file_temp.h"  // d_dir_temp
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP


//==============================================================================
// 1.  TEMPORARY FILES
//==============================================================================


// 1.1    Location
//------------------------------------------------------------------------------
/**
 * @brief Reports the directory the system uses for temporary files, honouring
 *        TMPDIR and the platform's rules as c/fs decides them.
 *
 * @note Rare to fail, but a caller that must write there should check rather
 *       than assume "/tmp".
 *
 * @param[out] _ec  cleared on success; set when the location cannot be
 *                  determined.
 * @return the directory, or an invalid path on failure.
 */
inline path
temp_directory_path(
    error& _ec
)
{
    char buf[D_FILE_PATH_MAX + 1];

    // c/fs decides where the directory is, and reports through errno
    if (!d_dir_temp(buf,
                    sizeof(buf)))
    {
        _ec = error::from_errno();

        return path();
    }

    _ec.clear();

    return path(buf);
}


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_TEMP_HPP
