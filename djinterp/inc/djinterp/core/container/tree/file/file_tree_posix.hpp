/*******************************************************************************
* djinterp [core]                                            file_tree_posix.hpp
*
* POSIX file tree scanner (portable baseline):
*   Defines posix_scanner, the portable directory-walk backend used by
* every Unix-like target unless a more specialized header overrides it.
* It opens each directory once with opendir, then resolves child
* metadata relative to the directory fd with fstatat - avoiding the
* repeated full-path string construction and root-relative path walks
* that a plain lstat(full_path) incurs on deep trees.
*
*   Type and size come from a single fstatat per entry.  Where the
* platform populates dirent::d_type, bsd_scanner (file_tree_bsd.hpp)
* specializes this to skip the stat for non-size queries; this baseline
* stays correct everywhere by always stat-ing.
*
*   This header is self-contained: it pulls in <dirent.h>, <fcntl.h>,
* <sys/stat.h>, and <unistd.h> directly.  It is safe to include on any
* POSIX.1-2008 host (Linux, the BSDs, macOS, Solaris, illumos).
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_posix.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_POSIX_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_POSIX_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "./file_tree_common.hpp"
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t

#if D_FILESYS_ENABLE_POSIX

// POSIX headers
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <string>


NS_DJINTERP


// ================================================================
//  posix_scanner
// ================================================================

// posix_scanner
//   policy: portable POSIX directory walk. Uses a per-directory fd plus
// fstatat for child metadata. fstatat resolves names relative
// to the open directory, so neither a full path string nor a kernel path
// re-walk is needed per entry, and PATH_MAX is never a concern.
struct posix_scanner
{
    // classify
    //   maps a struct stat mode onto a file_type.
    static file_type
    classify(
        mode_t _mode
    )
    {
        if (S_ISLNK(_mode))  { return file_type_symlink;   }
        if (S_ISDIR(_mode))  { return file_type_directory; }
        if (S_ISREG(_mode))  { return file_type_regular;   }

        return file_type_other;
    }

    // is_dot_entry
    //   true for "." and "..".
    static bool
    is_dot_entry(
        const char* _name
    )
    {
        if (_name[0] != '.')
        {
            return false;
        }

        if (_name[1] == '\0')
        {
            return true;
        }

        return (_name[1] == '.' && _name[2] == '\0');
    }

    // scan
    //   recursively walks _dir_path, interning each child through _ctx and
    // recursing into subdirectories.
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

            if (is_dot_entry(child_name))
            {
                continue;
            }

            std::size_t child_len = std::strlen(child_name);

            struct stat st;

            // fstatat relative to the open directory fd; no full path build,
            // no root-relative re-walk.
            if (::fstatat(dfd, child_name, &st,
                          AT_SYMLINK_NOFOLLOW) != 0)
            {
                continue;
            }

            file_type        type = classify(st.st_mode);
            re_std::uint64_t sz    =
                static_cast<re_std::uint64_t>(st.st_size);

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

#else  // !D_FILESYS_ENABLE_POSIX

// POSIX backend not enabled for this build. Without it this header declares
// nothing, rather than stopping the build: a disabled backend is absent, and
// naming its scanner fails at the point of use (see os_scanner in
// file_tree.hpp). It is a foreign backend on the current target; set
// D_CFG_FILESYS_ALLOW_POSIX (or D_CFG_FILESYS_ALLOW_POSIX_FAMILY, or
// D_CFG_FILESYS_ALLOW_FOREIGN) to 1 before including file_tree.hpp to permit
// it.

#endif  // D_FILESYS_ENABLE_POSIX

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_POSIX_HPP
