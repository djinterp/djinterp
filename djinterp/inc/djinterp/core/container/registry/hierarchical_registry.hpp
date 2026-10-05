/*******************************************************************************
* djinterp [core]                                      hierarchical_registry.hpp
*
*   hierarchical_registry -- a registry that NESTS (the Structure axis).  The
* Structure section factors a container's component type as the composite
*
*       T  ~=  tau  +  F[T]        (a LEAF summand + a NODE summand),
*
* the node summand present exactly where nesting occurs; the depth d counts
* nodes
* and a container is HIERARCHICAL when d >= 2. Here each entry maps a KEY to a
* CELL that is either a LEAF (a value : tau) or a NODE (a child registry, the
* F[T] summand) -- the composite pattern, so navigation treats a value and a
* sub-registry alike.  A real registry (HKEY\..., a config tree, a filesystem)
* is exactly this: keys naming values or further keyed sub-trees.
*
*   FLAT IS THE DEGENERATE CASE. A hierarchical_registry all of whose cells
* are
* leaves has depth 1 -- the single element-level L_1 -- and behaves as a flat
* keyed map.  Depth grows only where a cell is a node; the general (composite)
* depth is a property of VALUES, a runtime quantity (depth()), not fixed by
* the
* type. The TYPE is hierarchical all the same (it carries a node summand),
* which
* is what the structure_category tag and the node_type alias below declare to
* the
* structure traits.
*
*   ADDRESSING.  Two surfaces, sharing the composite:
*     LOCAL -- one node's own entries: local_contains / local_leaf /
*   local_node
*                / put_leaf / make_node / local_erase, plus indexed traversal.
*     PATH    -- a sequence of keys walking root -> leaf:  set (creating
*                intermediate nodes, mkdir -p), get / at (read a leaf),
*              node_at
*                (reach a sub-tree), contains / is_leaf / is_node, erase. A
*              path
*                is any forward range of keys (an initializer_list, or the
*                project path<> type via its iterators).
*
*   MUTABILITY. Keys are const (a re-key is erase-then-insert); leaf values
* are
* writable in place (value-mutable, key-const), matching the flat registry.  A
* cell's KIND is fixed once set: put_leaf refuses to overwrite a node,
* make_node
* refuses to shadow a leaf -- convert only through an explicit erase, so a
* subtree
* is never dropped silently.
*
*   VALUE SEMANTICS.  Nodes are held by unique_ptr for the recursion, but the
* registry is a VALUE: copy deep-clones the tree, move transfers it.
*
*   REUSE / SCOPE. A node keeps its entries in sorted key order and looks them
* up
* by binary search -- the same ordered discipline as the flat registry's
* kernel
* (kept local here because the cells are move-only).  Per-node multiplicity is
* unique (a key names one cell); a multi-valued leaf is modelled by a leaf
* whose
* value is itself a container.
*
*   PORTABILITY:
*   C++17 (std::less<> transparent comparator; the options-style axis
* markers).
*
*
* path:      /inc/djinterp/core/container/registry/hierarchical_registry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_hierarchical_registry (detection trait)
      ------------------------------------------

II.   hierarchical_registry (class)
      -----------------------------
      1.    member types, cell kind, overlay / axis markers
      2.    construction (default / deep-copy / move / swap)
      3.    local surface (one node: query / value / structural)
      4.    path surface (descend a key sequence; set / get / node_at / erase)
      5.    shape (depth / leaf_count) and local traversal

III.  hkey_registry (kv-leaf convenience alias)
      -----------------------------------------
*/

#ifndef DJINTERP_CONTAINER_REGISTRY_HIERARCHICAL_REGISTRY_HPP
#define DJINTERP_CONTAINER_REGISTRY_HIERARCHICAL_REGISTRY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>         // std::lower_bound
#include <cstddef>
#include <functional>        // std::less
#include <initializer_list>
#include <memory>            // std::unique_ptr
#include <stdexcept>
#include <type_traits>
#include <utility>           // std::move, std::swap
#include <vector>
// djinterp
#include "../../../djinterp.hpp"                 // NS_*, D_NODISCARD, clean_t
#include "../container_options.hpp"           // axis enums
#include "../../meta/hierarchical.hpp"      // hierarchical structure tag


