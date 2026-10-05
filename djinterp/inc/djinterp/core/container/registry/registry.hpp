/*******************************************************************************
* djinterp [core]                                                   registry.hpp
*
*   registry -- a key-lookup-optimised map/multimap over a contiguous record
* store, the runtime, dynamic generalisation of the value-carrying option_set.
* It wears the `map` overlay (Overlays: containers as restriction bundles):
*
*     keyed eta_{Key,Val} (static) a record projects a key; a "duplicate"
*                                         is a repeated KEY, whatever its
*                                       value.
*     sorted varsigma (sequence-level)    records kept in non-decreasing key
*                                         order, so lookup is a binary search.
*     multiplicity on E_key               mu_1 (unique keys) is the MAP;
*                                         mu_m>1 (keys may repeat) the
*                                       MULTIMAP.
*
*   THE KEY COLUMN IS IMMUTABLE (the Mutability axis's KEY-CONST restriction).
* Rows and value columns may be erased and value cells overwritten in place,
* but
* a stored key is never rewritten -- so varsigma and mu hold BY CONSTRUCTION,
* checked only at insert and never re-validated after a value write. A
* registry
* is therefore structure-mutable at the whole-record grade (no wholesale
* element
* overwrite, the key being frozen) with a key-const value coordinate that
* stays
* writable -- exactly std::map's placement.
*
*   THE TRICHOTOMY.  A record's key and value regions are named by two OPEN
* projections, so one class serves every entry shape:
*     (A) kv_pair rows            -- key = .m_key, value = .m_value (see
*                                    kv_registry / kv_key / kv_value below).
*     (B) projected-key records   -- key at any tuple index / any accessor,
*                                    value a disjoint sub-region
*                                  (tuple_at<J>).
*     (C) whole-record value -- value IS the record (whole_record); here a
*                                    value edit can move the key, so it is
*                                  gated
*                                    behind the guarded update() family.
*   When the value region is disjoint from the key ((A)/(B)), KeyDisjoint is
* true: the raw value-mutable surface (at/operator[]/set) is offered and edits
* skip the re-key check entirely. When it is not ((C)), KeyDisjoint is false:
* those raw setters are withdrawn and every edit routes through update(),
* whose
* rekey strategy (reject / reposition) decides a key move -- both built from
* the
* one common fold in registry_update.hpp.
*
*   REUSE.  The runtime lookup, ordered insert, and E_key equivalence live in
* registry_update.hpp (equal_under, the sorted-key kernel); this class is the
* key-const surface over them plus the options mixin.  Options configure the
* axes (container_opt_*) exactly as for any container.
*
*   PORTABILITY:
*   C++17 (std::invoke; if constexpr in the shared fold; std::less<>
* transparent
* comparator; the options surface).
*
*
* path:      /inc/djinterp/core/container/registry/registry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_registry (detection trait)
      -----------------------------

II.   registry (class)
      ----------------
      1.    member types and overlay / axis markers
      2.    construction
      3.    record access (delegated, const)
      4.    key queries (binary search over the sorted key order)
      5.    value surface (key-const: read always; write when KeyDisjoint)
      6.    structural mutation (insert / erase, invariant-preserving)
      7.    value update (the guarded fold; guarded / relaxed entry points)

III.  kv projections + kv_registry
      ----------------------------

IV.   make_registry
      -------------
*/

#ifndef DJINTERP_CONTAINER_REGISTRY_REGISTRY_HPP
#define DJINTERP_CONTAINER_REGISTRY_REGISTRY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>        // std::less, std::invoke
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"           // NS_*, D_NODISCARD, D_CONSTEXPR, clean_t
#include "./registry_update.hpp"        // update_at, rekey strategies, sorted-key kernel
#include "../container_options.hpp"     // axis enums, options_container_base
#include "../../meta/kv_pair.hpp"       // kv_pair (kv_registry)


NS_DJINTERP


// ===========================================================================
// I.   is_registry (detection trait)
// ===========================================================================

// registry (fwd)
template<typename    Record,
         typename    KeyProj,
         typename    ValueProj,
         typename    KeyCompare,
         bool        UniqueKeys,
         bool        KeyDisjoint,
         typename    RekeyPolicy,
         typename    SizeType,
         typename    DifferenceType,
         typename... Options>
class registry;

