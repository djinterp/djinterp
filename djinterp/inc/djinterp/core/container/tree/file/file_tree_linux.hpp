/******************************************************************************
* djinterp [fs]                                             file_tree_linux.hpp
*
* Linux file tree scanner:
*   Defines linux_scanner, which builds on the BSD d_type fast path and
* upgrades the per-entry metadata call from fstatat to statx where the
* kernel and libc provide it (Linux 4.11+, glibc 2.28+).  statx lets the
* scanner request only the fields file_entry needs via a mask
* (STATX_TYPE | STATX_SIZE), and - unlike stat - can return the file
* birth time (STATX_BTIME) that plain POSIX cannot.
*
*   statx availability is probed at compile time via <sys/stat.h> /
* __NR_statx.  When it is unavailable the scanner degrades to the BSD
* d_type + fstatat path, so the header compiles and runs on older
* kernels and on non-glibc libcs.
*
*
* path:      /inc/cpp/fs/file_tree_linux.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_LINUX_
#define DJINTERP_FS_FILE_TREE_LINUX_ 1

#include "./file_tree_common.hpp"
#include "./file_tree_bsd.hpp"     // d_type fast path + fallback

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <string>


// statx availability probe.
#if defined(__linux__) && defined(STATX_TYPE) && defined(AT_STATX_SYNC_AS_STAT)
    #define D_FS_HAVE_STATX 1
#else
    #define D_FS_HAVE_STATX 0
#endif


NS_DJINTERP
NS_FS


// ================================================================
//  linux_scanner
// ================================================================

// linux_scanner
//   policy: Linux directory walk.  Consults d_type, then issues a
// masked statx (or fstatat fallback) only when a size or a missing
// type must be resolved.
struct linux_scanner
{
#if D_FS_HAVE_STATX

    // statx_size_type
    //   resolves type (if unknown) and size for _name relative to
    // _dfd via a masked statx.  Returns true on success.
    static bool
    statx_size_type(
        int          _dfd,
        const char*  _name,
        file_type&   _io_type,
        std::uint64_t& _out_size
    )
    {
        struct statx stx;

        unsigned int mask = STATX_TYPE | STATX_SIZE;

        if (::statx(_dfd, _name,
                    AT_SYMLINK_NOFOLLOW | AT_STATX_DONT_SYNC,
                    mask, &stx) != 0)
        {
            return false;
        }

        if (_io_type == file_type_unknown)
        {
            mode_t m = stx.stx_mode;

            if (S_ISLNK(m))      { _io_type = file_type_symlink;   }
            else if (S_ISDIR(m)) { _io_type = file_type_directory; }
            else if (S_ISREG(m)) { _io_type = file_type_regular;   }
            else                 { _io_type = file_type_other;     }
        }

        _out_size = static_cast<std::uint64_t>(stx.stx_size);

        return true;
    }

#endif  // D_FS_HAVE_STATX

    // resolve_size_type
    //   fills type (if unknown) and size, preferring statx and
    // falling back to fstatat.
    static bool
    resolve_size_type(
        int            _dfd,
        const char*    _name,
        file_type&     _io_type,
        std::uint64_t& _out_size
    )
    {
#if D_FS_HAVE_STATX
        return statx_size_type(_dfd, _name, _io_type, _out_size);
#else
        struct stat st;

        if (::fstatat(_dfd, _name, &st, AT_SYMLINK_NOFOLLOW) != 0)
        {
            return false;
        }

        if (_io_type == file_type_unknown)
        {
            _io_type = posix_scanner::classify(st.st_mode);
        }

        _out_size = static_cast<std::uint64_t>(st.st_size);

        return true;
#endif
    }

    // scan
    template<typename _Ctx>
    static void
    scan(
        _Ctx&              _ctx,
        const std::string& _dir_path,
        node_id            _parent
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
            file_type type = bsd_scanner::type_from_dtype(ent->d_type);
#else
            file_type type = file_type_unknown;
#endif
            std::uint64_t sz = 0;

            if (type == file_type_unknown ||
                type == file_type_regular)
            {
                if (!resolve_size_type(dfd, child_name, type, sz))
                {
                    if (type == file_type_unknown)
                    {
                        continue;
                    }
                }
            }

            node_id id = _ctx.intern_child(
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


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_LINUX_