NS_DJINTERP


// ===========================================================================
// I.   is_hierarchical_registry (detection trait)
// ===========================================================================

// hierarchical_registry (fwd)
template<typename Key,
         typename Value,
         typename KeyCompare,
         typename SizeType>
class hierarchical_registry;

NS_INTERNAL

    template<typename Type>
    struct is_hierarchical_registry_impl : std::false_type
    {};

    template<typename K, typename V, typename KC, typename S>
    struct is_hierarchical_registry_impl<hierarchical_registry<K, V, KC, S>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_hierarchical_registry
    : internal::is_hierarchical_registry_impl<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
inline constexpr bool is_hierarchical_registry_v =
    is_hierarchical_registry<Type>::value;
#endif


// ===========================================================================
// II.  hierarchical_registry (class)
// ===========================================================================

// hierarchical_registry
//   class: a keyed, sorted node whose cells are leaves (value : Value) or
// nodes (a child hierarchical_registry). One class is every level of the tree.
template<typename Key,
         typename Value,
         typename KeyCompare = std::less<>,
         typename SizeType    = std::size_t>
class hierarchical_registry
{
public:
    // --- 1. member types, cell kind, markers ---

    using key_type    = Key;
    using mapped_type = Value;      // a LEAF value
    using size_type   = SizeType;
    using key_compare = KeyCompare;

    // node_type -- the F[T] node summand: a sub-registry is one of THESE. This
    // container-shaped alias is the STRONG structural tell the hierarchy
    // traits read (hierarchical_container_traits.hpp).
    using node_type = hierarchical_registry;

    // structure_category -- the OPT-IN structure tag; its `nests` bit (true)
    // asserts hierarchy the value_type chain cannot show.
    using structure_category = hierarchical;

    // cell_kind -- a cell is a leaf or a node (the composite's two summands).
    enum class cell_kind
    {
        leaf,
        node
    };

    // npos -- "no such entry" sentinel.
    static constexpr size_type npos = static_cast<size_type>(-1);

    // overlay / axis markers.
    static constexpr bool keyed            = true;                 // eta (per node)
    static constexpr bool sorted_invariant = true;                 // varsigma
    static constexpr bool key_const        = true;                 // keys immutable
    static constexpr bool value_mutable    = true;                 // leaves writable
    static constexpr bool nests            = true;                 // node summand present
    static constexpr std::size_t min_depth = hierarchical::min_depth;  // 2

    static constexpr container_structure     structure     =
        container_structure::hierarchical;
    static constexpr container_ordering      ordering      =
        container_ordering::sorted;
    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::dynamic_storage;
    static constexpr container_bounds        bounds        =
        container_bounds::unbounded;
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        container_multiplicity::unique;   // a key names ONE cell per node

private:
    // entry -- one keyed cell: a leaf value OR a child node (held by pointer
    // for the recursion). Move-only (the unique_ptr); the registry deep-copies
    // at the class level.
    struct entry
    {
        Key                                    m_key;
        cell_kind                              m_kind;
        Value                                  m_leaf;    // valid when m_kind == leaf
        std::unique_ptr<hierarchical_registry> m_child;   // valid when m_kind == node

        entry()
            : m_key(), m_kind(cell_kind::leaf), m_leaf(), m_child()
        {}
    };

    using store = std::vector<entry>;

    store       m_entries;   // sorted by m_key
    KeyCompare m_cmp;

    // key equivalence induced by the strict-weak comparator.
    D_NODISCARD bool eq(const Key& _a, const Key& _b) const
    {
        return (!m_cmp(_a, _b)) && (!m_cmp(_b, _a));
    }