// is_registry
//   trait: true when Type (after stripping cv/ref) is a specialization of
// registry.
NS_INTERNAL

    template<typename Type>
    struct is_registry_impl : std::false_type
    {};

    template<typename    R,
             typename    KP,
             typename    VP,
             typename    KC,
             bool        U,
             bool        KD,
             typename    RP,
             typename    S,
             typename    D,
             typename... O>
    struct is_registry_impl<
        registry<R, KP, VP, KC, U, KD, RP, S, D, O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_registry : internal::is_registry_impl<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
inline constexpr bool is_registry_v = is_registry<Type>::value;
#endif


// ===========================================================================
// II.  registry (class)
// ===========================================================================

// registry
//   class: a sorted map (unique keys) or multimap (repeated keys) of Record,
// keyed by KeyProj and ordered by KeyCompare, with an immutable key column.
// Records are stored contiguously in key order; lookup is O(log n); the value
// region (named by ValueProj) is writable in place while the key is frozen.
template<typename    Record,
         typename    KeyProj,
         typename    ValueProj,
         typename    KeyCompare      = std::less<>,
         bool        UniqueKeys      = true,
         bool        KeyDisjoint     = false,
         typename    RekeyPolicy     = reject_rekey,
         typename    SizeType        = std::size_t,
         typename    DifferenceType = std::ptrdiff_t,
         typename... Options>
