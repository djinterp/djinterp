/*******************************************************************************
* djinterp [core]                                           file_tree_common.hpp
*
* File tree common core (OS-independent):
*   This header carries the greatest common subset shared by every
* platform-specific file_tree backend: the payload type, the string pool, the
* (parent, name) hash index, ADDRESSING, traversal, filtering, and manual
* mutation.  None of this code touches an OS API.
*
*   The platform seam is a single customization point: a *scanner policy*.
* file_tree_core is templated on that policy and exposes a scan_context to it
* -
* a narrow interface granting exactly the operations a scanner needs (intern a
* name, link a child, recurse) without exposing core internals. Each OS header
* supplies one scanner policy; the file_tree.hpp umbrella maps an
* operating_system enum value onto the right policy.
*
* ============================================================================
* WHAT CHANGED, AND WHY
* ============================================================================
*   1. IT DID NOT COMPILE.  full_path(), visit_depth_first(), and
*      visit_breadth_first() reached arena_node's links as though they were
*      accessor FUNCTIONS -- id->parent(), .last_child(),
*      .prev_sibling(), .next_sibling() -- but arena_node exposes them as
*    public
*      MEMBERS, which is how file_tree_path.hpp and file_tree_filter.hpp read
* them, and how this file itself read .data and (in one place) .first_child.
* The
* error is latent because members of a class template are only checked when
* instantiated; the first call to full_path() is a hard error. FIXED. 2. THE
* ADDRESS DID NOT RESOLVE. full_path(n) walked the parent chain to the root
* and
* INCLUDED THE ROOT'S OWN NAME; resolve(p) starts AT the root and descends.
* They
* disagreed by exactly one component, so resolve(full_path(n)) == null_file_node,
* never n. The round trip -- the one contract an addressable container owes --
* was broken for every node in the tree, and it took every path predicate down
* with it: by_path_prefix("src") could never match, because every full_path
* began "project/...". THE ROOT CONTRIBUTES NO LABEL. address() now names a
* node
* the way resolve() reads it, and the round trip holds. 3. full_path() WAS
* ALSO
* NOT A PATH. scan("/home/me/project") kept only the LEAF of the scanned path
* as
* the root's name and discarded "/home/me", so full_path() returned
* "project/src/main.cpp" -- not an address in this tree, and not a path you
* could open either. The scan root is now retained, so full_path() is once
* again
* an openable filesystem path and address() is the tree-relative name. They
* are
* different things and this file no longer conflates them. 4. SEPARATION WAS
* ASSUMED, NEVER ENFORCED, AND SILENTLY BREAKABLE. The lookup is a map from
* (parent, name) to ONE id, written with `m_lookup[key] = id`. Two children of
* one parent bearing one name therefore did not collide loudly -- the second
* QUIETLY OVERWROTE the first, leaving a node that exists in the arena, hangs
* in
* the sibling chain, is walked by every traversal, and CANNOT BE ADDRESSED. A
* real filesystem never hands us that, but add_child() is public and did no
* check. Insertion now preserves separation by construction. 5. depth WAS THE
* LEVEL. visit_depth_first / visit_breadth_first report the distance DOWN FROM
* THE ROOT, which is the LEVEL (lambda); the spec reserves DEPTH for a node's
* HEIGHT, measured to its deepest leaf. Both now exist and are named apart.
* ============================================================================
* THE SPEC: A FILE TREE IS THE ADDRESSABLE CONTAINER
* ============================================================================
*   Every other hierarchical container has to be TOLD what its labels are.  A
* file tree is born with them: a filename IS a label, a path IS an address,
* and
* resolve() IS the fold of the descent step along a word of labels. This is
* the
* addressability axis in its native habitat, and that makes its two
* obligations
* concrete rather than abstract:
*
*   SEPARATION (mu_1, read sibling-wise).  A directory cannot hold two entries
* of one name. That is not our rule, it is the filesystem's, and it is exactly
* what makes a path resolve to at most one file.  The tree must not be able to
* represent a violation, or it stops being addressable while still looking
* fine.
*
*   THE ROUND TRIP.  resolve(address(n)) == n, for every n.  round_trips()
* checks it; the whole of section VI exists to keep it true.
*
*   FILTERING.  Dropping a directory severs every descent through it, and the
* addresses of everything beneath it cease to resolve. So a selection
* preserves
* the survivors' addresses IF AND ONLY IF the retained set is PREFIX-CLOSED
* (ancestor-closed) -- and "*.cpp" is not ancestor-closed, since no directory
* is
* named *.cpp.  Keeping only the matching nodes would therefore return a tree
* with no directories and no paths.  filter() keeps the ANCESTOR-CLOSURE: the
* matches AND the directories that lead to them. That is `tree -P '*.cpp'`,
* and
* it is the only total, addressability-preserving reading.  file_tree_query
* (file_tree_filter.hpp) is the OTHER reading -- a flat gather that builds no
* tree and so raises no structural question. Both are wanted; they are not the
* same operation.
*
*   RENAMING. A map over names keeps every position, and so keeps every path
* --
* but only while the names stay APART.  A rename that is not injective ON
* SIBLINGS collides two entries onto one name, and on a filesystem that is not
* an abstraction being violated, it is two files becoming one.
* rename_preserves_separation() decides it BEFORE the rename is applied.
*
* CASE FOLDING (a known, deliberate divergence):
*   The index compares names BYTE-EXACTLY.  NTFS and APFS (in its default
* configuration) fold case, so on those hosts "README" and "readme" name ONE
* file -- the filesystem's labelling is COARSER than ours.  A scan can never
* hand us both, so the tree stays separating; but resolve("readme") will not
* find "README".  The labelling function is the filesystem's, not the byte
* string's, and where the two differ this index follows the bytes.
* find_child()
* is public so a caller who needs the host's folding can descend on their own
* terms.
*
* Contents:
*   - file_type           kind of filesystem entry
*   - file_entry          fixed-size arena payload (pooled name + type + size)
*   - scan_context        narrow builder interface handed to a scanner policy
*   - null_scanner        fallback policy (no OS support; scans nothing)
*   - file_tree_core      the OS-independent tree, templated on a scanner
*
* Encoding:
*   - All names are stored as UTF-8.  Wide-character platforms convert at the
*     scanner boundary before calling scan_context::intern_child.
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    file_type
      ---------

II.   file_entry
      ----------

III.  scan_context
      ------------

IV.   null_scanner
      ------------

V.    file_tree_core
      --------------
      1.    axes and options surface
      2.    construction / scanning
      3.    ADDRESSABILITY  (root / find_child / resolve / address / full_path /

      round_trips / level / height / is_ancestor / lca)
      4.    name access
      5.    element access + traversal
      6.    FILTERABILITY   (filter / select / count_if)
      7.    TRANSFORMABILITY(transform / rename / rename_preserves_separation)
      8.    mutation        (clear / add_child)
*/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_COMMON_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_COMMON_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "file_tree_common.hpp can only be used in C++ compilation mode"
#endif

