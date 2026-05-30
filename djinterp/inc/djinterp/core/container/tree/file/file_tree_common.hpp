/******************************************************************************
* djinterp [fs]                                            file_tree_common.hpp
*
* File tree common core (OS-independent):
*   This header carries the greatest common subset shared by every
* platform-specific file_tree backend: the payload type, the string
* pool, the (parent, name) hash index, path reconstruction, traversal,
* and manual mutation.  None of this code touches an OS API.
*
*   The platform seam is a single customization point: a *scanner
* policy*.  file_tree_core is templated on that policy and exposes a
* scan_context to it - a narrow interface granting exactly the
* operations a scanner needs (intern a name, allocate a node, link a
* child, recurse) without exposing core internals.  Each OS header
* (file_tree_posix.hpp, file_tree_linux.hpp, file_tree_windows.hpp, ...)
* supplies one scanner policy; the file_tree.hpp umbrella maps an
* operating_system enum value onto the right policy.
*
* Contents:
*   - operating_system    enum of selectable scan backends
*   - file_type           kind of filesystem entry
*   - file_entry          fixed-size arena payload (pooled name + type + size)
*   - scan_context        narrow builder interface handed to a scanner policy
*   - null_scanner        fallback policy (no OS support; scans nothing)
*   - file_tree_core      the OS-independent tree, templated on a scanner
*
* Encoding:
*   - All names are stored as UTF-8.  Wide-character platforms convert
*     at the scanner boundary before calling scan_context::intern_child.
*
*
* path:      /inc/cpp/fs/file_tree_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_COMMON_
#define DJINTERP_FS_FILE_TREE_COMMON_ 1

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "file_tree_common.hpp can only be used in C++ compilation mode"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
// djinterp
#include "../../../djinterp.hpp"
#include "../../arena/arena.hpp"


NS_DJINTERP
NS_FS


// bring in the arena vocabulary types we depend on.
using djinterp::container::node_id;
using djinterp::container::null_node;
using djinterp::container::arena;
using djinterp::container::arena_node;


// ================================================================
//  operating_system
// ================================================================

// operating_system
//   enum: selects which scan backend a file_tree instantiation
// uses.  Values intentionally mirror the env.h D_ENV_OS_FLAG_*
// families so a detected D_ENV_OS_ID can be folded onto one of
// these by the umbrella header.
//
//   The enum is the *public selector*; the umbrella maps each
// value onto a concrete scanner policy via os_scanner<>.  Members
// whose backend is not compiled in on the current build fall back
// to null_scanner (compiles, scans nothing) rather than failing to
// instantiate.
enum class operating_system : std::uint8_t
{
    // generic families
    automatic       = 0,    // detected from D_ENV_OS_ID at compile time
    posix           = 1,    // portable readdir + fstatat baseline

    // unix-like specializations
    linux_generic   = 2,    // getdents64 / statx fast path
    bsd             = 3,    // dirent::d_type + fstatat
    apple           = 4,    // macOS: getattrlistbulk batch metadata
    ios             = 5,    // iOS: sandbox-aware POSIX subset

    // windows
    windows         = 6,    // generic Win32 FindFirstFileExW
    windows10       = 7,    // Win32 with modern enumeration hints
    windows11       = 8,    // alias of windows10 backend

    // explicit no-op
    none            = 255   // null_scanner; never touches the OS
};


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
//   struct: payload stored in each arena node.  Names live in a
// separate string pool, referenced by offset + length, keeping the
// node fixed-size and cache-dense.
struct file_entry
{
    std::uint32_t name_offset;
    std::uint16_t name_length;
    file_type     type;
    std::uint8_t  _pad0;
    std::uint64_t size;

    // file_entry (default)
    file_entry()
        : name_offset (0),
          name_length (0),
          type        (file_type_unknown),
          _pad0       (0),
          size        (0)
    {}

    // file_entry (parameterized)
    file_entry(
        std::uint32_t _name_offset,
        std::uint16_t _name_length,
        file_type     _type,
        std::uint64_t _size
    )
        : name_offset (_name_offset),
          name_length (_name_length),
          type        (_type),
          _pad0       (0),
          size        (_size)
    {}
};


// ================================================================
//  scan_context
// ================================================================

// forward declaration so scan_context can befriend the core.
template<typename _Scanner>
class file_tree_core;

// scan_context
//   class: the narrow interface a scanner policy uses to build the
// tree.  It owns no state of its own - it is a thin, non-owning
// view onto a file_tree_core that exposes exactly four operations:
//
//     intern_child(parent, name, len, type, size) -> node_id
//         interns the UTF-8 name, allocates a node, links it under
//         parent, registers the (parent, name) lookup, and returns
//         the new node id.
//
//     recurse(scanner, dir_path, node)
//         re-enters the active scanner policy for a child directory.
//         Routed through the context so the policy never needs a
//         pointer back to the core or knowledge of its template
//         parameter.
//
//   This keeps every OS backend free of core internals (string pool
// layout, hash scheme, arena calls) and makes the backends trivially
// unit-testable against a mock context.
template<typename _Scanner>
class scan_context
{
public:
    using core_type = file_tree_core<_Scanner>;

    explicit scan_context(core_type& _core)
        : m_core(_core)
    {}

    // intern_child
    //   interns a child under _parent and returns its node id.
    node_id
    intern_child(
        node_id       _parent,
        const char*   _name,
        std::size_t   _len,
        file_type     _type,
        std::uint64_t _size
    )
    {
        return m_core.intern_child(_parent, _name, _len, _type, _size);
    }

    // recurse
    //   descends into a child directory via the active scanner.
    void
    recurse(
        const std::string& _dir_path,
        node_id            _node
    )
    {
        _Scanner::scan(*this, _dir_path, _node);
    }

private:
    core_type& m_core;
};


// ================================================================
//  null_scanner
// ================================================================

// null_scanner
//   policy: the fallback scanner.  Compiles everywhere and scans
// nothing.  Selected when a requested operating_system has no
// backend compiled into the current build, so that
// file_tree<operating_system::X> always instantiates.
struct null_scanner
{
    // scan
    //   no-op.  A null scanner produces a root-only tree.
    template<typename _Ctx>
    static void
    scan(
        _Ctx&              /*_ctx*/,
        const std::string& /*_dir_path*/,
        node_id            /*_parent*/
    )
    {
        return;
    }
};