class registry
    : public options_container_base<Options...>
{
private:
    using storage_type = std::vector<Record>;

public:
    // --- 1. member types and overlay / axis markers ---

    using value_type      = Record;   // a record projecting a key
    using record_type     = Record;
    using size_type       = SizeType;
    using difference_type = DifferenceType;
    using const_iterator  = typename storage_type::const_iterator;
    using key_projection   = KeyProj;
    using value_projection = ValueProj;
    using key_compare      = KeyCompare;
    using rekey_policy      = RekeyPolicy;

    // key_type / mapped_type -- the projected key and value, deduced by
    // applying the projections to a record (std::invoke resolves functors AND
    // pointer-to-member projections).
    using key_type = clean_t<
        decltype(std::invoke(std::declval<const KeyProj&>(),
                             std::declval<const Record&>()))>;

    using mapped_type = clean_t<
        decltype(std::invoke(std::declval<const ValueProj&>(),
                             std::declval<Record&>()))>;

    // npos -- "no such entry" sentinel for the positional key queries.
    static constexpr size_type npos = static_cast<size_type>(-1);

    // overlay markers (the restriction bundle this container wears).
    static constexpr bool keyed            = true;          // eta
    static constexpr bool sorted_invariant = true;          // varsigma
    static constexpr bool unique_keys      = UniqueKeys;   // mu_1 vs mu_m on E_key
    static constexpr bool key_const        = true;          // key column immutable
    static constexpr bool value_disjoint   = KeyDisjoint;  // value region misses the key

    // axis positions.
    static constexpr container_lifetime      lifetime      =
        container_lifetime::mutable_storage;
    static constexpr container_storage_kind  storage_kind  =
        container_storage_kind::dynamic_storage;
    static constexpr container_ordering      ordering      =
        container_ordering::sorted;
    static constexpr container_bounds        bounds        =
        container_bounds::unbounded;
    static constexpr container_iterability   iterability   =
        container_iterability::iterable;
    static constexpr container_multiplicity  multiplicity_grade =
        UniqueKeys ? container_multiplicity::unique
                    : container_multiplicity::multi;
    static constexpr container_structure     structure     =
        container_structure::flat;           // a flat (depth-1) registry of records

    // --- 2. construction ---

    registry()
        : m_records(),
          m_kproj(),
          m_vproj(),
          m_cmp()
    {}

    explicit registry(
        KeyProj     _kproj,
        ValueProj   _vproj,
        KeyCompare _cmp = KeyCompare()
    )
        : m_records(),
          m_kproj(_kproj),
          m_vproj(_vproj),
          m_cmp(_cmp)
    {}

    // record list: each record is inserted at its sorted key position, so any
    // input order yields the keyed, sorted invariant.
    registry(
        std::initializer_list<Record> _records,
        KeyProj                        _kproj = KeyProj(),
        ValueProj                      _vproj = ValueProj(),
        KeyCompare                     _cmp   = KeyCompare()
    )
        : m_records(),
          m_kproj(_kproj),
          m_vproj(_vproj),
          m_cmp(_cmp)
    {
        // insert record by record; each restores eta/varsigma (and mu on E_key)
        for (const Record& rec : _records)
        {
            insert(rec);
        }
    }

    registry(const registry&)            = default;
    registry(registry&&)                 = default;
    registry& operator=(const registry&) = default;
    registry& operator=(registry&&)      = default;
    ~registry()                          = default;

    // --- 3. record access (delegated, const) ---

    D_NODISCARD size_type size() const noexcept
    {
        return static_cast<size_type>(m_records.size());
    }

    D_NODISCARD bool empty() const noexcept
    {
        return m_records.empty();
    }

    // record_at -- the record at a position in key order (checked). Named
    // apart from at() so a size_type-keyed registry has no key/position
    // ambiguity.
    D_NODISCARD const Record& record_at(size_type _i) const
    {
        return m_records.at(static_cast<std::size_t>(_i));
    }

    // key_of -- the key a record projects.
    D_NODISCARD key_type key_of(const Record& _rec) const
    {
        return std::invoke(m_kproj, _rec);
    }

    // record iteration, in key order.
    D_NODISCARD const_iterator begin() const noexcept
    {
        return m_records.begin();
    }

    D_NODISCARD const_iterator end() const noexcept
    {
        return m_records.end();
    }

    D_NODISCARD const_iterator cbegin() const noexcept
    {
        return m_records.cbegin();
    }

    D_NODISCARD const_iterator cend() const noexcept
    {
        return m_records.cend();
    }

    // --- 4. key queries (binary search over the sorted key order) ---

    // index_of -- position of the first record equivalent to _k, or npos.
    D_NODISCARD size_type index_of(const key_type& _k) const
    {
        const std::size_t pos =
            internal::reg_find(m_records, _k, m_kproj, m_cmp);

        return (pos < m_records.size()) ? static_cast<size_type>(pos)
                                        : npos;
    }

    // find -- pointer to the first record whose key is equivalent to _k, or
    // nullptr (records are const: the key must not be rewritten in place).
    D_NODISCARD const Record* find(const key_type& _k) const
    {
        const std::size_t pos =
            internal::reg_find(m_records, _k, m_kproj, m_cmp);

        return (pos < m_records.size())
                   ? &m_records[static_cast<std::size_t>(pos)]
                   : nullptr;
    }

    // contains -- whether any record carries a key equivalent to _k.
    D_NODISCARD bool contains(const key_type& _k) const
    {
        return (find(_k) != nullptr);
    }

    // count -- how many records carry a key equivalent to _k (0 or 1 when keys
    // are unique).
    D_NODISCARD size_type count(const key_type& _k) const
    {
        const std::size_t lo =
            internal::reg_lower_bound(m_records, _k, m_kproj, m_cmp);
        const std::size_t hi =
            internal::reg_upper_bound(m_records, _k, m_kproj, m_cmp);

        return static_cast<size_type>(hi - lo);
    }

    // --- 5. value surface (key-const) ---
    //   Reads are always available. Direct value WRITES (at/operator[]/set)
    // are offered only when KeyDisjoint -- there the value region provably
    // cannot reach the key, so an in-place write is safe. When it is not
    // (whole-record value), these are withdrawn and edits must go through
    // update() (Section 7), whose fold guards the key. For a multimap, all
    // target the FIRST equivalent record.

    // at (value, read) -- always available.
    D_NODISCARD const mapped_type& at(const key_type& _k) const
    {
        const size_type pos = index_of(_k);

        if (pos == npos)
        {
            throw std::out_of_range(
                "registry::at: no entry for the given key.");
        }

        return std::invoke(m_vproj,
                           m_records[static_cast<std::size_t>(pos)]);
    }

    // try_get (value, read) -- pointer to the value, or nullptr; always
    // available.
    D_NODISCARD const mapped_type* try_get(const key_type& _k) const
    {
        const std::size_t pos =
            internal::reg_find(m_records, _k, m_kproj, m_cmp);

        return (pos < m_records.size())
                   ? &std::invoke(m_vproj, m_records[pos])
                   : nullptr;
    }

    // at (value, write) -- models A/B only (value disjoint from key).
    template<bool KD = KeyDisjoint,
             typename std::enable_if<KD, int>::type = 0>
    D_NODISCARD mapped_type& at(const key_type& _k)
    {
        const size_type pos = index_of(_k);

        if (pos == npos)
        {
            throw std::out_of_range(
                "registry::at: no entry for the given key.");
        }

        return std::invoke(m_vproj,
                           m_records[static_cast<std::size_t>(pos)]);
    }

    // operator[] -- writable value at an existing key (models A/B). Unlike
    // std::map it does NOT insert a missing key (there is no record factory in
    // the general core); it throws, as at() does.
    template<bool KD = KeyDisjoint,
             typename std::enable_if<KD, int>::type = 0>
    D_NODISCARD mapped_type& operator[](const key_type& _k)
    {
        return at(_k);
    }

    // try_get (value, write) -- pointer to the writable value, or nullptr
    // (models A/B).
    template<bool KD = KeyDisjoint,
             typename std::enable_if<KD, int>::type = 0>
    D_NODISCARD mapped_type* try_get(const key_type& _k)
    {
        const std::size_t pos =
            internal::reg_find(m_records, _k, m_kproj, m_cmp);

        return (pos < m_records.size())
                   ? &std::invoke(m_vproj, m_records[pos])
                   : nullptr;
    }

    // set -- overwrite the value at an existing key in place (key-const value
    // write); returns whether the key was present (models A/B).
    template<bool KD = KeyDisjoint,
             typename std::enable_if<KD, int>::type = 0>
    bool set(
        const key_type&    _k,
        const mapped_type& _v
    )
    {
        const std::size_t pos =
            internal::reg_find(m_records, _k, m_kproj, m_cmp);

        if (pos >= m_records.size())
        {
            return false;
        }

        std::invoke(m_vproj, m_records[pos]) = _v;

        return true;
    }

    // --- 6. structural mutation (invariant-preserving) ---

    // insert -- place _rec at its sorted key position. Map (unique keys): a
    // record of an equivalent key is replaced (the key stays equivalent, so
    // this is a value overwrite, not a re-key); multimap: added after any
    // equal keys. Returns the position it occupies.
    size_type insert(Record _rec)
    {
        const std::size_t pos = internal::reg_insert(
            m_records, static_cast<Record&&>(_rec),
            m_kproj, m_cmp, UniqueKeys);

        return static_cast<size_type>(pos);
    }

    // insert (key, value) -- convenience for records constructible from a
    // key/value pair (kv_pair and the like); absent otherwise.
    template<typename K = key_type,
             typename V = mapped_type,
             typename std::enable_if<
                 std::is_constructible<Record, K, V>::value, int>::type = 0>
    size_type insert(
        K _k,
        V _v
    )
    {
        return insert(Record(static_cast<K&&>(_k),
                              static_cast<V&&>(_v)));
    }

    // erase -- remove every record whose key is equivalent to _k; returns the
    // number removed (0 or 1 when keys are unique).
    size_type erase(const key_type& _k)
    {
        const std::size_t lo =
            internal::reg_lower_bound(m_records, _k, m_kproj, m_cmp);
        const std::size_t hi =
            internal::reg_upper_bound(m_records, _k, m_kproj, m_cmp);

        // the equivalent keys occupy the contiguous run [lo, hi)
        m_records.erase(m_records.begin() + static_cast<std::ptrdiff_t>(lo),
                        m_records.begin() + static_cast<std::ptrdiff_t>(hi));

        return static_cast<size_type>(hi - lo);
    }

    // erase_at -- remove the record at position _i (in key order).
    void erase_at(size_type _i)
    {
        internal::reg_erase_at(m_records, static_cast<std::size_t>(_i));

        return;
    }

    // clear -- drop all records.
    void clear() noexcept
    {
        m_records.clear();

        return;
    }

    // --- 7. value update (the guarded fold) ---
    //   The universal edit path. _mutate receives a mutable reference to the
    // value region; on return the fold (registry_update.hpp) commits in place
    // when the key is unchanged (or provably immovable), else applies Policy.
    // Available for every flavour: for A/B it takes the in-place fast path;
    // for C it is the ONLY value-write path, and guards the key.

    // update -- edit the value at _k under the given (or type-default) rekey
    // policy. Returns the fold's outcome; a missing key yields {rejected,
    // npos}.
    template<typename Policy = RekeyPolicy,
             typename Mutate>
    update_result update(
        const key_type& _k,
        Mutate          _mutate
    )
    {
        const std::size_t pos =
            internal::reg_find(m_records, _k, m_kproj, m_cmp);

        // no such key -- nothing to edit
        if (pos >= m_records.size())
        {
            return update_result{ update_status::rejected,
                                  static_cast<std::size_t>(npos) };
        }

        return update_at<Policy, KeyDisjoint>(
            m_records, pos, m_vproj, m_kproj, m_cmp, UniqueKeys, _mutate);
    }

    // try_update -- update guarded: a key move is refused, the entry untouched.
    template<typename Mutate>
    update_result try_update(
        const key_type& _k,
        Mutate          _mutate
    )
    {
        return this->template update<reject_rekey>(
            _k, static_cast<Mutate&&>(_mutate));
    }

    // update_or_move -- update relaxed: a key move relocates the entry to its
    // new sorted position.
    template<typename Mutate>
    update_result update_or_move(
        const key_type& _k,
        Mutate          _mutate
    )
    {
        return this->template update<reposition_rekey>(
            _k, static_cast<Mutate&&>(_mutate));
    }

private:
    storage_type m_records;   // records, invariant: sorted by projected key
    KeyProj      m_kproj;     // key projection   record -> Key   (const coordinate)
    ValueProj    m_vproj;     // value projection record -> Value (writable region)
    KeyCompare   m_cmp;       // strict-weak order on keys
};