// std
#include <cstddef>
#include <cstring>
#include <new>            // placement new into a pool slot
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>  // filter()'s keep-set: a pointer indexes nothing
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"
#include "../../../../config/core/container/tree/file/cfg_filesys.h"
#include "./fs_os.hpp"
#include "../../../memory/pool.hpp"               // pool, raw_pool: the node pool
#include "../../container_options.hpp"  // the nine axes
#include "../../../meta/hierarchical.hpp"          // structure_category
#include "../../../meta/type_traits.hpp"           // clean_t, void_t
#include "../../../functional/function_traits.hpp"   // is_invocable_with
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t,
                                                      // uint16_t, uint32_t,
                                                      // uint64_t, uintptr_t
//   function_traits.hpp is where is_invocable_with, the one probe this header
// uses, is defined. (functional_common.hpp and predicate.hpp once declared
// two incompatible is_predicate templates and could not share a unit; since
// d5637dd there is one, variadic, in functional_common.hpp, and the file
// tree's filter, which takes predicate.hpp, shares a unit with either.)


NS_DJINTERP





// ================================================================
// I.   file_type
// ================================================================

// file_type
//   enum: the kind of filesystem entry a node represents.
enum file_type : re_std::uint8_t
{
    file_type_unknown   = 0,
    file_type_regular   = 1,
    file_type_directory = 2,
    file_type_symlink   = 3,
    file_type_other     = 4
};


// ================================================================
// II.  file_entry
// ================================================================

// file_entry
//   struct: payload stored in each arena node. Names live in a separate string
// pool, referenced by offset + length, keeping the node fixed-size and
// cache-dense. NOTE. A file_entry does NOT carry its name; it carries a
// REFERENCE into the pool. So a predicate over a bare file_entry can test type
// and size but cannot see the name -- which is why the useful predicate shape
// here is bool(file_node_id), taking the tree along. filter() accepts both (see
// section 6).
struct file_entry
{
    re_std::uint32_t name_offset;
    re_std::uint16_t name_length;
    file_type        type;
    re_std::uint8_t  _pad0;
    re_std::uint64_t size;

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
        re_std::uint32_t _name_offset,
        re_std::uint16_t _name_length,
        file_type        _type,
        re_std::uint64_t _size
    )
        : name_offset (_name_offset),
          name_length (_name_length),
          type        (_type),
          _pad0       (0),
          size        (_size)
    {}
};


// ================================================================
// 0.   NODE STORAGE  (arena -> pool)
// ================================================================
//   The index-based arena is GONE.  Nodes now live in a djinterp pool
// (memory/pool.hpp, over the C kernel d_pool), which vends raw slots
// (acquire()) and never moves one, so a node HANDLE is a POINTER, not a
// dense uint32 index into a vector.
//
//   file_node_id therefore changes meaning: it is still the opaque handle every
// caller passes around, resolve() returns, and every bool(file_node_id) predicate
// takes -- so the public surface survives -- but it no longer indexes anything.
// The two places that cared:
//     - lookup_key() seeded the hash with the parent's INDEX; it now seeds with
//       the parent's ADDRESS (uintptr_t).
//     - filter() marked its keep-set in a vector sized by arena.size(); a
//       pointer indexes nothing, so the keep-set is now a hash set.
//
//   AND ONE THING THAT MATTERS FAR MORE THAN EITHER  ->  see the static_assert
// in file_tree_core.  A pool is only pointer-stable under SOME block policies.

// file_node
//   struct: a node of the tree. Lives in the pool; linked to its neighbours by
// pointer. Was arena_node<file_entry>, whose links were dense indices.
struct file_node
{
    file_entry data;

    file_node* parent;
    file_node* first_child;
    file_node* last_child;
    file_node* next_sibling;
    file_node* prev_sibling;

    file_node()
        : data         (),
          parent       (nullptr),
          first_child  (nullptr),
          last_child   (nullptr),
          next_sibling (nullptr),
          prev_sibling (nullptr)
    {}
};

// file_node_id / null_file_node
//   the node HANDLE. A pointer into the pool, not an index.
using file_node_id = file_node*;

D_STATIC_CONSTEXPR file_node_id null_file_node = nullptr;


// ================================================================
// III. scan_context
// ================================================================

// forward declaration so scan_context can name the core. The trailing option
// pack carries the framework's per-axis options (container_options.hpp) into
// the container; it must appear here or the friendship below names a DIFFERENT
// template and grants nothing.
template<typename Scanner,
         typename Pool,
         typename... Options>
class file_tree_core;

// scan_context
//   class: the narrow interface a scanner policy uses to build the tree. It
// owns no state of its own - it is a thin, non-owning view onto a
// file_tree_core that exposes exactly two operations: intern_child(parent,
// name, len, type, size) -> file_node_id interns the UTF-8 name, allocates a node,
// links it under parent, registers the (parent, name) lookup, and returns the
// new node id. recurse(dir_path, node) re-enters the active scanner policy for
// a child directory. Routed through the context so the policy never needs a
// pointer back to the core or knowledge of its template parameters. Templated
// on the CORE, not the scanner, so a core carrying options is still nameable;
// the scanner is read back off it. Every scanner takes its context as a
// deduced template parameter, so no backend needs to change.
template<typename Core>
class scan_context
{
public:
    using core_type    = Core;
    using scanner_type = typename Core::scanner_type;

    explicit scan_context(core_type& _core)
        : m_core(_core)
    {}

    // intern_child
    //   interns a child under _parent and returns its node id. SEPARATION. If
    // _parent already holds a child of this name, that child's id is returned
    // and NO second node is made. A directory cannot hold two entries of one
    // name; permitting it here would create a node that every traversal walks
    // and no address reaches.
    file_node_id
    intern_child(
        file_node_id       _parent,
        const char*      _name,
        std::size_t      _len,
        file_type        _type,
        re_std::uint64_t _size
    )
    {
        return m_core.intern_child(_parent, _name, _len, _type, _size);
    }

