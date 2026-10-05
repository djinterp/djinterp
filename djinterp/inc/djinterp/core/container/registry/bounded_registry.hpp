/*******************************************************************************
* djinterp [core]                                           bounded_registry.hpp
*
*   bounded_registry -- a registry carrying a CAPACITY (the Boundedness axis).
* The Boundedness section fixes a capacity
*
*       kappa in |N u {inf},      c valid  <=>  |c| <= kappa,
*
* the type being BOUNDED exactly when kappa < inf.  bounded_registry pins a
* compile-time kappa = Capacity and enforces |c| <= kappa at the one place
* size
* can grow: insertion. Everything else -- the keyed/sorted/multiplicity
* overlay,
* the key-const value surface, the guarded update fold -- is the base registry
* unchanged, so this is the base container with one added invariant, not a new
* one.
*
*   WHERE THE CAP BITES.  Capacity constrains only GROWTH, and only an insert
* that actually adds a position grows the container:
*     - unique keys (map): inserting an EXISTING key overwrites its value in
*       place (eta holds, |c| unchanged) and is always allowed, even when
*     full;
*       only a NEW key grows and is refused at capacity.
*     - repeated keys (multimap): every insert adds a position, so any insert
*   is
*       refused at capacity.
* A value overwrite (set / at / operator[]) and a repositioning update
* (erase-then-reinsert, net size unchanged) never grow the container and are
* never capacity-limited. A refused insert throws std::length_error and leaves
* the container untouched (the strong guarantee the base insert already
* gives).
*
*   ORTHOGONALITY.  Capacity caps the WHOLE container; multiplicity caps each
* key-class; the realised per-class count obeys #_E(c,x) <= min(m, kappa) (the
* Boundedness/Multiplicity interplay).  Domain-boundedness -- restricting the
* VALUES to a closed interval -- is the other, independent Boundedness axis
* and
* is not imposed here (a bounded_registry is size-bounded, domain-free).
*
*   STORAGE.  The cap is a logical bound, kept independent of Storage: the
* backing is still the base's out-of-line vector (dynamic storage), merely
* never
* grown past kappa.  A truly INLINE, static-storage bounded registry (cells
* reserved in-object, no allocator) is a further, storage-level variant.
*
*   PORTABILITY:
*   C++17 (inherits the base registry's requirements; adds only a size_t
* NTTP).
*
*
* path:      /inc/djinterp/core/container/registry/bounded_registry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_bounded_registry (detection trait)
      -------------------------------------

II.   bounded_registry (class)
      ------------------------
      1.    re-exported base surface (member types, lookup, value, update)
      2.    axis markers (bounds overridden to bounded; capacity)
      3.    construction (capacity-checked)
      4.    capacity queries (capacity / full / remaining)
      5.    structural mutation (capacity-guarded insert; base erase/clear)

III.  bounded_kv_registry
      -------------------
*/

#ifndef DJINTERP_CONTAINER_REGISTRY_BOUNDED_REGISTRY_HPP
#define DJINTERP_CONTAINER_REGISTRY_BOUNDED_REGISTRY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"        // NS_*, D_NODISCARD, clean_t
#include "./registry.hpp"            // registry (base), kv_pair, rekey policies


NS_DJINTERP


// ===========================================================================
// I.   is_bounded_registry (detection trait)
// ===========================================================================

// bounded_registry (fwd)
template<typename    Record,
         typename    KeyProj,
         typename    ValueProj,
         std::size_t Capacity,
         typename    KeyCompare,
         bool        UniqueKeys,
         bool        KeyDisjoint,
         typename    RekeyPolicy,
         typename    SizeType,
         typename    DifferenceType,
         typename... Options>
class bounded_registry;

NS_INTERNAL

    template<typename Type>
    struct is_bounded_registry_impl : std::false_type
    {};

    template<typename    R, typename KP, typename VP, std::size_t Cap,
             typename    KC, bool U, bool KD, typename RP,
             typename    S, typename D, typename... O>
    struct is_bounded_registry_impl<
        bounded_registry<R, KP, VP, Cap, KC, U, KD, RP, S, D, O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_bounded_registry : internal::is_bounded_registry_impl<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