// ===========================================================================
// III. kv projections + kv_registry
// ===========================================================================

// kv_key / kv_value
//   projections: the canonical Key x Val projections for a kv_pair record --
// the key (.m_key, read only) and the value (.m_value, writable). Forwarding
// call operators keep const-ness, so one functor serves reads and writes.
struct kv_key
{
    template<typename Pair>
    D_CONSTEXPR
    auto operator()(
        Pair&& _p
    ) const
    -> decltype((static_cast<Pair&&>(_p).m_key))
    {
        return static_cast<Pair&&>(_p).m_key;
    }
};

struct kv_value
{
    template<typename Pair>
    D_CONSTEXPR
    auto operator()(
        Pair&& _p
    ) const
    -> decltype((static_cast<Pair&&>(_p).m_value))
    {
        return static_cast<Pair&&>(_p).m_value;
    }
};

// kv_registry
//   alias: the common registry -- a map/multimap of kv_pair<Key, Value> keyed
// on the pair's key, its value region disjoint from the key (KeyDisjoint =
// true, so the direct value setters are available and edits skip the re-key
// check).
template<typename Key,
         typename Value,
         typename KeyCompare   = std::less<>,
         bool     UniqueKeys   = true,
         typename RekeyPolicy = reject_rekey>
using kv_registry = registry<kv_pair<Key, Value>,
                             kv_key,
                             kv_value,
                             KeyCompare,
                             UniqueKeys,
                             /*KeyDisjoint=*/true,
                             RekeyPolicy>;


