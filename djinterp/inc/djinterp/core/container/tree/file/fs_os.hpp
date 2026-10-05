/*******************************************************************************
* djinterp [core]                                                      fs_os.hpp
*
* Filesystem OS selector:
*   Defines the operating_system enum - the public selector shared by the
* filesystem layer (file_tree<>, file_attributes<>) to choose a backend.
* Its values mirror the env.h D_ENV_OS_FLAG_* families so a detected
* D_ENV_OS_ID can be folded onto one of them, and so cfg_filesys.h's
* D_CFG_FILESYS_ALLOW_* gating lines up one-to-one with the members here.
*
*   This header is deliberately tiny and dependency-free (beyond the
* framework prelude): it is the single definition point for the selector,
* so the tree and attribute layers cannot drift out of sync.
*
*
* path:      /inc/djinterp/core/container/tree/file/fs_os.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FS_OS_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FS_OS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "fs_os.hpp can only be used in C++ compilation mode"
#endif

// djinterp
#include "../../../../djinterp.hpp"
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t


NS_DJINTERP


// ================================================================
//  operating_system
// ================================================================

// operating_system
//   enum: selects which backend a filesystem facility uses (file_tree<>,
// file_attributes<>). Values intentionally mirror the env.h D_ENV_OS_FLAG_*
// families so a detected D_ENV_OS_ID can be folded onto one of these.
//
//   The enum is the *public selector*; each facility maps a value onto a
// concrete backend policy. Members whose backend is not enabled for the
// current build (see cfg_filesys.h) are a compile error to select - you cannot
// name a backend that could never run unless you opt in.
enum class operating_system : re_std::uint8_t
{
    // generic families
    automatic       = 0,    // detected from D_ENV_OS_ID at compile time
    posix           = 1,    // portable baseline

    // unix-like specializations
    linux_generic   = 2,    // Linux fast paths (statx / getdents64)
    bsd             = 3,    // BSD family (dirent::d_type)
    apple           = 4,    // macOS (getattrlistbulk / birthtime)
    ios             = 5,    // iOS (sandbox-aware Darwin)

    // windows
    windows         = 6,    // generic Win32
    windows10       = 7,    // Win32 with modern hints
    windows11       = 8,    // alias of windows10 backend

    // explicit no-op
    none            = 255   // null backend; never touches the OS
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FS_OS_HPP
