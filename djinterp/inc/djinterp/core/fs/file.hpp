/*******************************************************************************
* djinterp [core]                                                       file.hpp
*
* Umbrella for the C++ filesystem layer.
*   Including this one header pulls in the whole layer -- the common base,
* every module, and the C++23 return surface -- so a caller who wants "all of
* djinterp's file support" writes a single include rather than tracking the
* dependency order by hand.
*   The layer is a thin RAII / value-type skin over the C c/fs modules.
* Nothing here makes a platform decision; every OS branch lives in c/fs, and
* this side uses only the portable macros that layer guarantees. Everything
* public is flat in djinterp:: (never djinterp::fs::), the only nesting being
* djinterp::internal for implementation helpers.
*   Layering -- each file_*.hpp derives from file_common.hpp, and from the
* modules it builds on:
*     file_common.hpp    common to all (prelude + C common + error)
*     file_path.hpp      path            -- lexical path value
*     file_stat.hpp      file_status     -- metadata + status queries
*     file_stream.hpp    file            -- RAII FILE* handle (open/io/seek/...)
*     file_dir.hpp       directory       -- directory handle + range-for
*     file_ops.hpp                       -- remove / rename / copy_file
*     file_link.hpp                      -- symlinks
*     file_temp.hpp                      -- temp_directory_path
*     file_space.hpp     space_info      -- filesystem capacity
*     file_recursive.hpp                 -- walk / remove_all (on dir+stat+ops)
*     file_pipe.hpp      process         -- popen-model command pipe
*     file_expected.hpp                  -- C++23 std::expected return surface
*
*
* path:      /inc/djinterp/core/fs/file.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_FS_FILE_HPP
#define DJINTERP_FS_FILE_HPP 1

// djinterp
#include "file_common.hpp"     // common base: prelude, C common, error
#include "file_path.hpp"       // path
#include "file_stat.hpp"       // file_status, status, symlink_status,
                               // file_size, exists, is_*
#include "file_stream.hpp"     // file, the RAII FILE* handle
#include "file_dir.hpp"        // directory and its iterator,
                               // create_directory, remove_directory
#include "file_ops.hpp"        // remove, remove_file, rename, copy_file
#include "file_link.hpp"       // create_symlink, read_symlink, is_symlink
#include "file_temp.hpp"       // temp_directory_path
#include "file_space.hpp"      // space_info, space
#include "file_recursive.hpp"  // walk, remove_all
#include "file_pipe.hpp"       // process
#include "file_expected.hpp"   // C++23 std::expected overloads; inert below
                               // C++23
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_HPP