    // recurse
    //   descends into a child directory via the active scanner.
    void
    recurse(
        const std::string& _dir_path,
        file_node_id            _node
    )
    {
        scanner_type::scan(*this, _dir_path, _node);
    }

private:
    core_type& m_core;
};


// ================================================================
// IV.  null_scanner
// ================================================================

// null_scanner
//   policy: the fallback scanner. Compiles everywhere and scans nothing.
// Selected when a requested operating_system has no backend compiled into the
// current build, so that file_tree<operating_system::X> always instantiates.
struct null_scanner
{
    // scan
    //   no-op. A null scanner produces a root-only tree.
    template<typename Ctx>
    static void
    scan(
        Ctx&              /*_ctx*/,
        const std::string& /*_dir_path*/,
        file_node_id            /*_parent*/
    )
    {
        return;
    }
};


// ================================================================
// V.   file_tree_core
// ================================================================

// file_tree_core
//   class: the OS-independent file tree. Holds the arena, the string pool, the
// scan root, and the (parent, name) hash index, and implements everything that
// does not require an OS call: ADDRESSING, name access, traversal, filtering,
// renaming, and manual mutation. Directory population is delegated to the
// Scanner policy through a scan_context.
//
//   Options... is a TRAILING pack carrying the framework's per-axis options
// (container_options.hpp), so every existing instantiation keeps compiling
// untouched.
template<typename Scanner,
         typename Pool = ::djinterp::pool<file_node>,
         typename... Options>