    // lower_bound over entries by key (const / mutable).
    typename store::const_iterator lb(const Key& _k) const
    {
        return std::lower_bound(
            m_entries.begin(), m_entries.end(), _k,
            [this](const entry& _e, const Key& _key)
            { return m_cmp(_e.m_key, _key); });
    }

    typename store::iterator lb(const Key& _k)
    {
        return std::lower_bound(
            m_entries.begin(), m_entries.end(), _k,
            [this](const entry& _e, const Key& _key)
            { return m_cmp(_e.m_key, _key); });
    }

    // find_entry -- pointer to the entry keyed _k, or nullptr.
    const entry* find_entry(const Key& _k) const
    {
        auto it = lb(_k);
        return (it != m_entries.end() && eq(it->m_key, _k)) ? &*it : nullptr;
    }

    entry* find_entry(const Key& _k)
    {
        auto it = lb(_k);
        return (it != m_entries.end() && eq(it->m_key, _k)) ? &*it : nullptr;
    }

public:
    // --- 2. construction ---

    hierarchical_registry()
        : m_entries(), m_cmp()
    {}

    explicit hierarchical_registry(KeyCompare _cmp)
        : m_entries(), m_cmp(_cmp)
    {}

    // deep copy -- clone the whole tree (nodes are cloned recursively).
    hierarchical_registry(const hierarchical_registry& _other)
        : m_entries(), m_cmp(_other.m_cmp)
    {
        m_entries.reserve(_other.m_entries.size());

        for (const entry& e : _other.m_entries)
        {
            entry ne;
            ne.m_key  = e.m_key;
            ne.m_kind = e.m_kind;

            if (e.m_kind == cell_kind::leaf)
            {
                ne.m_leaf = e.m_leaf;
            }
            else if (e.m_child)
            {
                ne.m_child.reset(new hierarchical_registry(*e.m_child));
            }

            m_entries.push_back(std::move(ne));
        }
    }

    hierarchical_registry(hierarchical_registry&&) = default;

    // copy-and-swap assignment (serves copy AND move: _other is built by the
    // deep-copy or the move constructor, then swapped in).
    hierarchical_registry& operator=(hierarchical_registry _other) noexcept
    {
        swap(_other);
        return *this;
    }

    ~hierarchical_registry() = default;

    void swap(hierarchical_registry& _other) noexcept
    {
        m_entries.swap(_other.m_entries);
        std::swap(m_cmp, _other.m_cmp);
    }

    // --- 3. local surface (this node only) ---

    D_NODISCARD size_type local_size() const noexcept
    {
        return static_cast<size_type>(m_entries.size());
    }

    D_NODISCARD bool local_empty() const noexcept
    {
        return m_entries.empty();
    }

    D_NODISCARD bool local_contains(const Key& _k) const
    {
        return (find_entry(_k) != nullptr);
    }

    D_NODISCARD bool local_is_leaf(const Key& _k) const
    {
        const entry* e = find_entry(_k);
        return (e != nullptr) && (e->m_kind == cell_kind::leaf);
    }

    D_NODISCARD bool local_is_node(const Key& _k) const
    {
        const entry* e = find_entry(_k);
        return (e != nullptr) && (e->m_kind == cell_kind::node);
    }

    // local_leaf -- pointer to the leaf value at _k, or nullptr (absent, or a
    // node). const and mutable (the value is writable in place).
    D_NODISCARD const Value* local_leaf(const Key& _k) const
    {
        const entry* e = find_entry(_k);
        return (e != nullptr && e->m_kind == cell_kind::leaf) ? &e->m_leaf
                                                              : nullptr;
    }

    D_NODISCARD Value* local_leaf(const Key& _k)
    {
        entry* e = find_entry(_k);
        return (e != nullptr && e->m_kind == cell_kind::leaf) ? &e->m_leaf
                                                              : nullptr;
    }

