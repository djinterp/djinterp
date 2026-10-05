/*******************************************************************************
* djinterp [core]                                              file_tree_bsd.hpp
*
* BSD-family file tree scanner:
*   Defines bsd_scanner, used by the BSD-derived systems (FreeBSD,
* OpenBSD, NetBSD, DragonFly) and any host whose dirent carries a
* populated d_type field.  It reads the entry type directly from the
* directory stream, skipping fstatat entirely for everything except
* regular files - where a stat is still issued to obtain the size that
* file_entry stores.
*
*   When d_type reports DT_UNKNOWN (some filesystems, network mounts),
* it falls back to an fstatat for that entry only, so the result is
* always correct.
*
*   Directories carry no size in file_entry, so a directory entry seen
* via d_type costs zero stat calls - the common case on a tree walk.
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_bsd.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_BSD_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_BSD_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "./file_tree_common.hpp"
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t

#if D_FILESYS_ENABLE_BSD

#include "./file_tree_posix.hpp"   // reuse classify / is_dot_entry / fallback

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <string>


NS_DJINTERP


// ================================================================
//  bsd_scanner
// ================================================================

// bsd_scanner
//   policy: BSD/Linux directory walk that consults dirent::d_type before
// paying for a stat.
struct bsd_scanner
{
    // type_from_dtype
    //   maps a dirent d_type onto a file_type, or file_type_unknown if the
    // type is not directly representable (caller stats).
    static file_type
    type_from_dtype(
        unsigned char _dtype
    )
    {
#ifdef DT_DIR
        switch (_dtype)
        {
            case DT_DIR: return file_type_directory;
            case DT_REG: return file_type_regular;
            case DT_LNK: return file_type_symlink;
            case DT_UNKNOWN: return file_type_unknown;
            default:     return file_type_other;
        }
#else
        (void)_dtype;
        return file_type_unknown;
#endif
    }

    // scan
    //   walks _dir_path using d_type, stat-ing only when a size is required
    // (regular files) or the type is unknown.
    template<typename Ctx>
    static void
    scan(
        Ctx&              _ctx,
        const std::string& _dir_path,
        file_node_id            _parent
    )
    {
        DIR* dir = ::opendir(_dir_path.c_str());

        if (dir == nullptr)
        {
            return;
        }

        int dfd = ::dirfd(dir);

        struct dirent* ent;

        while ((ent = ::readdir(dir)) != nullptr)
        {
            const char* child_name = ent->d_name;

            if (posix_scanner::is_dot_entry(child_name))
            {
                continue;
            }

            std::size_t child_len = std::strlen(child_name);

#ifdef DT_DIR
            file_type type = type_from_dtype(ent->d_type);
#else
            file_type type = file_type_unknown;
#endif
            re_std::uint64_t sz = 0;

            // size needed for regular files; type needed when the directory
            // stream couldn't tell us.
            if (type == file_type_unknown ||
                type == file_type_regular)
            {
                struct stat st;

                if (::fstatat(dfd, child_name, &st,
                              AT_SYMLINK_NOFOLLOW) == 0)
                {
                    if (type == file_type_unknown)
                    {
                        type = posix_scanner::classify(st.st_mode);
                    }

                    sz = static_cast<re_std::uint64_t>(st.st_size);
                }
                else if (type == file_type_unknown)
                {
                    // could not classify; skip rather than guess.
                    continue;
                }
            }

            file_node_id id = _ctx.intern_child(
                _parent, child_name, child_len, type, sz);

            if (type == file_type_directory)
            {
                _ctx.recurse(_dir_path + "/" + child_name, id);
            }
        }

        ::closedir(dir);

        return;
    }
};


NS_END  // djinterp

#else  // !D_FILESYS_ENABLE_BSD

// BSD backend not enabled for this build. Without it this header declares
// nothing, rather than stopping the build: a disabled backend is absent, and
// naming its scanner fails at the point of use (see os_scanner in
// file_tree.hpp). Set D_CFG_FILESYS_ALLOW_BSD (or
// D_CFG_FILESYS_ALLOW_POSIX_FAMILY, or D_CFG_FILESYS_ALLOW_FOREIGN) to 1 before
// including file_tree.hpp.

#endif  // D_FILESYS_ENABLE_BSD

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_BSD_HPP
