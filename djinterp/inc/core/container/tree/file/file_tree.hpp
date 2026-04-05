/******************************************************************************
* djinterp [core]                                             file_tree.hpp
*
* Portable arena-ed file/directory tree:
*   This header provides a cache-friendly, index-based representation
* of a filesystem subtree.  Built on top of arena<file_entry>, it adds
* a string pool for names, a hash index for O(1) child-by-name lookup,
* and cross-platform directory scanning.
*
*   The scanning layer reads a real directory tree into the arena in a
* single pass.  After population, path resolution, traversal, and
* subtree operations are all arena-speed — no OS calls, no heap
* chasing.
*
* Contents:
*   - file_type          enumeration of entry kinds
*   - file_entry         payload for arena_node
*   - file_tree          arena + string pool + name index + scanner
*
* Platform support:
*   - Win32    FindFirstFileW / FindNextFileW
*   - POSIX    opendir / readdir / lstat
*
* Encoding:
*   - All names stored as UTF-8 regardless of platform.
*   - Win32 paths converted at the API boundary.
*
*
* path:      /inc/cpp/fs/file_tree.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_
#define DJINTERP_FS_FILE_TREE_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include "../djinterp.hpp"
#include "../arena/arena.hpp"


// ================================================================
//  platform headers
// ================================================================

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#else
    // fall: attempt POSIX headers — most non-Windows hosted
    // environments provide them.
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif


// D_KEYWORD_FS
//   keyword: resolves to `fs`.
#define D_KEYWORD_FS                fs

// NS_FS
//   namespace: the filesystem subsystem namespace.
#define NS_FS                       D_NAMESPACE(D_KEYWORD_FS)


NS_DJINTERP
NS_FS


// bring arena types into this namespace.
using djinterp::container::node_id;
using djinterp::container::null_node;
using djinterp::container::arena;
using djinterp::container::arena_node;


// ================================================================
//  file_type
// ================================================================

// file_type
//   enum: the kind of filesystem entry a node represents.
enum file_type : std::uint8_t
{
    file_type_unknown   = 0,
    file_type_regular   = 1,
    file_type_directory = 2,
    file_type_symlink   = 3,
    file_type_other     = 4
};


// ================================================================
//  file_entry
// ================================================================

// file_entry
//   struct: payload stored in each arena_node.  Names are not
// inlined — they live in a separate string pool, referenced
// by offset and length.  This keeps node size fixed and small.
struct file_entry
{
    std::uint32_t   name_offset;
    std::uint16_t   name_length;
    file_type       type;
    std::uint8_t    _pad0;
    std::uint64_t   size;

    // file_entry (default)
    //   constructs an empty entry.
    file_entry
    ()
        : name_offset  (0),
          name_length  (0),
          type         (file_type_unknown),
          _pad0        (0),
          size         (0)
    {}

    // file_entry (parameterized)
    //   constructs an entry with the given name pool location,
    // type, and file size.
    file_entry
    (
        std::uint32_t _name_offset,
        std::uint16_t _name_length,
        file_type     _type,
        std::uint64_t _size
    )
        : name_offset  (_name_offset),
          name_length  (_name_length),
          type         (_type),
          _pad0        (0),
          size         (_size)
    {}
};


// ================================================================
//  file_tree
// ================================================================

// file_tree
//   class: an arena-ed file/directory tree with a pooled
// string table for names and a hash index for O(1) child-by-name
// resolution.
//
//   Typical use:
//     file_tree ft;
//     ft.scan("/some/path");
//     node_id n = ft.resolve("src/core/main.cpp");
//
class file_tree
{
public:
    using node_type = arena_node<file_entry>;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // file_tree (default)
    //   constructs an empty tree.
    file_tree
    ()
        : m_arena       (),
          m_names        (),
          m_lookup       (),
          m_next_stable  (1)
    {}

    // --------------------------------------------------------
    //  scanning
    // --------------------------------------------------------

    // scan
    //   populates the tree from the directory at _path.
    // Clears any existing content first.  Returns the root
    // node_id, or null_node on failure.
    node_id
    scan
    (
        const char* _path
    )
    {
        clear();

        // create root node.
        file_entry root_entry = make_entry(
            _path,
            file_type_directory,
            0
        );

        node_id root = m_arena.allocate(
            alloc_stable_id(),
            static_cast<file_entry&&>(root_entry)
        );

        scan_impl(std::string(_path), root);

        return root;
    }

    // scan (std::string overload)
    //   populates the tree from the directory at _path.
    node_id
    scan
    (
        const std::string& _path
    )
    {
        return scan(_path.c_str());
    }


    // --------------------------------------------------------
    //  path resolution
    // --------------------------------------------------------

    // resolve
    //   resolves a relative path (components separated by '/')
    // from _root.  Returns the target node_id, or null_node
    // if any component is not found.
    node_id
    resolve
    (
        const char* _path,
        node_id     _root = 0
    ) const
    {
        node_id current = _root;

        const char* p   = _path;
        const char* end = _path + std::strlen(_path);

        while (p < end && current != null_node)
        {
            // skip leading separators.
            while (p < end && (*p == '/' || *p == '\\'))
            {
                ++p;
            }

            if (p >= end)
            {
                break;
            }

            // find end of component.
            const char* comp_start = p;

            while (p < end && *p != '/' && *p != '\\')
            {
                ++p;
            }

            std::size_t comp_len = static_cast<std::size_t>(
                p - comp_start
            );

            if (comp_len == 0)
            {
                continue;
            }

            current = find_child(
                current,
                comp_start,
                comp_len
            );
        }

        return current;
    }

    // resolve (std::string overload)
    //   resolves a relative path from _root.
    node_id
    resolve
    (
        const std::string& _path,
        node_id            _root = 0
    ) const
    {
        return resolve(_path.c_str(), _root);
    }


    // --------------------------------------------------------
    //  name access
    // --------------------------------------------------------

    // name
    //   returns the name of the node at _id as a pointer into
    // the string pool.  The pointer is valid until the next
    // mutation of the tree.  Optionally writes the length to
    // _length.
    const char*
    name
    (
        node_id      _id,
        std::size_t* _length = nullptr
    ) const
    {
        const file_entry& e = m_arena[_id].data;

        if (_length != nullptr)
        {
            *_length = e.name_length;
        }

        return m_names.data() + e.name_offset;
    }

    // name_str
    //   returns the name of the node at _id as a std::string.
    std::string
    name_str
    (
        node_id _id
    ) const
    {
        const file_entry& e = m_arena[_id].data;

        return m_names.substr(e.name_offset, e.name_length);
    }

    // full_path
    //   reconstructs the full path from root to _id by
    // walking the parent chain.
    std::string
    full_path
    (
        node_id _id,
        char    _sep = '/'
    ) const
    {
        // collect ancestors into a stack via the arena links.
        std::vector<node_id> chain;
        node_id current = _id;

        while (current != null_node)
        {
            chain.push_(current);
            current = m_arena[current].parent;
        }

        std::string result;

        for (std::size_t i = chain.size(); i > 0; --i)
        {
            const file_entry& e = m_arena[chain[i - 1]].data;

            if (!result.empty())
            {
                result += _sep;
            }

            result.append(
                m_names.data() + e.name_offset,
                e.name_length
            );
        }

        return result;
    }


    // --------------------------------------------------------
    //  element access (forwarded to arena)
    // --------------------------------------------------------

    // operator[]
    //   returns a mutable reference to the node at _id.
    node_type&
    operator[]
    (
        node_id _id
    )
    {
        return m_arena[_id];
    }

    // operator[] (const)
    //   returns an immutable reference to the node at _id.
    const node_type&
    operator[]
    (
        node_id _id
    ) const
    {
        return m_arena[_id];
    }

    // size
    //   returns the total number of nodes in the tree.
    std::size_t
    size() const
    {
        return m_arena.size();
    }

    // empty
    //   returns true if the tree contains no nodes.
    bool
    empty() const
    {
        return m_arena.empty();
    }

    // nodes
    //   returns a const reference to the underlying arena.
    const arena<file_entry>&
    nodes() const
    {
        return m_arena;
    }

    // nodes (mutable)
    //   returns a mutable reference to the underlying arena.
    arena<file_entry>&
    nodes()
    {
        return m_arena;
    }


    // --------------------------------------------------------
    //  traversal
    // --------------------------------------------------------

    // visit_depth_first
    //   invokes _fn(node_id, depth) in pre-order.
    // Uses prev_sibling to push children in reverse — zero
    // temporary allocations beyond the stack itself.
    template<typename _Fn>
    void
    visit_depth_first
    (
        node_id _root,
        _Fn     _fn
    ) const
    {
        struct frame
        {
            node_id     id;
            std::size_t depth;
        };

        std::vector<frame> stack;
        stack.push_({ _root, 0 });

        while (!stack.empty())
        {
            frame f = stack.();
            stack.pop_();

            _fn(f.id, f.depth);

            // push children in reverse (last → first) so that
            // the leftmost child is popped first.
            node_id c = m_arena[f.id].last_child;

            while (c != null_node)
            {
                stack.push_({ c, f.depth + 1 });
                c = m_arena[c].prev_sibling;
            }
        }

        return;
    }

    // visit_breadth_first
    //   invokes _fn(node_id, depth) in BFS order.
    // Uses the output queue as its own ring — no extra
    // allocations.
    template<typename _Fn>
    void
    visit_breadth_first
    (
        node_id _root,
        _Fn     _fn
    ) const
    {
        struct frame
        {
            node_id     id;
            std::size_t depth;
        };

        std::vector<frame> queue;
        queue.push_({ _root, 0 });

        std::size_t scan = 0;

        while (scan < queue.size())
        {
            frame f = queue[scan];
            ++scan;

            _fn(f.id, f.depth);

            node_id c = m_arena[f.id].first_child;

            while (c != null_node)
            {
                queue.push_({ c, f.depth + 1 });
                c = m_arena[c].next_sibling;
            }
        }

        return;
    }


    // --------------------------------------------------------
    //  mutation
    // --------------------------------------------------------

    // clear
    //   resets the tree to an empty state.
    void
    clear()
    {
        m_arena = arena<file_entry>();
        m_names.clear();
        m_lookup.clear();
        m_next_stable = 1;

        return;
    }

    // add_child
    //   manually inserts a child node under _parent with the
    // given _name, _type, and optional _size.  Returns the
    // new node's id.
    node_id
    add_child
    (
        node_id       _parent,
        const char*   _name,
        file_type     _type,
        std::uint64_t _size = 0
    )
    {
        std::size_t len = std::strlen(_name);

        file_entry entry(
            static_cast<std::uint32_t>(m_names.size()),
            static_cast<std::uint16_t>(len),
            _type,
            _size
        );

        m_names.append(_name, len);

        node_id id = m_arena.allocate(
            alloc_stable_id(),
            static_cast<file_entry&&>(entry)
        );

        m_arena.append_child(_parent, id);
        register_lookup(_parent, _name, len, id);

        return id;
    }


private:

    // --------------------------------------------------------
    //  string pool
    // --------------------------------------------------------

    // make_entry
    //   creates a file_entry, extracting the leaf component
    // from _name and appending it to the string pool.
    file_entry
    make_entry
    (
        const char*   _name,
        file_type     _type,
        std::uint64_t _size
    )
    {
        // extract the leaf name from the path.
        const char* leaf = _name;
        const char* p    = _name;

        while (*p != '\0')
        {
            if (*p == '/' || *p == '\\')
            {
                leaf = p + 1;
            }

            ++p;
        }

        std::size_t len = static_cast<std::size_t>(p - leaf);

        // handle trailing separator (e.g. "/foo/bar/").
        if (len == 0 && leaf > _name)
        {
            const char* comp = _name;

            for (const char* s = _name; s < (p - 1); ++s)
            {
                if (*s == '/' || *s == '\\')
                {
                    comp = s + 1;
                }
            }

            leaf = comp;
            len  = static_cast<std::size_t>(p - leaf);

            // strip trailing separator from length.
            if (len > 0 && (leaf[len - 1] == '/' ||
                            leaf[len - 1] == '\\'))
            {
                --len;
            }
        }

        std::uint32_t offset = static_cast<std::uint32_t>(
            m_names.size()
        );

        m_names.append(leaf, len);

        return file_entry(
            offset,
            static_cast<std::uint16_t>(len),
            _type,
            _size
        );
    }


    // --------------------------------------------------------
    //  hash index
    // --------------------------------------------------------

    // lookup_key
    //   combines parent id and name into a single uint64 hash
    // using FNV-1a seeded with the parent index.
    D_STATIC_INLINE
    std::uint64_t
    lookup_key
    (
        node_id     _parent,
        const char* _name,
        std::size_t _len
    )
    {
        std::uint64_t h = static_cast<std::uint64_t>(_parent);

        h ^= UINT64_C(0xcbf29ce484222325);
        h *= UINT64_C(0x100000001b3);

        for (std::size_t i = 0; i < _len; ++i)
        {
            h ^= static_cast<std::uint64_t>(
                static_cast<unsigned char>(_name[i])
            );
            h *= UINT64_C(0x100000001b3);
        }

        return h;
    }

    // register_lookup
    //   inserts a (parent, name) → node_id mapping into the
    // hash index.
    void
    register_lookup
    (
        node_id     _parent,
        const char* _name,
        std::size_t _len,
        node_id     _id
    )
    {
        std::uint64_t key = lookup_key(_parent, _name, _len);

        m_lookup[key] = _id;

        return;
    }

    // find_child
    //   looks up a child of _parent by name.  Returns null_node
    // if not found.  Verifies against the string pool to guard
    // against hash collisions.
    node_id
    find_child
    (
        node_id     _parent,
        const char* _name,
        std::size_t _len
    ) const
    {
        std::uint64_t key = lookup_key(_parent, _name, _len);

        auto it = m_lookup.find(key);

        if (it == m_lookup.end())
        {
            return null_node;
        }

        // verify against the string pool to handle hash
        // collisions.
        const file_entry& e = m_arena[it->second].data;

        if (e.name_length != static_cast<std::uint16_t>(_len))
        {
            return null_node;
        }

        if (std::memcmp(
                m_names.data() + e.name_offset,
                _name,
                _len) != 0)
        {
            return null_node;
        }

        return it->second;
    }


    // --------------------------------------------------------
    //  stable id
    // --------------------------------------------------------

    // alloc_stable_id
    //   returns the next stable identity value.
    std::uint64_t
    alloc_stable_id()
    {
        return m_next_stable++;
    }


    // --------------------------------------------------------
    //  platform scanning
    // --------------------------------------------------------

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

    // ============================================
    //  Win32 implementation
    // ============================================

    // widen
    //   converts a UTF-8 string to a wide string for Win32
    // APIs.
    D_STATIC_INLINE
    std::wstring
    widen
    (
        const std::string& _utf8
    )
    {
        if (_utf8.empty())
        {
            return std::wstring();
        }

        int needed = MultiByteToWideChar(
            CP_UTF8,
            0,
            _utf8.data(),
            static_cast<int>(_utf8.size()),
            nullptr,
            0
        );

        std::wstring out(
            static_cast<std::size_t>(needed),
            L'\0'
        );

        MultiByteToWideChar(
            CP_UTF8,
            0,
            _utf8.data(),
            static_cast<int>(_utf8.size()),
            &out[0],
            needed
        );

        return out;
    }

    // narrow
    //   converts a wide string to UTF-8 for internal storage.
    D_STATIC_INLINE
    std::string
    narrow
    (
        const wchar_t* _wide,
        int            _len = -1
    )
    {
        if (_len == 0)
        {
            return std::string();
        }

        int needed = WideCharToMultiByte(
            CP_UTF8,
            0,
            _wide,
            _len,
            nullptr,
            0,
            nullptr,
            nullptr
        );

        std::string out(
            static_cast<std::size_t>(needed),
            '\0'
        );

        WideCharToMultiByte(
            CP_UTF8,
            0,
            _wide,
            _len,
            &out[0],
            needed,
            nullptr,
            nullptr
        );

        return out;
    }

    // scan_impl (Win32)
    //   recursively reads a directory tree using the Win32 API
    // and populates the arena.
    void
    scan_impl
    (
        const std::string& _dir_path,
        node_id            _parent
    )
    {
        std::wstring pattern = widen(_dir_path + "\\*");

        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            // skip . and ..
            if (fd.cFileName[0] == L'.')
            {
                if (fd.cFileName[1] == L'\0')
                {
                    continue;
                }

                if (fd.cFileName[1] == L'.' &&
                    fd.cFileName[2] == L'\0')
                {
                    continue;
                }
            }

            std::string child_name = narrow(fd.cFileName);

            file_type type = file_type_other;

            if (fd.dwFileAttributes &
                FILE_ATTRIBUTE_REPARSE_POINT)
            {
                type = file_type_symlink;
            }
            else if (fd.dwFileAttributes &
                     FILE_ATTRIBUTE_DIRECTORY)
            {
                type = file_type_directory;
            }
            else
            {
                type = file_type_regular;
            }

            std::uint64_t sz =
                (static_cast<std::uint64_t>(
                    fd.nFileSizeHigh) << 32) |
                static_cast<std::uint64_t>(fd.nFileSizeLow);

            // pool the name.
            std::uint32_t name_off = static_cast<std::uint32_t>(
                m_names.size()
            );
            std::uint16_t name_len = static_cast<std::uint16_t>(
                child_name.size()
            );

            m_names.append(child_name);

            file_entry entry(name_off, name_len, type, sz);

            node_id id = m_arena.allocate(
                alloc_stable_id(),
                static_cast<file_entry&&>(entry)
            );

            m_arena.append_child(_parent, id);
            register_lookup(
                _parent,
                child_name.c_str(),
                child_name.size(),
                id
            );

            if (type == file_type_directory)
            {
                scan_impl(
                    _dir_path + "\\" + child_name,
                    id
                );
            }

        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);

        return;
    }

#else

    // ============================================
    //  POSIX implementation (Linux, macOS, *BSD)
    // ============================================

    // scan_impl (POSIX)
    //   recursively reads a directory tree using POSIX APIs
    // and populates the arena.
    void
    scan_impl
    (
        const std::string& _dir_path,
        node_id            _parent
    )
    {
        DIR* dir = opendir(_dir_path.c_str());

        if (dir == nullptr)
        {
            return;
        }

        struct dirent* ent;

        while ((ent = readdir(dir)) != nullptr)
        {
            // skip . and ..
            if (ent->d_name[0] == '.')
            {
                if (ent->d_name[1] == '\0')
                {
                    continue;
                }

                if (ent->d_name[1] == '.' &&
                    ent->d_name[2] == '\0')
                {
                    continue;
                }
            }

            const char* child_name = ent->d_name;
            std::size_t child_len  = std::strlen(child_name);

            std::string child_path =
                _dir_path + "/" + child_name;

            // stat for size and type confirmation.
            struct stat st;

            if (lstat(child_path.c_str(), &st) != 0)
            {
                continue;
            }

            file_type type = file_type_other;

            if (S_ISLNK(st.st_mode))
            {
                type = file_type_symlink;
            }
            else if (S_ISDIR(st.st_mode))
            {
                type = file_type_directory;
            }
            else if (S_ISREG(st.st_mode))
            {
                type = file_type_regular;
            }

            std::uint64_t sz =
                static_cast<std::uint64_t>(st.st_size);

            // pool the name.
            std::uint32_t name_off = static_cast<std::uint32_t>(
                m_names.size()
            );
            std::uint16_t name_len = static_cast<std::uint16_t>(
                child_len
            );

            m_names.append(child_name, child_len);

            file_entry entry(name_off, name_len, type, sz);

            node_id id = m_arena.allocate(
                alloc_stable_id(),
                static_cast<file_entry&&>(entry)
            );

            m_arena.append_child(_parent, id);
            register_lookup(
                _parent,
                child_name,
                child_len,
                id
            );

            if (type == file_type_directory)
            {
                scan_impl(child_path, id);
            }
        }

        closedir(dir);

        return;
    }

#endif  // platform


    // --------------------------------------------------------
    //  members
    // --------------------------------------------------------

    arena<file_entry>                           m_arena;
    std::string                                 m_names;
    std::unordered_map<std::uint64_t, node_id>  m_lookup;
    std::uint64_t                               m_next_stable;
};


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_