// ================================================================
//  file_tree_core
// ================================================================

// file_tree_core
//   class: the OS-independent file tree.  Holds the arena, the
// string pool, and the (parent, name) hash index, and implements
// everything that does not require an OS call: path resolution,
// name access, full-path reconstruction, traversal, and manual
// mutation.  Directory population is delegated to the _Scanner
// policy through a scan_context.
template<typename _Scanner>
class file_tree_core
{
public:
    using scanner_type = _Scanner;
    using node_type    = arena_node<file_entry>;
    using context_type = scan_context<_Scanner>;

    friend class scan_context<_Scanner>;

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    file_tree_core()
        : m_arena      (),
          m_names      (),
          m_lookup     (),
          m_next_stable(1)
    {}

    // --------------------------------------------------------
    //  scanning
    // --------------------------------------------------------

    // scan
    //   populates the tree from the directory at _path using the
    // bound scanner policy.  Clears existing content first.
    // Returns the root node_id, or null_node on failure.
    node_id
    scan(
        const char* _path
    )
    {
        clear();

        file_entry root_entry = make_entry(
            _path, file_type_directory, 0);

        node_id root = m_arena.allocate(
            alloc_stable_id(),
            static_cast<file_entry&&>(root_entry));

        context_type ctx(*this);
        _Scanner::scan(ctx, std::string(_path), root);

        return root;
    }

    // scan (std::string overload)
    node_id
    scan(
        const std::string& _path
    )
    {
        return scan(_path.c_str());
    }

    // --------------------------------------------------------
    //  path resolution
    // --------------------------------------------------------

    // resolve
    //   resolves a '/'- or '\\'-separated relative path from _root.
    // Returns the target node_id, or null_node if any component is
    // missing.
    node_id
    resolve(
        const char* _path,
        node_id     _root = 0
    ) const
    {
        node_id current = _root;

        const char* p   = _path;
        const char* end = _path + std::strlen(_path);

        while (p < end && current != null_node)
        {
            while (p < end && (*p == '/' || *p == '\\'))
            {
                ++p;
            }

            if (p >= end)
            {
                break;
            }

            const char* comp_start = p;

            while (p < end && *p != '/' && *p != '\\')
            {
                ++p;
            }

            std::size_t comp_len =
                static_cast<std::size_t>(p - comp_start);

            if (comp_len == 0)
            {
                continue;
            }

            current = find_child(current, comp_start, comp_len);
        }

        return current;
    }