inline constexpr bool is_bounded_registry_v = is_bounded_registry<Type>::value;
#endif


// ===========================================================================
// II.  bounded_registry (class)
// ===========================================================================

// bounded_registry
//   class: the base registry plus a capacity kappa = Capacity. Private
// inheritance takes the base's whole implementation; the surface is
// re-exported verbatim except (a) the bounds axis marker, now `bounded`, and
// (b) insert, which refuses a growth past kappa.
template<typename    Record,
         typename    KeyProj,
         typename    ValueProj,
         std::size_t Capacity,
         typename    KeyCompare      = std::less<>,
         bool        UniqueKeys      = true,
         bool        KeyDisjoint     = false,
         typename    RekeyPolicy     = reject_rekey,
         typename    SizeType        = std::size_t,
         typename    DifferenceType = std::ptrdiff_t,
         typename... Options>
class bounded_registry
    : private registry<Record, KeyProj, ValueProj, KeyCompare,
                       UniqueKeys, KeyDisjoint, RekeyPolicy,
                       SizeType, DifferenceType, Options...>
{
private:
    using base = registry<Record, KeyProj, ValueProj, KeyCompare,
                          UniqueKeys, KeyDisjoint, RekeyPolicy,
                          SizeType, DifferenceType, Options...>;

public:
    // --- 1. re-exported base surface ---

    // member types
    using typename base::value_type;
    using typename base::record_type;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::const_iterator;
    using typename base::key_projection;
    using typename base::value_projection;
    using typename base::key_compare;
    using typename base::rekey_policy;
    using typename base::key_type;
    using typename base::mapped_type;

    // record access + iteration (const)
    using base::size;
    using base::empty;
    using base::record_at;
    using base::key_of;
    using base::begin;
    using base::end;
    using base::cbegin;
    using base::cend;

    // key queries
    using base::index_of;
    using base::find;
    using base::contains;
    using base::count;

    // value surface (read always; write when KeyDisjoint) -- growth-free,
    // hence never capacity-limited
    using base::at;
    using base::try_get;
    using base::operator[];
    using base::set;

    // value update family (reject / reposition; reposition is
    // net-size-neutral, so it stays within kappa)
    using base::update;
    using base::try_update;
    using base::update_or_move;

    // structural removal (only shrinks) + clear
    using base::erase;
    using base::erase_at;
    using base::clear;

    // sentinels / overlay markers carried through
    using base::npos;

    // --- 2. axis markers ---
    //   Carried from the base, except bounds, now `bounded`, and the added
    // capacity constant.

    static constexpr bool keyed            = base::keyed;
    static constexpr bool sorted_invariant = base::sorted_invariant;
    static constexpr bool unique_keys      = base::unique_keys;
    static constexpr bool key_const        = base::key_const;
    static constexpr bool value_disjoint   = base::value_disjoint;

    static constexpr container_lifetime      lifetime           = base::lifetime;
    static constexpr container_storage_kind  storage_kind       = base::storage_kind;
    static constexpr container_ordering      ordering           = base::ordering;
    static constexpr container_iterability   iterability        = base::iterability;
    static constexpr container_multiplicity  multiplicity_grade = base::multiplicity_grade;
    static constexpr container_structure     structure          = base::structure;

    // the axis this variant fixes: a finite capacity kappa < inf
    static constexpr container_bounds        bounds   = container_bounds::bounded;
    static constexpr size_type               capacity = static_cast<size_type>(Capacity);

    // --- 3. construction (capacity-checked) ---

    bounded_registry()
        : base()
    {}

    explicit bounded_registry(
        KeyProj     _kproj,
        ValueProj   _vproj,
        KeyCompare _cmp = KeyCompare()
    )
        : base(_kproj, _vproj, _cmp)
    {}

    // record list: inserted one by one through the capacity-guarded insert, so
    // a list longer than kappa throws rather than silently overflowing.
    bounded_registry(
        std::initializer_list<Record> _records,
        KeyProj                        _kproj = KeyProj(),
        ValueProj                      _vproj = ValueProj(),
        KeyCompare                     _cmp   = KeyCompare()
    )
        : base(_kproj, _vproj, _cmp)
    {
        for (const Record& rec : _records)
        {
            insert(rec);
        }
    }

    bounded_registry(const bounded_registry&)            = default;
    bounded_registry(bounded_registry&&)                 = default;
    bounded_registry& operator=(const bounded_registry&) = default;
    bounded_registry& operator=(bounded_registry&&)      = default;
    ~bounded_registry()                                  = default;

    // --- 4. capacity queries ---

    // full -- whether the container is at capacity (no NEW key may be added).
    D_NODISCARD bool full() const noexcept
    {
        return this->size() >= static_cast<size_type>(Capacity);
    }

    // remaining -- capacity headroom, kappa - |c|.
    D_NODISCARD size_type remaining() const noexcept
    {
        const size_type n = this->size();

        return (static_cast<size_type>(Capacity) > n)
                   ? static_cast<size_type>(static_cast<size_type>(Capacity) - n)
                   : size_type(0);
    }

    // --- 5. structural mutation (capacity-guarded insert) ---
    //   base::insert is deliberately NOT re-exported; these shadow it with a
    // pre-growth capacity check. A refused insert throws and mutates nothing.

    // would_grow -- whether inserting a record of key _k adds a position.
    D_NODISCARD bool would_grow(const key_type& _k) const
    {
        return (!UniqueKeys) || (!this->contains(_k));
    }

    // insert (record) -- place _rec at its sorted key position, refusing a
    // capacity-exceeding growth.
    size_type insert(Record _rec)
    {
        if (would_grow(this->key_of(_rec)) &&
            this->size() >= static_cast<size_type>(Capacity))
        {
            throw std::length_error(
                "bounded_registry::insert: capacity exceeded.");
        }

        return base::insert(static_cast<Record&&>(_rec));
    }

    // insert (key, value) -- convenience for (key, value)-constructible
    // records; same capacity guard.
    template<typename K = key_type,
             typename V = mapped_type,
             typename std::enable_if<
                 std::is_constructible<Record, K, V>::value, int>::type = 0>
    size_type insert(
        K _k,
        V _v
    )
    {
        if (would_grow(_k) &&
            this->size() >= static_cast<size_type>(Capacity))
        {
            throw std::length_error(
                "bounded_registry::insert: capacity exceeded.");
        }

        return base::insert(static_cast<K&&>(_k), static_cast<V&&>(_v));
    }

    // try_insert -- non-throwing insert: returns npos (adding nothing) when a
    // growth would exceed capacity, else the inserted position.
    size_type try_insert(Record _rec)
    {
        if (would_grow(this->key_of(_rec)) &&
            this->size() >= static_cast<size_type>(Capacity))
        {
            return npos;
        }

        return base::insert(static_cast<Record&&>(_rec));
    }
};


// ===========================================================================
// III. bounded_kv_registry
// ===========================================================================

// bounded_kv_registry
//   alias: a capacity-capped kv map/multimap -- kv_registry with kappa. Value
// region disjoint from the key (the direct setters are available; edits skip
// the re-key check).
template<typename    Key,
         typename    Value,
         std::size_t Capacity,
         typename    KeyCompare   = std::less<>,
         bool        UniqueKeys   = true,
         typename    RekeyPolicy = reject_rekey>
using bounded_kv_registry = bounded_registry<kv_pair<Key, Value>,
                                            kv_key,
                                            kv_value,
                                            Capacity,
                                            KeyCompare,
                                            UniqueKeys,
                                            /*KeyDisjoint=*/true,
                                            RekeyPolicy>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_REGISTRY_BOUNDED_REGISTRY_HPP
