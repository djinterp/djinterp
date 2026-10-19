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
* path:      /inc/djinterp/core/filesys/file_temp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TEMPORARY FILES
    ---------------
    1.  Location
*/

#ifndef DJINTERP_FILESYS_FILE_TEMP_HPP
#define DJINTERP_FILESYS_FILE_TEMP_HPP 1

// djinterp
#include "file_path.hpp"                // path
#include "file_common.hpp"              // error, the D_* kit
#include "../../c/filesys/file_temp.h"  // d_dir_temp


NS_DJINTERP


//==============================================================================
// 1.  TEMPORARY FILES
//==============================================================================


// 1.1    Location
//------------------------------------------------------------------------------
/**
 * @brief Reports the directory the system uses for temporary files, honouring
 *        TMPDIR and the platform's rules as c/filesys decides them.
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

    // c/filesys decides where the directory is, and reports through errno;
    // the answer then is an invalid path, not an empty one, which would name
    // the working directory to anything joined onto it
    if (!d_dir_temp(buf,
                    sizeof(buf)))
    {
        _ec = error::from_errno();

        return path(static_cast<const char*>(0));
    }

    _ec.clear();

    return path(buf);
}


NS_END  // djinterp

#endif  // DJINTERP_FILESYS_FILE_TEMP_HPP