    // local_node -- pointer to the child registry at _k, or nullptr (absent,
    // or a leaf). const and mutable.
    D_NODISCARD const hierarchical_registry* local_node(const Key& _k) const
    {
        const entry* e = find_entry(_k);
        return (e != nullptr && e->m_kind == cell_kind::node) ? e->m_child.get()
                                                              : nullptr;
    }

    D_NODISCARD hierarchical_registry* local_node(const Key& _k)
    {
        entry* e = find_entry(_k);
        return (e != nullptr && e->m_kind == cell_kind::node) ? e->m_child.get()
                                                              : nullptr;
    }

    // put_leaf -- set the leaf value at _k in THIS node (create or overwrite a
    // leaf). Returns false, changing nothing, if _k already holds a NODE (a
    // subtree is never dropped implicitly -- erase it first).
    bool put_leaf(
        const Key& _k,
        Value       _v
    )
    {
        auto it = lb(_k);

        if (it != m_entries.end() && eq(it->m_key, _k))
        {
            if (it->m_kind == cell_kind::node)
            {
                return false;   // refuse to clobber a subtree
            }

            it->m_leaf = static_cast<Value&&>(_v);
            return true;
        }

        entry ne;
        ne.m_key  = _k;
        ne.m_kind = cell_kind::leaf;
        ne.m_leaf = static_cast<Value&&>(_v);

        m_entries.insert(it, std::move(ne));
        return true;
    }

    // make_node -- ensure a child node at _k and return it. Returns the
    // existing child if _k is already a node; throws std::logic_error if _k
    // holds a leaf (convert only through an explicit erase).
    hierarchical_registry& make_node(const Key& _k)
    {
        auto it = lb(_k);

        if (it != m_entries.end() && eq(it->m_key, _k))
        {
            if (it->m_kind == cell_kind::node)
            {
                return *it->m_child;
            }

            throw std::logic_error(
                "hierarchical_registry::make_node: key already holds a leaf.");
        }

        entry ne;
        ne.m_key  = _k;
        ne.m_kind = cell_kind::node;
        ne.m_child.reset(new hierarchical_registry(m_cmp));

        auto ins = m_entries.insert(it, std::move(ne));
        return *ins->m_child;
    }

    // local_erase -- remove the cell at _k (a leaf, or a whole subtree);
    // returns the number removed (0 or 1).
    size_type local_erase(const Key& _k)
    {
        auto it = lb(_k);

        if (it != m_entries.end() && eq(it->m_key, _k))
        {
            m_entries.erase(it);
            return size_type(1);
        }

        return size_type(0);
    }

    // --- 4. path surface ---
    //   A path is a forward range of keys. All ops resolve the path against
    // the composite; the initializer_list overloads simply forward to the
    // range form.

private:
    // descend_parent (const) -- follow every key but the LAST as a node;
    // yields the node that should CONTAIN the last key, and writes that key to
    // _out. nullptr if the path is empty or an intermediate key is missing / a
    // leaf.
    template<typename It>
    const hierarchical_registry*
    descend_parent(It _first, It _last, Key& _out) const
    {
        if (_first == _last)
        {
            return nullptr;
        }

        const hierarchical_registry* cur = this;
        It it  = _first;
        It nxt = it;
        ++nxt;

        while (nxt != _last)
        {
            const hierarchical_registry* child = cur->local_node(*it);
            if (child == nullptr)
            {
                return nullptr;   // missing, or a leaf blocks the descent
            }
            cur = child;
            it  = nxt;
            ++nxt;
        }

        _out = *it;
        return cur;
    }