class file_tree_core
    : public options_container_base<Options...>
{
private:
    using option_base = options_container_base<Options...>;

public:
    using scanner_type = Scanner;
    using pool_type    = clean_t<Pool>;
    using node_type    = file_node;
    using context_type = scan_context<file_tree_core>;

    // ------------------------------------------------------------------
    //   THE GUARD.  Read this one.
    // ------------------------------------------------------------------
    //   Every link in this tree -- parent, first_child, last_child, and both
    // sibling pointers -- is a RAW POINTER into the pool, and so is every
    // file_node_id the caller is holding. So the pool must never move a slot.
    //
    //   The djinterp pool (memory/pool.hpp, over the C kernel d_pool) grows
    // by taking another block from its source and never moves a slot: every
    // block holds the same number of slots, and only the table of blocks is
    // ever reallocated. So any djinterp pool will do, and the guard checks
    // that the Pool IS one -- a pool of some other kind that grew by moving
    // its storage would turn every link, every file_node_id you are holding
    // and the root itself into dangling pointers, and would usually appear to
    // work while it did.
    static_assert(
        std::is_base_of<raw_pool, pool_type>::value,
        "file_tree stores every node link and every file_node_id as a raw pointer "
        "into the pool, so the pool must never move a slot. Use a djinterp pool "
        "(memory/pool.hpp): it grows by taking another block and never moves a "
        "slot.");

    // value_type
    //   the PAYLOAD, as the container traits mean it. Declared so
    // has_native_filter / has_native_transform can form their probe
    // expressions; see section 6 for why a payload predicate can see a node's
    // type and size but not its NAME.
    using value_type = file_entry;

    // level_type
    //   the LEVEL (lambda) of a node: its distance from the root, which is the
    // length of its address. Named apart from any "depth" because the spec's
    // DEPTH is a node's HEIGHT, and the two are different numbers on one node.
    using level_type = std::size_t;

    // keep_set
    //   the set filter() marks its ancestor-closure into. It was a vector
    // indexed by file_node_id; a file_node_id is a pointer now and indexes nothing.
    using keep_set = std::unordered_set<file_node_id>;

    friend class scan_context<file_tree_core>;

    // --------------------------------------------------------
    //  1.  axes and options surface
    // --------------------------------------------------------

    // structure_category
    //   the OPT-IN structure tag (structure/hierarchical.hpp) that
    // hierarchical_container_traits consults. A file tree is the hierarchical
    // container; this states it outright so no heuristic has to infer it.
    using structure_category = hierarchical;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    using typename option_base::options_type;

    static constexpr container_structure axis_structure =
        container_structure::hierarchical;   // not configurable

    static constexpr container_ordering axis_ordering =
        container_axis_value_v<options_type,
                               container_axis::ordering,
                               container_ordering::ordered>;

    static constexpr container_bounds axis_bounds =
        container_axis_value_v<options_type,
                               container_axis::bounds,
                               container_bounds::unbounded>;

    // MULTIPLICITY. Unique, and this is the axis that matters most here: a
    // directory cannot hold two entries of one name. That restriction IS mu_1
    // read sibling-wise, it IS what makes a path resolve to at most one file,
    // and it is enforced on insertion rather than hoped for.
    static constexpr container_multiplicity axis_multiplicity =
        container_axis_value_v<options_type,
                               container_axis::multiplicity,
                               container_multiplicity::unique>;

    static constexpr container_storage_kind axis_storage_kind =
        container_axis_value_v<options_type,
                               container_axis::storage_kind,
                               container_storage_kind::dynamic_storage>;

    static constexpr container_thread_safety axis_thread_safety =
        container_axis_value_v<options_type,
                               container_axis::thread_safety,
                               container_thread_safety::none>;

    static constexpr container_backing axis_backing =
        container_axis_value_v<options_type,
                               container_axis::backing,
                               container_backing::fundamental>;

    static constexpr container_iterability axis_iterability =
        container_axis_value_v<options_type,
                               container_axis::iterability,
                               container_iterability::iterable>;

    static_assert(
        container_axis_value_v<options_type,
                               container_axis::structure,
                               container_structure::hierarchical>
            == container_structure::hierarchical,
        "file_tree is hierarchical by construction: "
        "container_opt_structure<flat> cannot be honoured.");

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


    // --------------------------------------------------------
    //  2.  construction / scanning
    // --------------------------------------------------------

    file_tree_core()
        : m_pool       (),
          m_root       (nullptr),
          m_size       (0),
          m_names      (),
          m_lookup     (),
          m_root_path  ()
    {}

    // scan
    //   populates the tree from the directory at _path using the bound scanner
    // policy. Clears existing content first. Returns the root file_node_id, or
    // null_file_node on failure.
    //
    //   The SCANNED PATH IS RETAINED (m_root_path). It used to be thrown away
    // -- only the leaf survived, as the root's name -- which left full_path()
    // unable to name a file on disk.
    file_node_id
    scan(
        const char* _path
    )
    {
        clear();

        m_root_path = std::string(_path);

        file_entry root_entry = make_entry(
            _path, file_type_directory, 0);

        m_root = new_node(root_entry);

        file_node_id root = m_root;

        context_type ctx(*this);
        Scanner::scan(ctx, m_root_path, root);

        return root;
    }

    // scan (std::string overload)
    file_node_id
    scan(
        const std::string& _path
    )
    {
        return scan(_path.c_str());
    }


    // --------------------------------------------------------
    //  3.  ADDRESSABILITY
    // --------------------------------------------------------
    //   A filename is a LABEL.  A path is an ADDRESS.  resolve() is the DESCENT
    // STEP folded along a word of labels.  The three obligations:
    //
    //     SEPARATION   no directory holds two entries of one name.  Enforced on
    //                  insertion (see intern_child), so it cannot be violated.
    // THE ROOT contributes NO LABEL. |address(n)| == level(n), which is exactly
    // what resolve() consumes. ROUND TRIP resolve(address(n)) == n, for every
    // n. round_trips() checks it.

    // root
    //   the scan root, or null_file_node when the tree is empty. It is always arena
    // index 0: clear() empties the arena and scan() allocates the root first.
    file_node_id
    root() const
    {
        return m_root;
    }

    // root_path the path that was scanned - "/home/me/project", not "project".
    // Retained so full_path() can name a file on disk.
    const std::string&
    root_path() const
    {
        return m_root_path;
    }

    // find_child THE DESCENT STEP: the child of _parent bearing _name, or
    // null_file_node. One step of resolution, and the whole of it. O(1) through the
    // index, verified against the pool so a hash collision cannot produce a
    // false positive. PUBLIC. It used to be private, which left a caller with
    // no way to take a single step - and no way to descend on a host whose
    // name comparison is not ours (see the case-folding note in the banner).
    file_node_id
    find_child(
        file_node_id     _parent,
        const char* _name,
        std::size_t _len
    ) const
    {
        if (_parent == null_file_node)
        {
            return null_file_node;
        }

        re_std::uint64_t key = lookup_key(_parent, _name, _len);

        auto it = m_lookup.find(key);

        if (it == m_lookup.end())
        {
            return null_file_node;
        }

        const file_entry& e = it->second->data;

        if (e.name_length != static_cast<re_std::uint16_t>(_len))
        {
            return null_file_node;
        }

        if (std::memcmp(
                m_names.data() + e.name_offset, _name, _len) != 0)
        {
            return null_file_node;
        }

        return it->second;
    }

    // find_child (std::string overload)
    file_node_id
    find_child(
        file_node_id            _parent,
        const std::string& _name
    ) const
    {
        return find_child(_parent, _name.c_str(), _name.size());
    }

    // contains_child
    //   true if _parent already holds a child named _name. The separation test
    // at one directory, which is what add_child consults.
    bool
    contains_child(
        file_node_id            _parent,
        const std::string& _name
    ) const
    {
        return (find_child(_parent, _name.c_str(), _name.size()) != null_file_node);
    }

    // resolve the node an ADDRESS names, or null_file_node. Resolution is the
    // descent step FOLDED along the label word, one level per component,
    // starting at _root. Both '/' and '\\' are accepted as separators; empty
    // components are skipped, so a leading, trailing, or doubled separator is
    // harmless. resolve(address(n)) == n. That is the contract, and
    // round_trips() checks it.
    file_node_id
    resolve(
        const char* _path,
        file_node_id     _root = null_file_node
    ) const
    {
        file_node_id current = ( (_root == null_file_node) ? root() : _root );

        if (current == null_file_node)
        {
            return null_file_node;
        }

        const char* p   = _path;
        const char* end = _path + std::strlen(_path);

        while (p < end && current != null_file_node)
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
    file_node_id
    resolve(
        const std::string& _path,
        file_node_id            _root = null_file_node
    ) const
    {
        return resolve(_path.c_str(), _root);
    }

    // address
    //   addr(_id): the word of LABELS from the root down to _id, rendered with
    // _sep. THE ROOT CONTRIBUTES NO LABEL - addressing starts from it, it is
    // not a step taken - so the word has exactly level(_id) components, which
    // is exactly what resolve() consumes. resolve(address(n)) == n for every n
    // in the tree. The root's own address is the EMPTY word, and resolve("")
    // is the root. That is not a degenerate case to be patched around; it is
    // the base of the induction, and the thing the old full_path() got wrong.
    std::string
    address(
        file_node_id _id,
        char    _sep = '/'
    ) const
    {
        std::vector<file_node_id> chain;

        collect_labels(_id, chain);

        std::string result;

        for (std::size_t i = chain.size(); i > 0; --i)
        {
            const file_entry& e = chain[i - 1]->data;

            if (!result.empty())
            {
                result += _sep;
            }

            result.append(
                m_names.data() + e.name_offset, e.name_length);
        }

        return result;
    }

    // full_path
    //   the path of _id ON DISK: root_path() joined to address(_id). This is
    // what you open, print, or hand to a shell.
    //
    //   It is NOT an address in this tree, and it does not resolve() - the
    // scan root is not a node's label, it is where the tree begins. Those are
    // two different jobs and this file used to do neither: full_path()
    // returned "project/src/main.cpp", which named no file on disk AND
    // resolved to nothing. Use address() to name a node; use this to name a
    // file.
    std::string
    full_path(
        file_node_id _id,
        char    _sep = '/'
    ) const
    {
        std::string rel = address(_id, _sep);

        if (m_root_path.empty())
        {
            return rel;
        }

        if (rel.empty())
        {
            return m_root_path;
        }

        std::string result = m_root_path;

        // don't double the separator on a root path that ends in one
        char back = result[result.size() - 1];

        if (back != '/' && back != '\\')
        {
            result += _sep;
        }

        result += rel;

        return result;
    }

    // round_trips
    //   true if resolve(address(_id)) == _id -- the addressability contract,
    // made executable. It holds for every node of a separating tree, and
    // insertion keeps this tree separating, so it holds always. Provided
    // because a contract you cannot check is a hope.
    bool
    round_trips(
        file_node_id _id
    ) const
    {
        if (_id == null_file_node)
        {
            return false;
        }

        return (resolve(address(_id)) == _id);
    }

    // is_addressable
    //   true if EVERY node round-trips. An audit, not a hope: it should be
    // vacuously true, because intern_child refuses to create the node that
    // would falsify it. O(n * level).
    bool
    is_addressable() const
    {
        file_node_id r = root();

        if (r == null_file_node)
        {
            return true;
        }

        // walk the TREE. There is no dense index to sweep any more, and a
        // sweep of the POOL would visit freed slots that are not nodes.
        bool ok = true;

        visit_depth_first(
            r,
            [&](file_node_id _id, level_type)
            {
                if (!round_trips(_id))
                {
                    ok = false;
                }
            });

        return ok;
    }

    // level
    //   the LEVEL (lambda) of _id: the number of parent links up to the root,
    // which is at level 0. This is the length of the node's ADDRESS, and it is
    // the number visit_depth_first / visit_breadth_first report. O(level).
    level_type
    level(
        file_node_id _id
    ) const
    {
        level_type n = 0;
        file_node_id    c = _id;

        if (c == null_file_node)
        {
            return 0;
        }

        while (c->parent != null_file_node)
        {
            c = c->parent;
            ++n;
        }

        return n;
    }

    // height
    //   the HEIGHT of _id: the longest descent below it, in links. A file has
    // height 0; so does an empty directory. THIS is what the spec calls DEPTH,
    // and the tree had no way to ask for it. O(subtree).
    level_type
    height(
        file_node_id _id
    ) const
    {
        if (_id == null_file_node)
        {
            return 0;
        }

        level_type best = 0;

        for (file_node_id c = _id->first_child;
             c != null_file_node;
             c = c->next_sibling)
        {
            level_type h = height(c) + 1;

            if (h > best)
            {
                best = h;
            }
        }

        return best;
    }

    // height
    //   the height of the whole tree.
    level_type
    height() const
    {
        return height(root());
    }

    // parent
    //   the parent of _id, or null_file_node at the root.
    file_node_id
    parent(
        file_node_id _id
    ) const
    {
        return ( (_id == null_file_node) ? null_file_node : _id->parent );
    }

    // is_ancestor true if _ancestor is a STRICT ancestor of _descendant. By
    // the spec this is the PREFIX ORDER: addr(_ancestor) is a proper prefix of
    // addr(_descendant).
    bool
    is_ancestor(
        file_node_id _ancestor,
        file_node_id _descendant
    ) const
    {
        if (_ancestor   == null_file_node ||
            _descendant == null_file_node ||
            _ancestor   == _descendant)
        {
            return false;
        }

        file_node_id c = _descendant->parent;

        while (c != null_file_node)
        {
            if (c == _ancestor)
            {
                return true;
            }

            c = c->parent;
        }

        return false;
    }

    // lca the lowest common ancestor of _a and _b - the MEET of their
    // addresses in the prefix order, the longest common prefix of the two
    // words. In a single-rooted tree the meet ALWAYS exists (at worst the
    // empty word, the root), so this cannot fail for two nodes of one tree.
    file_node_id
    lca(
        file_node_id _a,
        file_node_id _b
    ) const
    {
        if (_a == null_file_node || _b == null_file_node)
        {
            return null_file_node;
        }

        level_type la = level(_a);
        level_type lb = level(_b);

        file_node_id ca = _a;
        file_node_id cb = _b;

        // equalise levels: a common prefix is no longer than the shorter word
        while (la > lb) { ca = ca->parent; --la; }
        while (lb > la) { cb = cb->parent; --lb; }

        while (ca != cb)
        {
            if (ca == null_file_node || cb == null_file_node)
            {
                return null_file_node;
            }

            ca = ca->parent;
            cb = cb->parent;
        }

        return ca;
    }


    // --------------------------------------------------------
    //  4.  name access
    // --------------------------------------------------------

    // name
    //   the pooled name pointer for _id. Valid until the next mutation.
    // Optionally writes the length to _length.
    const char*
    name(
        file_node_id      _id,
        std::size_t* _length = nullptr
    ) const
    {
        const file_entry& e = _id->data;

        if (_length != nullptr)
        {
            *_length = e.name_length;
        }

        return m_names.data() + e.name_offset;
    }

    // name_str
    //   the name of _id as a std::string.
    std::string
    name_str(
        file_node_id _id
    ) const
    {
        const file_entry& e = _id->data;

        return m_names.substr(e.name_offset, e.name_length);
    }

    // name_pool
    //   the backing string pool. file_tree_path.hpp binds its component policy
    // to this; it had no way to reach it before.
    const std::string&
    name_pool() const
    {
        return m_names;
    }


    // --------------------------------------------------------
    //  5.  element access + traversal
    // --------------------------------------------------------

    // operator[]
    //   dereferences the handle. Kept so file_tree_path.hpp and
    // file_tree_filter.hpp read a node the same way they always did; a file_node_id
    // is simply a pointer now, so this is the indirection the arena used to
    // do.
    node_type&       operator[](file_node_id _id)       { return *_id; }
    const node_type& operator[](file_node_id _id) const { return *_id; }

    std::size_t size()  const { return m_size;              }
    bool        empty() const { return (m_root == nullptr); }

    // pool
    //   the backing store. Replaces nodes(): there is no indexable arena to
    // hand out any more, and a caller who wants to walk every node should walk
    // the TREE (visit_depth_first) rather than the storage -- the pool may
    // hold freed slots that are not nodes.
    const pool_type& pool() const { return m_pool; }
    pool_type&       pool()       { return m_pool; }

    // visit_depth_first
    //   invokes _fn(file_node_id, level) in pre-order.
    //
    //   The second argument is the LEVEL - the distance DOWN FROM THE ROOT,
    // and the length of the node's address. It is not the depth; see height().
    template<typename Fn>
    void
    visit_depth_first(
        file_node_id _root,
        Fn      _fn
    ) const
    {
        if (_root == null_file_node || empty())
        {
            return;
        }

        struct frame { file_node_id id; level_type level; };

        std::vector<frame> stack;
        stack.push_back({ _root, 0 });

        while (!stack.empty())
        {
            frame f = stack.back();
            stack.pop_back();

            _fn(f.id, f.level);

            // push right-to-left so the leftmost child is popped first
            file_node_id c = f.id->last_child;

            while (c != null_file_node)
            {
                stack.push_back({ c, f.level + 1 });
                c = c->prev_sibling;
            }
        }

        return;
    }

    // visit_breadth_first
    //   invokes _fn(file_node_id, level) in level order - the strata L_0, L_1, ...
    template<typename Fn>
    void
    visit_breadth_first(
        file_node_id _root,
        Fn      _fn
    ) const
    {
        if (_root == null_file_node || empty())
        {
            return;
        }

        struct frame { file_node_id id; level_type level; };

        std::vector<frame> queue;
        queue.push_back({ _root, 0 });

        std::size_t scan_head = 0;

        while (scan_head < queue.size())
        {
            frame f = queue[scan_head];
            ++scan_head;

            _fn(f.id, f.level);

            file_node_id c = f.id->first_child;

            while (c != null_file_node)
            {
                queue.push_back({ c, f.level + 1 });
                c = c->next_sibling;
            }
        }

        return;
    }


    // --------------------------------------------------------
    //  6.  FILTERABILITY
    // --------------------------------------------------------
    //   READ THIS BEFORE USING IT.  Dropping a DIRECTORY severs every descent
    // through it, and the addresses of everything beneath it cease to resolve.
    // A selection preserves the survivors' addresses IF AND ONLY IF the
    // retained set is PREFIX-CLOSED (ancestor-closed) -- and "*.cpp" is not
    // ancestor closed, because no directory is named *.cpp. Keeping only the
    // nodes that match would hand you a tree with no directories and no paths.
    // So the naive reading is not offered. Two are: filter(p) const. A NEW TREE
    // holding the ANCESTOR-CLOSURE of the matching set: the matches, AND THE
    // DIRECTORIES THAT LEAD TO THEM. Prefix-closed by construction, so every
    // survivor keeps its address and full_path() still names it on disk. This
    // is `tree -P '*.cpp'`. select(p) a flat gather of the matching ids. Builds
    // no tree, so it raises no structural question. This is `find -name`, and
    // it is what file_tree_query (file_tree_filter.hpp) does at greater length.
    // Both are wanted. They are not the same operation, and the difference is
    // not a matter of taste -- one of them preserves addressability and the
    // other does not build a tree to preserve it in. TWO PREDICATE SHAPES. A
    // file_entry carries a name OFFSET, not a name, so a predicate over a bare
    // payload can test type and size but CANNOT SEE THE NAME. The useful shape
    // is therefore bool(file_node_id) -- it takes the tree along. Both are accepted,
    // disjointly: bool(file_node_id) the useful one; sees names, paths, levels
    // bool(const file_entry&) the payload one -- and the shape the framework's
    // native_filter_helper probes for, which is what makes this container's
    // filter strategy NATIVE

    // filter (node-predicate form)
    //   const: a NEW TREE holding the ANCESTOR-CLOSURE of {n : _pred(n)} -- a
    // node is kept if it, or anything beneath it, matches. This tree is not
    // touched; the result carries the same root_path, so its full_path() still
    // names files on disk.
    //
    //   PREFIX-CLOSED, and therefore ADDRESSABILITY-PRESERVING: if a node is
    // kept then so is every one of its ancestors -- an ancestor of a match is
    // itself an ancestor of a match -- so every survivor's path is intact in
    // the result and its address still resolves there. When nothing matches,
    // the closure is empty and so is the result.
    template<typename Pred,
             typename std::enable_if<
                 is_invocable_with<Pred, file_node_id>::value,
                 int>::type = 0>
    file_tree_core
    filter(
        Pred _pred
    ) const
    {
        file_tree_core out;

        file_node_id r = root();

        if (r == null_file_node)
        {
            return out;
        }

        // 1. mark the closure bottom-up. Whether a node survives is a question
        // about its DESCENDANTS, so they must be settled first. The arena has
        // no free list, so we decide BEFORE building rather than building and
        // undoing.
        //     A file_node_id is a POINTER now, so it indexes nothing: the keep-set
        //     that used to be a vector sized by arena.size() is a hash set.
        keep_set keep;

        mark_closure(r, _pred, keep);

        if (keep.find(r) == keep.end())
        {
            return out;   // nothing matched: the closure is empty
        }

        // 2. build the survivors, root-first, so every parent exists before
        // its children are interned under it.
        out.m_root_path = m_root_path;

        const file_entry& re = r->data;

        file_entry root_entry = out.pool_entry(
            m_names.data() + re.name_offset,
            re.name_length, re.type, re.size);

        out.m_root = out.new_node(root_entry);

        file_node_id nr = out.m_root;

        copy_kept(r, nr, out, keep);

        return out;
    }

    // filter (payload-predicate form)
    //   the shape has_native_filter / has_filter_method probe for: a CONST
    // member taking bool(const value_type&) and yielding a RESULT. Satisfying
    // it is what makes this container's filter strategy NATIVE.
    //
    //   Note what it can and cannot ask: a file_entry knows its type and size,
    // and does NOT know its name. For anything that reads a name, use the
    // bool(file_node_id) form above.
    template<typename Pred,
             typename std::enable_if<
                 !is_invocable_with<Pred, file_node_id>::value &&
                 is_invocable_with<Pred, const file_entry&>::value,
                 int>::type = 0>
    file_tree_core
    filter(
        Pred _pred
    ) const
    {
        return filter(
            [&_pred](file_node_id _id) -> bool
            {
                return _pred(_id->data);
            });
    }

    // select
    //   the ids whose node satisfies _pred, in pre-order. A read: no tree is
    // built and no address moves. The container in its is_filter_source
    // reading.
    template<typename Pred,
             typename std::enable_if<
                 is_invocable_with<Pred, file_node_id>::value,
                 int>::type = 0>
    std::vector<file_node_id>
    select(
        Pred _pred
    ) const
    {
        std::vector<file_node_id> out;

        visit_depth_first(
            root(),
            [&](file_node_id _id, level_type)
            {
                if (_pred(_id))
                {
                    out.push_back(_id);
                }
            });

        return out;
    }

    // count_if
    //   how many nodes satisfy _pred. A read.
    template<typename Pred,
             typename std::enable_if<
                 is_invocable_with<Pred, file_node_id>::value,
                 int>::type = 0>
    std::size_t
    count_if(
        Pred _pred
    ) const
    {
        std::size_t n = 0;

        visit_depth_first(
            root(),
            [&](file_node_id _id, level_type)
            {
                if (_pred(_id))
                {
                    ++n;
                }
            });

        return n;
    }


    // --------------------------------------------------------
    //  7.  TRANSFORMABILITY
    // --------------------------------------------------------

    // rename_preserves_separation
    //   true if applying _fn to every name would leave the children of every
    // directory bearing DISTINCT names -- that is, if the rename would NOT
    // cost this tree its addressability.
    //
    //   THIS IS THE ONE TO READ. A map keeps every position and so keeps every
    // path, but only while the labels stay APART. A rename that is not
    // injective ON SIBLINGS collides two entries onto one name -- and here
    // that is not an abstraction being violated, it is TWO FILES BECOMING ONE.
    // The spec records this as the loss of mu_1 under transformation; on a
    // filesystem it is a data-loss bug. Ask BEFORE renaming, not after. O(n *
    // k) with a set per directory.
    template<typename Fn>
    bool
    rename_preserves_separation(
        Fn _fn
    ) const
    {
        file_node_id r = root();

        if (r == null_file_node)
        {
            return true;
        }

        bool ok = true;

        visit_depth_first(
            r,
            [&](file_node_id _id, level_type)
            {
                if (!ok)
                {
                    return;
                }

                // would this directory's children still bear distinct names?
                std::vector<std::string> seen;

                for (file_node_id c = _id->first_child;
                     c != null_file_node;
                     c = c->next_sibling)
                {
                    std::string mapped = _fn(name_str(c));

                    for (std::size_t i = 0; i < seen.size(); ++i)
                    {
                        if (seen[i] == mapped)
                        {
                            ok = false;

                            return;
                        }
                    }

                    seen.push_back(mapped);
                }
            });

        return ok;
    }

    // rename
    //   const: a NEW TREE of the SAME SHAPE, every name rewritten to
    // _fn(name). Every position is kept, so every path carries over --
    // PROVIDED the rename separates. Ask rename_preserves_separation() first;
    // where it says no, two
    // siblings collide and the second is dropped rather than silently
    // shadowing the first (see intern_child), so the result is smaller than
    // the source and says so through size().
    template<typename Fn,
             typename std::enable_if<
                 is_invocable_with<Fn, const std::string&>::value,
                 int>::type = 0>
    file_tree_core
    rename(
        Fn _fn
    ) const
    {
        file_tree_core out;

        file_node_id r = root();

        if (r == null_file_node)
        {
            return out;
        }

        out.m_root_path = m_root_path;

        const file_entry& re = r->data;

        std::string  rn = _fn(name_str(r));
        file_entry   root_entry =
            out.pool_entry(rn.c_str(), rn.size(), re.type, re.size);

        out.m_root = out.new_node(root_entry);

        file_node_id nr = out.m_root;

        copy_renamed(r, nr, out, _fn);

        return out;
    }

    // transform
    //   const: a NEW TREE of the SAME SHAPE, every PAYLOAD rewritten to
    // _fn(entry). The names are the tree's labels and are NOT touched here --
    // that is rename()'s job, and the reason it is a separate operation is
    // that renaming can break separation while re-typing and re-sizing cannot.
    //
    //   This is the shape has_native_transform probes for.
    template<typename Fn,
             typename std::enable_if<
                 is_invocable_with<Fn, const file_entry&>::value,
                 int>::type = 0>
    file_tree_core
    transform(
        Fn _fn
    ) const
    {
        file_tree_core out;

        file_node_id r = root();

        if (r == null_file_node)
        {
            return out;
        }

        out.m_root_path = m_root_path;

        const file_entry& re = r->data;

        file_entry mapped = _fn(re);

        file_entry root_entry = out.pool_entry(
            m_names.data() + re.name_offset, re.name_length,
            mapped.type, mapped.size);

        out.m_root = out.new_node(root_entry);

        file_node_id nr = out.m_root;

        copy_transformed(r, nr, out, _fn);

        return out;
    }


    // --------------------------------------------------------
    //  8.  mutation
    // --------------------------------------------------------

    // clear
    //   resets the tree to an empty state.
    void
    clear()
    {
        // file_node is trivially destructible, so releasing the slots is
        // enough; release() returns every block to the source.
        m_pool.release();
        m_root = nullptr;
        m_size = 0;
        m_names.clear();
        m_lookup.clear();
        m_root_path.clear();

        return;
    }

    // add_child
    //   inserts a child under _parent and returns its id. SEPARATION IS AN
    // INVARIANT, NOT A HOPE. If _parent already holds a child of this name,
    // THE EXISTING ID IS RETURNED and no second node is made --
    // a
    // directory cannot hold two entries of one name, and the old code, which
    // wrote `m_lookup[key] = id`, did not so much reject the duplicate as
    // create a node that every traversal walked and no address could reach.
    // Use contains_child() first if you need to tell "created" from "already
    // there".
    file_node_id
    add_child(
        file_node_id       _parent,
        const char*      _name,
        file_type        _type,
        re_std::uint64_t _size = 0
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
    //   interns _name, allocates a node, links it under _parent, and registers
    // the lookup. The single mutation primitive shared by add_child and every
    // OS scanner.
    //
    //   The separation check costs ONE index probe, which is the same probe
    // the registration would have done anyway. In exchange, the tree cannot
    // hold an unaddressable node. That trade is not close.
    file_node_id
    intern_child(
        file_node_id       _parent,
        const char*      _name,
        std::size_t      _len,
        file_type        _type,
        re_std::uint64_t _size
    )
    {
        if (_parent == null_file_node)
        {
            return null_file_node;
        }

        // SEPARATION. A second child of this name would be unreachable by
        // address; it is not created.
        file_node_id existing = find_child(_parent, _name, _len);

        if (existing != null_file_node)
        {
            return existing;
        }

        file_entry entry = pool_entry(_name, _len, _type, _size);

        file_node_id id = new_node(entry);

        link_child(_parent, id);
        m_lookup.emplace(lookup_key(_parent, _name, _len), id);

        return id;
    }

    // --------------------------------------------------------
    //  node creation  (was arena.allocate + arena.append_child)
    // --------------------------------------------------------

    // new_node
    //   acquires a slot from the pool, constructs a file_node in it, returns
    // the handle. The one place a node comes into being.
    file_node_id
    new_node(
        const file_entry& _entry
    )
    {
        void* slot = m_pool.acquire();

        if (slot == nullptr)
        {
            throw std::bad_alloc();
        }

        file_node_id n = ::new (slot) file_node();

        n->data = _entry;

        ++m_size;

        return n;
    }

    // link_child
    //   appends _child to _parent's sibling list. Was arena.append_child.
    void
    link_child(
        file_node_id _parent,
        file_node_id _child
    )
    {
        _child->parent = _parent;

        if (_parent->first_child == nullptr)
        {
            _parent->first_child = _child;
            _parent->last_child  = _child;
        }
        else
        {
            file_node_id last = _parent->last_child;

            last->next_sibling   = _child;
            _child->prev_sibling = last;
            _parent->last_child  = _child;
        }

        return;
    }

    // --------------------------------------------------------
    //  string pool
    // --------------------------------------------------------

    // pool_entry
    //   appends _name to the pool and returns a file_entry referencing it. The
    // one place a name enters the pool.
    file_entry
    pool_entry(
        const char*      _name,
        std::size_t      _len,
        file_type        _type,
        re_std::uint64_t _size
    )
    {
        re_std::uint32_t offset =
            static_cast<re_std::uint32_t>(m_names.size());

        m_names.append(_name, _len);

        return file_entry(
            offset, static_cast<re_std::uint16_t>(_len), _type, _size);
    }

    // make_entry
    //   creates a file_entry for the LEAF component of _name (the scan root's
    // own name: "project" out of "/home/me/project") and pools that leaf. The
    // rest of the scanned path is not a label -- it is where the tree begins
    // -- and is kept in m_root_path.
    file_entry
    make_entry(
        const char*      _name,
        file_type        _type,
        re_std::uint64_t _size
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

        return pool_entry(leaf, len, _type, _size);
    }

    // --------------------------------------------------------
    //  addressing helpers
    // --------------------------------------------------------

    // collect_labels
    //   fills _out with the chain from _id UP TO BUT NOT INCLUDING the root,
    // so it holds exactly level(_id) entries -- the components of the ADDRESS,
    // leaf first. Empty for the root, whose address is the empty word. THE
    // ROOT IS EXCLUDED. Including it is the bug this file carried: it made the
    // word one component longer than resolve() reads, so nothing round
    // tripped.
    void
    collect_labels(
        file_node_id               _id,
        std::vector<file_node_id>& _out
    ) const
    {
        file_node_id r = root();
        file_node_id c = _id;

        while (c != null_file_node && c != r)
        {
            _out.push_back(c);

            c = c->parent;
        }

        // _id was not in this tree: it has no address here
        if (c == null_file_node)
        {
            _out.clear();
        }

        return;
    }

    // --------------------------------------------------------
    //  filter / rename helpers
    // --------------------------------------------------------

    // mark_closure sets _keep[n] for every n in the ANCESTOR-CLOSURE of {n :
    // _pred(n)} over _id's subtree, and returns whether _id itself is in it.
    // Bottom-up, because whether a node survives is a question about its
    // descendants.
    template<typename Pred>
    bool
    mark_closure(
        file_node_id   _id,
        Pred&    _pred,
        keep_set& _keep
    ) const
    {
        bool any = false;

        for (file_node_id c = _id->first_child;
             c != null_file_node;
             c = c->next_sibling)
        {
            if (mark_closure(c, _pred, _keep))
            {
                any = true;
            }
        }

        bool k = ( any || _pred(_id) );

        if (k)
        {
            _keep.insert(_id);
        }

        return k;
    }

    // copy_kept
    //   interns every kept child of _src under _dst in _out, and recurses.
    // Root-first, so a parent always exists before its children.
    void
    copy_kept(
        file_node_id         _src,
        file_node_id         _dst,
        file_tree_core& _out,
        const keep_set& _keep
    ) const
    {
        for (file_node_id c = _src->first_child;
             c != null_file_node;
             c = c->next_sibling)
        {
            if (_keep.find(c) == _keep.end())
            {
                continue;
            }

            const file_entry& e = c->data;

            file_node_id d = _out.intern_child(
                _dst,
                m_names.data() + e.name_offset,
                e.name_length,
                e.type,
                e.size);

            copy_kept(c, d, _out, _keep);
        }

        return;
    }

    // copy_renamed
    //   clones _src's children into _out under _dst, mapping every name
    // through _fn. Where _fn collides two siblings, intern_child returns the
    // first and the second is not created -- the collision is a separation
    // failure and is not represented.
    template<typename Fn>
    void
    copy_renamed(
        file_node_id         _src,
        file_node_id         _dst,
        file_tree_core& _out,
        Fn&            _fn
    ) const
    {
        for (file_node_id c = _src->first_child;
             c != null_file_node;
             c = c->next_sibling)
        {
            const file_entry& e = c->data;

            std::string rn = _fn(name_str(c));

            file_node_id d = _out.intern_child(
                _dst, rn.c_str(), rn.size(), e.type, e.size);

            copy_renamed(c, d, _out, _fn);
        }

        return;
    }

    // copy_transformed
    //   clones _src's children into _out under _dst, mapping every PAYLOAD
    // through _fn while keeping the names, and hence the shape and the
    // addresses.
    template<typename Fn>
    void
    copy_transformed(
        file_node_id         _src,
        file_node_id         _dst,
        file_tree_core& _out,
        Fn&            _fn
    ) const
    {
        for (file_node_id c = _src->first_child;
             c != null_file_node;
             c = c->next_sibling)
        {
            const file_entry& e = c->data;

            file_entry mapped = _fn(e);

            file_node_id d = _out.intern_child(
                _dst,
                m_names.data() + e.name_offset,
                e.name_length,
                mapped.type,
                mapped.size);

            copy_transformed(c, d, _out, _fn);
        }

        return;
    }

    // --------------------------------------------------------
    //  hash index
    // --------------------------------------------------------

    // lookup_key
    //   FNV-1a over the name, seeded with the parent index.
    D_STATIC_INLINE
    re_std::uint64_t
    lookup_key(
        file_node_id     _parent,
        const char* _name,
        std::size_t _len
    )
    {
        // the parent used to be a dense index; it is a pointer now, so the
        // seed is its address.
        re_std::uint64_t h = static_cast<re_std::uint64_t>(
            reinterpret_cast<re_std::uintptr_t>(_parent));

        h ^= UINT64_C(0xcbf29ce484222325);
        h *= UINT64_C(0x100000001b3);

        for (std::size_t i = 0; i < _len; ++i)
        {
            h ^= static_cast<re_std::uint64_t>(
                static_cast<unsigned char>(_name[i]));
            h *= UINT64_C(0x100000001b3);
        }

        return h;
    }


    // --------------------------------------------------------
    //  members
    // --------------------------------------------------------

    pool_type                                  m_pool;
    file_node_id                                    m_root;
    std::size_t                                m_size;
    std::string                                m_names;
    std::unordered_map<re_std::uint64_t, file_node_id> m_lookup;
    std::string                                m_root_path;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_COMMON_HPP