    // resolve (std::string overload)
    node_id
    resolve(
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
    //   returns the pooled name pointer for _id.  Valid until the
    // next mutation.  Optionally writes the length to _length.
    const char*
    name(
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
    //   returns the name of _id as a std::string.
    std::string
    name_str(
        node_id _id
    ) const
    {
        const file_entry& e = m_arena[_id].data;

        return m_names.substr(e.name_offset, e.name_length);
    }

    // full_path
    //   reconstructs the full path from root to _id by walking the
    // parent chain.
    std::string
    full_path(
        node_id _id,
        char    _sep = '/'
    ) const
    {
        std::vector<node_id> chain;
        node_id current = _id;

        while (current != null_node)
        {
            chain.push_back(current);
            current = m_arena[current].parent();
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
                m_names.data() + e.name_offset, e.name_length);
        }

        return result;
    }

    // --------------------------------------------------------
    //  element access (forwarded to arena)
    // --------------------------------------------------------

    node_type&       operator[](node_id _id)       { return m_arena[_id]; }
    const node_type& operator[](node_id _id) const { return m_arena[_id]; }

    std::size_t size()  const { return m_arena.size();  }
    bool        empty() const { return m_arena.empty(); }

    const arena<file_entry>& nodes() const { return m_arena; }
    arena<file_entry>&       nodes()       { return m_arena; }

    // --------------------------------------------------------
    //  traversal
    // --------------------------------------------------------

    // visit_depth_first
    //   invokes _fn(node_id, depth) in pre-order.
    template<typename _Fn>
    void
    visit_depth_first(
        node_id _root,
        _Fn     _fn
    ) const
    {
        struct frame { node_id id; std::size_t depth; };

        std::vector<frame> stack;
        stack.push_back({ _root, 0 });

        while (!stack.empty())
        {
            frame f = stack.back();
            stack.pop_back();

            _fn(f.id, f.depth);

            node_id c = m_arena[f.id].last_child();

            while (c != null_node)
            {
                stack.push_back({ c, f.depth + 1 });
                c = m_arena[c].prev_sibling();
            }
        }

        return;
    }

    // visit_breadth_first
    //   invokes _fn(node_id, depth) in level order.
    template<typename _Fn>
    void
    visit_breadth_first(
        node_id _root,
        _Fn     _fn
    ) const
    {
        struct frame { node_id id; std::size_t depth; };

        std::vector<frame> queue;
        queue.push_back({ _root, 0 });

        std::size_t scan_head = 0;

        while (scan_head < queue.size())
        {
            frame f = queue[scan_head];
            ++scan_head;

            _fn(f.id, f.depth);

            node_id c = m_arena[f.id].first_child;

            while (c != null_node)
            {
                queue.push_back({ c, f.depth + 1 });
                c = m_arena[c].next_sibling();
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
    //   manually inserts a child under _parent.  Returns the new id.
    node_id
    add_child(
        node_id       _parent,
        const char*   _name,
        file_type     _type,
        std::uint64_t _size = 0
    )
    {
        return intern_child(
            _parent, _name, std::strlen(_name), _type, _size);
    }

private:

    // --------------------------------------------------------
    //  scanner-facing builder (used via scan_context)
    // --------------------------------------------------------

    // intern_child
    //   interns _name, allocates a node, links it under _parent, and
    // registers the lookup.  The single mutation primitive shared by
    // add_child and every OS scanner.
    node_id
    intern_child(
        node_id       _parent,
        const char*   _name,
        std::size_t   _len,
        file_type     _type,
        std::uint64_t _size
    )
    {
        std::uint32_t name_off =
            static_cast<std::uint32_t>(m_names.size());
        std::uint16_t name_len =
            static_cast<std::uint16_t>(_len);

        m_names.append(_name, _len);

        file_entry entry(name_off, name_len, _type, _size);

        node_id id = m_arena.allocate(
            alloc_stable_id(),
            static_cast<file_entry&&>(entry));

        m_arena.append_child(_parent, id);
        register_lookup(_parent, _name, _len, id);

        return id;
    }

    // --------------------------------------------------------
    //  string pool
    // --------------------------------------------------------

    // make_entry
    //   creates a file_entry for the leaf component of _name and
    // appends that leaf to the string pool.
    file_entry
    make_entry(
        const char*   _name,
        file_type     _type,
        std::uint64_t _size
    )
    {
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

        // trailing separator (e.g. "/foo/bar/").
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

            if (len > 0 && (leaf[len - 1] == '/' ||
                            leaf[len - 1] == '\\'))
            {
                --len;
            }
        }

        std::uint32_t offset =
            static_cast<std::uint32_t>(m_names.size());

        m_names.append(leaf, len);

        return file_entry(
            offset, static_cast<std::uint16_t>(len), _type, _size);
    }

    // --------------------------------------------------------
    //  hash index
    // --------------------------------------------------------

    // lookup_key
    //   FNV-1a over the name, seeded with the parent index.
    D_STATIC_INLINE
    std::uint64_t
    lookup_key(
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
                static_cast<unsigned char>(_name[i]));
            h *= UINT64_C(0x100000001b3);
        }

        return h;
    }

    // register_lookup
    void
    register_lookup(
        node_id     _parent,
        const char* _name,
        std::size_t _len,
        node_id     _id
    )
    {
        m_lookup[lookup_key(_parent, _name, _len)] = _id;

        return;
    }

    // find_child
    //   resolves a child of _parent by name, verifying against the
    // pool to guard against hash collisions.
    node_id
    find_child(
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

        const file_entry& e = m_arena[it->second].data;

        if (e.name_length != static_cast<std::uint16_t>(_len))
        {
            return null_node;
        }

        if (std::memcmp(
                m_names.data() + e.name_offset, _name, _len) != 0)
        {
            return null_node;
        }

        return it->second;
    }

    // --------------------------------------------------------
    //  stable id
    // --------------------------------------------------------

    std::uint64_t alloc_stable_id() { return m_next_stable++; }

    // --------------------------------------------------------
    //  members
    // --------------------------------------------------------

    arena<file_entry>                          m_arena;
    std::string                                m_names;
    std::unordered_map<std::uint64_t, node_id> m_lookup;
    std::uint64_t                              m_next_stable;
};


// ================================================================
//  lookup_key helper for scanners (free function)
// ================================================================
// Scanners only ever need intern_child + recurse via scan_context;
// nothing else from the core is part of the scanner contract.


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_COMMON_