    // descend_parent_create (mutable) -- as above but creates intermediate
    // nodes (mkdir -p). nullptr only if the path is empty or an intermediate
    // key is a LEAF (which cannot become a node implicitly).
    template<typename It>
    hierarchical_registry*
    descend_parent_create(It _first, It _last, Key& _out)
    {
        if (_first == _last)
        {
            return nullptr;
        }

        hierarchical_registry* cur = this;
        It it  = _first;
        It nxt = it;
        ++nxt;

        while (nxt != _last)
        {
            hierarchical_registry* child = cur->local_node(*it);
            if (child == nullptr)
            {
                if (cur->local_contains(*it))
                {
                    return nullptr;   // a leaf blocks the descent
                }
                child = &cur->make_node(*it);
            }
            cur = child;
            it  = nxt;
            ++nxt;
        }

        _out = *it;
        return cur;
    }

public:
    // set (range) -- write a leaf value at the path, creating intermediate
    // nodes. Returns false (changing nothing) if the path is empty, an
    // intermediate key is a leaf, or the final key already holds a node.
    template<typename It>
    bool set(It _first, It _last, Value _value)
    {
        Key last_key;
        hierarchical_registry* parent =
            descend_parent_create(_first, _last, last_key);

        if (parent == nullptr)
        {
            return false;
        }

        return parent->put_leaf(last_key, static_cast<Value&&>(_value));
    }

    bool set(std::initializer_list<Key> _path, Value _value)
    {
        return set(_path.begin(), _path.end(), static_cast<Value&&>(_value));
    }

    // get (range) -- pointer to the leaf value at the path, or nullptr (path
    // absent, or the target is a node).
    template<typename It>
    D_NODISCARD const Value* get(It _first, It _last) const
    {
        Key last_key;
        const hierarchical_registry* parent =
            descend_parent(_first, _last, last_key);

        return (parent != nullptr) ? parent->local_leaf(last_key) : nullptr;
    }

    D_NODISCARD const Value* get(std::initializer_list<Key> _path) const
    {
        return get(_path.begin(), _path.end());
    }

    // at (range) -- the leaf value at the path; throws std::out_of_range if
    // the path is absent or the target is not a leaf.
    template<typename It>
    D_NODISCARD const Value& at(It _first, It _last) const
    {
        const Value* p = get(_first, _last);
        if (p == nullptr)
        {
            throw std::out_of_range(
                "hierarchical_registry::at: no leaf at the given path.");
        }
        return *p;
    }

    D_NODISCARD const Value& at(std::initializer_list<Key> _path) const
    {
        return at(_path.begin(), _path.end());
    }

    // node_at (range) -- pointer to the sub-registry at the path (every key
    // followed as a node), or nullptr. const and mutable.
    template<typename It>
    D_NODISCARD const hierarchical_registry* node_at(It _first, It _last) const
    {
        const hierarchical_registry* cur = this;
        for (It it = _first; it != _last; ++it)
        {
            cur = cur->local_node(*it);
            if (cur == nullptr)
            {
                return nullptr;
            }
        }
        return cur;
    }

    D_NODISCARD const hierarchical_registry*
    node_at(std::initializer_list<Key> _path) const
    {
        return node_at(_path.begin(), _path.end());
    }

    template<typename It>
    D_NODISCARD hierarchical_registry* node_at(It _first, It _last)
    {
        hierarchical_registry* cur = this;
        for (It it = _first; it != _last; ++it)
        {
            cur = cur->local_node(*it);
            if (cur == nullptr)
            {
                return nullptr;
            }
        }
        return cur;
    }

    D_NODISCARD hierarchical_registry*
    node_at(std::initializer_list<Key> _path)
    {
        return node_at(_path.begin(), _path.end());
    }

    // contains / is_leaf / is_node (range + list) -- classify the path target.
    template<typename It>
    D_NODISCARD bool contains(It _first, It _last) const
    {
        Key last_key;
        const hierarchical_registry* parent =
            descend_parent(_first, _last, last_key);
        return (parent != nullptr) && parent->local_contains(last_key);
    }

    D_NODISCARD bool contains(std::initializer_list<Key> _path) const
    {
        return contains(_path.begin(), _path.end());
    }

    template<typename It>
    D_NODISCARD bool is_leaf(It _first, It _last) const
    {
        Key last_key;
        const hierarchical_registry* parent =
            descend_parent(_first, _last, last_key);
        return (parent != nullptr) && parent->local_is_leaf(last_key);
    }

