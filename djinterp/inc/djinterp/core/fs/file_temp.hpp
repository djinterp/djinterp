/******************************************************************************
* djinterp [fs]                                                     file_temp.hpp
*
*   Temporary-file support (roadmap Phase 8). The atomic, safe part of temp
* handling -- creating an anonymous scratch file -- is a METHOD on file
* (file::open_temp), because a temp file IS a file and wants file's RAII. What
* remains here is the one query that returns a path rather than a handle: where
* the system keeps its temporary files.
*   WHY NOT A NAME GENERATOR. file_temp.h offers d_tmpnam_s, and deliberately
* labels it racy: a name handed back is not a file, and between generating it
* and opening it another process can win the name. This header does not surface
* that -- the safe pattern is file::open_temp (choose the name and open it as
* one atomic step, with no name ever exposed), and temp_directory_path only
* tells you the DIRECTORY, for when you must place a named artifact there
* yourself and accept the responsibility that comes with a name.
*
* 
* path:      /inc/djinterp/core/fs/file_temp.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TEMP_
#define DJINTERP_FS_FILE_TEMP_ 1

#include "file_path.hpp"
#include "file_common.hpp"

#include "../../c/fs/file_temp.h"    // d_tempdir


NS_DJINTERP


// temp_directory_path
//   function: the directory the system uses for temporary files (honouring
// TMPDIR and the platform's rules, all decided in c/fs). Returns an invalid
// path with _ec set if the location cannot be determined -- rare, but a caller
// that must write there should check rather than assume "/tmp".
inline path
temp_directory_path(error& _ec)
{
    char buf[D_FILE_PATH_MAX + 1];

    if (!d_tempdir(buf, sizeof(buf)))
    {
        _ec = error::from_errno();
        return path();
    }

    _ec.clear();
    return path(buf);
}

NS_END  // djinterp


#endif // DJINTERP_FS_FILE_TEMP_