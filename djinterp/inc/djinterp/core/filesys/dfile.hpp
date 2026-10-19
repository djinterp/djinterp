/*******************************************************************************
* djinterp [core]                                                      dfile.hpp
*
* Umbrella for the C++ filesystem layer.
*   Including this one header pulls in the whole layer -- the common base,
* every module, and the C++23 return surface -- so a caller who wants "all of
* djinterp's file support" writes a single include rather than tracking the
* dependency order by hand.
*   The layer is a thin RAII / value-type skin over the C c/filesys modules.
* Nothing here makes a platform decision; every OS branch lives in c/filesys,
* and this side uses only the portable macros that layer guarantees.
* Everything public is flat in djinterp:: (never djinterp::filesys::), the
* only nesting being djinterp::internal for implementation helpers.
*   WIDTH. The layer follows the C layer's D_FILE_WIDTH: file's offsets,
* file_status's size and time types, file_size and space_info are 64-bit where
* it is 64 and 32-bit where it is 32 (space_info then counting KiB), failing
* with EOVERFLOW where C does. What only asks a type -- file_kind, exists,
* is_directory, the directory walk, remove_all -- has no width and exists on
* every build. Unlike the C twins, the classes that carry a width have one
* name at both widths, so they never change between language levels: where a
* level has had to narrow the width the rest of the build keeps at 64
* (D_FILE_WIDTH_IS_NARROWED, below the 64-bit floor) they are absent, and
* every translation unit of a program must agree on the width it configures,
* as on _FILE_OFFSET_BITS for struct stat.
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
* path:      /inc/djinterp/core/filesys/dfile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_FILESYS_DFILE_HPP
#define DJINTERP_FILESYS_DFILE_HPP 1

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

#endif  // DJINTERP_FILESYS_DFILE_HPP