    D_NODISCARD bool is_leaf(std::initializer_list<Key> _path) const
    {
        return is_leaf(_path.begin(), _path.end());
    }

    template<typename It>
    D_NODISCARD bool is_node(It _first, It _last) const
    {
        Key last_key;
        const hierarchical_registry* parent =
            descend_parent(_first, _last, last_key);
        return (parent != nullptr) && parent->local_is_node(last_key);
    }

    D_NODISCARD bool is_node(std::initializer_list<Key> _path) const
    {
        return is_node(_path.begin(), _path.end());
    }

    // erase (range + list) -- remove the cell (leaf or subtree) at the path;
    // returns the number removed (0 or 1).
    template<typename It>
    size_type erase(It _first, It _last)
    {
        Key last_key;
        // descend without creating; the parent must already exist
        hierarchical_registry* cur = this;
        if (_first == _last)
        {
            return size_type(0);
        }
        It it  = _first;
        It nxt = it;
        ++nxt;
        while (nxt != _last)
        {
            cur = cur->local_node(*it);
            if (cur == nullptr)
            {
                return size_type(0);
            }
            it  = nxt;
            ++nxt;
        }
        last_key = *it;

        return cur->local_erase(last_key);
    }

    size_type erase(std::initializer_list<Key> _path)
    {
        return erase(_path.begin(), _path.end());
    }

    // --- 5. shape + local traversal ---

    // depth -- the tree height rooted here (a runtime property of the value):
    // 0 when empty, 1 when every cell is a leaf (the flat case), >= 2 when a
    // cell is a node.
    D_NODISCARD size_type depth() const noexcept
    {
        if (m_entries.empty())
        {
            return size_type(0);
        }

        size_type deepest = 0;
        for (const entry& e : m_entries)
        {
            const size_type cd =
                (e.m_kind == cell_kind::leaf)
                    ? size_type(0)
                    : (e.m_child ? e.m_child->depth() : size_type(0));

            if (cd > deepest)
            {
                deepest = cd;
            }
        }

        return static_cast<size_type>(1 + deepest);
    }

    // leaf_count -- the number of leaves in the whole subtree.
    D_NODISCARD size_type leaf_count() const noexcept
    {
        size_type n = 0;
        for (const entry& e : m_entries)
        {
            n = static_cast<size_type>(
                n + ((e.m_kind == cell_kind::leaf)
                         ? size_type(1)
                         : (e.m_child ? e.m_child->leaf_count() : size_type(0))));
        }
        return n;
    }

    // indexed local traversal (cells at THIS node, in key order) -- entries
    // are private (they hold owning pointers), so key + kind are surfaced by
    // index.
    D_NODISCARD const Key& local_key_at(size_type _i) const
    {
        return m_entries.at(static_cast<std::size_t>(_i)).m_key;
    }

    D_NODISCARD cell_kind local_kind_at(size_type _i) const
    {
        return m_entries.at(static_cast<std::size_t>(_i)).m_kind;
    }
};

// swap (free) -- ADL swap for hierarchical_registry.
template<typename K, typename V, typename KC, typename S>
void swap(
    hierarchical_registry<K, V, KC, S>& _a,
    hierarchical_registry<K, V, KC, S>& _b
) noexcept
{
    _a.swap(_b);
}


// ===========================================================================
// III. hkey_registry
// ===========================================================================

// hkey_registry
//   alias: the common hierarchical registry -- keys of type Key naming leaf
// values of type Value or sub-trees of the same shape. (Named for the
// registry-hive idiom: an HKEY addresses a tree of keyed values.)
template<typename Key,
         typename Value,
         typename KeyCompare = std::less<>>
using hkey_registry = hierarchical_registry<Key, Value, KeyCompare>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_REGISTRY_HIERARCHICAL_REGISTRY_HPP
