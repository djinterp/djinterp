/******************************************************************************
* djinterp [fs]                                             file_tree_apple.hpp
*
* Apple (macOS) file tree scanner:
*   Defines apple_scanner, which uses getattrlistbulk to read directory
* entries and their attributes in batches - returning the name, object
* type, and data size for many entries in a single syscall, the pattern
* purpose-built for tree indexing on Apple platforms.
*
*   getattrlistbulk operates on an open directory fd and is drained in a
* loop until it returns 0.  Each pass yields a packed buffer of variable
* length attribute records, which the scanner walks field by field.
*
*   When the bulk API is unavailable (e.g. building against an old SDK),
* the scanner degrades to the BSD d_type path, which is correct on
* Darwin since its dirent populates d_type.
*
* NOTE: iOS shares the Darwin kernel but runs sandboxed; see
* file_tree_ios.hpp, which reuses this scanner under a sandbox-aware
* wrapper.
*
*
* path:      /inc/cpp/fs/file_tree_apple.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_APPLE_
#define DJINTERP_FS_FILE_TREE_APPLE_ 1

#include "./file_tree_common.hpp"
#include "./file_tree_bsd.hpp"     // fallback on Darwin

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>

#if defined(__APPLE__)
    #include <sys/attr.h>
    #include <sys/vnode.h>
    #include <unistd.h>
    // getattrlistbulk is available on macOS 10.10+.
    #if defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && \
        (__MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
        #define D_FS_HAVE_GETATTRLISTBULK 1
    #else
        #define D_FS_HAVE_GETATTRLISTBULK 0
    #endif
#else
    #define D_FS_HAVE_GETATTRLISTBULK 0
#endif


NS_DJINTERP
NS_FS


// ================================================================
//  apple_scanner
// ================================================================

// apple_scanner
//   policy: Darwin directory walk.  Prefers getattrlistbulk batch
// enumeration; falls back to the BSD d_type path otherwise.
struct apple_scanner
{
#if D_FS_HAVE_GETATTRLISTBULK

    // packed attribute layout requested from getattrlistbulk.
    // Order matches the attrgroup bit order, which is how the API
    // packs the per-entry record.
    struct attr_buf_entry
    {
        std::uint32_t length;       // total record length
        attribute_set_t returned;   // which attrs are present
        // followed by, in order:
        //   attrreference_t name_ref   (ATTR_CMN_NAME)
        //   fsobj_type_t    obj_type   (ATTR_CMN_OBJTYPE)
        //   off_t           data_size  (ATTR_FILE_DATALENGTH)
    };

    // map_objtype
    static file_type
    map_objtype(
        fsobj_type_t _t
    )
    {
        switch (_t)
        {
            case VDIR:  return file_type_directory;
            case VREG:  return file_type_regular;
            case VLNK:  return file_type_symlink;
            default:    return file_type_other;
        }
    }

    // scan_bulk
    //   getattrlistbulk-based enumeration.
    template<typename _Ctx>
    static void
    scan_bulk(
        _Ctx&              _ctx,
        const std::string& _dir_path,
        node_id            _parent
    )
    {
        int dfd = ::open(_dir_path.c_str(),
                         O_RDONLY | O_DIRECTORY);

        if (dfd < 0)
        {
            return;
        }

        struct attrlist al;
        std::memset(&al, 0, sizeof(al));
        al.bitmapcount = ATTR_BIT_MAP_COUNT;
        al.commonattr  = ATTR_CMN_RETURNED_ATTRS |
                         ATTR_CMN_NAME |
                         ATTR_CMN_OBJTYPE;
        al.fileattr    = ATTR_FILE_DATALENGTH;

        // 64 KiB batch buffer.
        char buf[64 * 1024];

        for (;;)
        {
            int count = ::getattrlistbulk(
                dfd, &al, buf, sizeof(buf), 0);

            if (count <= 0)
            {
                break;  // 0 = done, <0 = error
            }

            char* cursor = buf;

            for (int i = 0; i < count; ++i)
            {
                char*         field = cursor;
                std::uint32_t reclen =
                    *reinterpret_cast<std::uint32_t*>(field);
                field += sizeof(std::uint32_t);

                attribute_set_t returned =
                    *reinterpret_cast<attribute_set_t*>(field);
                field += sizeof(attribute_set_t);

                const char* name = nullptr;
                file_type   type = file_type_other;
                std::uint64_t sz = 0;

                if (returned.commonattr & ATTR_CMN_NAME)
                {
                    attrreference_t* nr =
                        reinterpret_cast<attrreference_t*>(field);
                    name = reinterpret_cast<const char*>(field)
                           + nr->attr_dataoffset;
                    field += sizeof(attrreference_t);
                }

                if (returned.commonattr & ATTR_CMN_OBJTYPE)
                {
                    fsobj_type_t* ot =
                        reinterpret_cast<fsobj_type_t*>(field);
                    type = map_objtype(*ot);
                    field += sizeof(fsobj_type_t);
                }

                if (returned.fileattr & ATTR_FILE_DATALENGTH)
                {
                    off_t* dl = reinterpret_cast<off_t*>(field);
                    sz = static_cast<std::uint64_t>(*dl);
                    field += sizeof(off_t);
                }

                if (name != nullptr &&
                    !posix_scanner::is_dot_entry(name))
                {
                    std::size_t nlen = std::strlen(name);

                    node_id id = _ctx.intern_child(
                        _parent, name, nlen, type, sz);

                    if (type == file_type_directory)
                    {
                        _ctx.recurse(
                            _dir_path + "/" + std::string(name, nlen),
                            id);
                    }
                }

                cursor += reclen;
            }
        }

        ::close(dfd);

        return;
    }

#endif  // D_FS_HAVE_GETATTRLISTBULK

    // scan
    template<typename _Ctx>
    static void
    scan(
        _Ctx&              _ctx,
        const std::string& _dir_path,
        node_id            _parent
    )
    {
#if D_FS_HAVE_GETATTRLISTBULK
        scan_bulk(_ctx, _dir_path, _parent);
#else
        bsd_scanner::scan(_ctx, _dir_path, _parent);
#endif
        return;
    }
};


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_APPLE_