// ===========================================================================
// IV.  make_registry
// ===========================================================================

// make_registry (kv)
//   function: a unique-key kv_registry from a list of kv_pairs, deducing the
// key and value types.
template<typename Key,
         typename Value>
D_NODISCARD kv_registry<Key, Value>
make_registry(
    std::initializer_list<kv_pair<Key, Value>> _entries)
{
    return kv_registry<Key, Value>(_entries);
}

// make_registry (general)
//   function: a registry over Record with explicit key / value projections,
// deducing the projection types. KeyDisjoint defaults false (the safe,
// whole-record-capable setting); pass true for a value region carved off the
// key to enable the direct value setters and the in-place update fast path.
template<bool     KeyDisjoint = false,
         bool     UniqueKeys   = true,
         typename Record,
         typename KeyProj,
         typename ValueProj>
D_NODISCARD registry<Record, KeyProj, ValueProj, std::less<>,
                     UniqueKeys, KeyDisjoint>
make_registry(
    std::initializer_list<Record> _records,
    KeyProj                        _kproj,
    ValueProj                      _vproj)
{
    return registry<Record, KeyProj, ValueProj, std::less<>,
                    UniqueKeys, KeyDisjoint>(_records, _kproj, _vproj);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_REGISTRY_REGISTRY_HPP
