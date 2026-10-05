/*******************************************************************************
* djinterp [core]                                             fixed_registry.hpp
*
*   fixed_registry -- a registry whose KEY SET is settled at construction and
* never grows or shrinks thereafter: STRUCTURALLY FIXED.  On the Mutability
* axis it is the value-mutable grade with the map's key-const access
* restriction -- element mutation (overwrite a value in place) WITHOUT
* structural mutation (no insert / erase / clear).  This is the runtime,
* value-carrying analogue of option_set: fixed keys addressing settable value
* slots, established once and then only read or re-valued.
*
*     grade value-mutable (element mutation, no structural change)
*     access restriction key-const      (the key coordinate frozen)
*
*   FIXITY FORCES REJECT. A registry's guarded update fold offers two answers
* to
* an edit that moves a key: reject (leave the entry) or reposition (erase the
* stale slot and reinsert at the new key). Reposition IS structural mutation
* --
* it changes the set of positions -- so it is unavailable here: a
* fixed_registry
* pins the rekey policy to reject_rekey and exposes update() (reject-only) and
* try_update(), but not update_or_move().  A key move is therefore always
* refused, the entry left untouched.
*
*   WHAT IS AND IS NOT OFFERED.  Re-exported from the base verbatim: the whole
* const surface (size / lookup / iteration), the key-const value surface (read
* always; the direct writes at / operator[] / set when the value region is
* disjoint from the key), and the reject-only update path.  WITHHELD: insert,
* erase, erase_at, clear -- the structural operations. The container is thus a
* container by the Mutability section's own test (it answers a const size()
* and
* admits element mutation), distinguished from a plain value, yet fixed in
* shape.
*
*   NOTE ON LIFETIME vs MUTABILITY.  "Fixed" here is a MUTABILITY statement
* (which changes the interface admits), not a Lifetime one: a fixed_registry
* is
* still built at runtime with a runtime-expressible size; its size simply
* cannot
* change once built.  A fully const, observation-only registry (the immutable
* grade, withholding even value writes) is a further tightening of this one.
*
*   PORTABILITY:
*   C++17 (inherits the base registry's requirements).
*
*
* path:      /inc/djinterp/core/container/registry/fixed_registry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_fixed_registry (detection trait)
      -----------------------------------

II.   fixed_registry (class)
      ----------------------
      1.    re-exported base surface (member types, lookup, value)
      2.    axis markers (lifetime immutable; structurally_fixed; value_mutable)
      3.    construction (the key set is settled here, once)
      4.    value update (reject-only; no reposition)

III.  fixed_kv_registry
      -----------------
*/

#ifndef DJINTERP_CONTAINER_REGISTRY_FIXED_REGISTRY_HPP
#define DJINTERP_CONTAINER_REGISTRY_FIXED_REGISTRY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"        // NS_*, D_NODISCARD, clean_t
#include "./registry.hpp"            // registry (base), kv_pair, reject_rekey


NS_DJINTERP


// ===========================================================================
// I.   is_fixed_registry (detection trait)
// ===========================================================================

// fixed_registry (fwd)
template<typename    Record,
         typename    KeyProj,
         typename    ValueProj,
         typename    KeyCompare,
         bool        UniqueKeys,
         bool        KeyDisjoint,
         typename    SizeType,
         typename    DifferenceType,
         typename... Options>
class fixed_registry;

NS_INTERNAL

    template<typename Type>
    struct is_fixed_registry_impl : std::false_type
    {};

    template<typename    R, typename KP, typename VP, typename KC,
             bool        U, bool KD, typename S, typename D, typename... O>
    struct is_fixed_registry_impl<
        fixed_registry<R, KP, VP, KC, U, KD, S, D, O...>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_fixed_registry : internal::is_fixed_registry_impl<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
inline constexpr bool is_fixed_registry_v = is_fixed_registry<Type>::value;
#endif


// ===========================================================================
// II.  fixed_registry (class)
// ===========================================================================

// fixed_registry
//   class: the base registry with structural mutation withdrawn and the rekey
// policy pinned to reject. Private inheritance takes the implementation; the
// public surface re-exports everything EXCEPT the structural operations and
// the policy-taking update, and adds a reject-only update.
template<typename    Record,
         typename    KeyProj,
         typename    ValueProj,
         typename    KeyCompare      = std::less<>,
         bool        UniqueKeys      = true,
         bool        KeyDisjoint     = false,
         typename    SizeType        = std::size_t,
         typename    DifferenceType = std::ptrdiff_t,
         typename... Options>
class fixed_registry
    : private registry<Record, KeyProj, ValueProj, KeyCompare,
                       UniqueKeys, KeyDisjoint, reject_rekey,
                       SizeType, DifferenceType, Options...>
{
private:
    using base = registry<Record, KeyProj, ValueProj, KeyCompare,
                          UniqueKeys, KeyDisjoint, reject_rekey,
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
    using typename base::rekey_policy;      // reject_rekey (pinned)
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

    // value surface: read always; direct writes (at / operator[] / set /
    // try_get) when the value region is disjoint from the key -- element
    // mutation, permitted for a fixed (value-mutable) registry
    using base::at;
    using base::try_get;
    using base::operator[];
    using base::set;

    // guarded reject-only update (try_update is reject-based already)
    using base::try_update;

    // sentinels carried through
    using base::npos;

    //   NOTE the DELIBERATE OMISSIONS: base::insert, base::erase,
    // base::erase_at, base::clear (structural mutation) and base::update /
    // base::update_or_move (the latter would reposition = structural). A
    // reject-only update is re-provided in section 4.

    // --- 2. axis markers ---

    static constexpr bool keyed            = base::keyed;
    static constexpr bool sorted_invariant = base::sorted_invariant;
    static constexpr bool unique_keys      = base::unique_keys;
    static constexpr bool key_const        = base::key_const;
    static constexpr bool value_disjoint   = base::value_disjoint;

    // fixity: structurally frozen, but value cells remain writable (key-const)
    static constexpr bool structurally_fixed  = true;
    static constexpr bool structural_mutation = false;
    static constexpr bool value_mutable       = true;

    static constexpr container_storage_kind  storage_kind       = base::storage_kind;
    static constexpr container_ordering      ordering           = base::ordering;
    static constexpr container_bounds        bounds             = base::bounds;
    static constexpr container_iterability   iterability        = base::iterability;
    static constexpr container_multiplicity  multiplicity_grade = base::multiplicity_grade;
    static constexpr container_structure     structure          = base::structure;

    // fixed after construction: immutable lifetime (the value is settled and
    // enduring, even though its cells stay individually writable)
    static constexpr container_lifetime      lifetime = container_lifetime::immutable;

    // --- 3. construction ---
    //   The key set is settled HERE and nowhere else. After construction no
    // structural operation exists, so the shape is frozen.

    fixed_registry()
        : base()
    {}

    explicit fixed_registry(
        KeyProj     _kproj,
        ValueProj   _vproj,
        KeyCompare _cmp = KeyCompare()
    )
        : base(_kproj, _vproj, _cmp)
    {}

    // record list: the frozen contents. Each record is placed at its sorted
    // key position (the base init-list constructor), establishing eta /
    // varsigma / mu once; thereafter the shape cannot change.
    fixed_registry(
        std::initializer_list<Record> _records,
        KeyProj                        _kproj = KeyProj(),
        ValueProj                      _vproj = ValueProj(),
        KeyCompare                     _cmp   = KeyCompare()
    )
        : base(_records, _kproj, _vproj, _cmp)
    {}

    fixed_registry(const fixed_registry&)            = default;
    fixed_registry(fixed_registry&&)                 = default;
    fixed_registry& operator=(const fixed_registry&) = default;
    fixed_registry& operator=(fixed_registry&&)      = default;
    ~fixed_registry()                                = default;

    // --- 4. value update (reject-only; no reposition) ---
    //   The universal value-edit path, hard-pinned to reject: a key move is
    // refused (repositioning it would be structural mutation, which this
    // container does not admit). For a whole-record (non-disjoint) registry
    // this is the only value-write path, and it guards the key.

    // update -- edit the value at _k; a mutation that moves the key is
    // rejected and the entry left untouched. No policy parameter is offered
    // (reject is the only lawful answer for a fixed container).
    template<typename Mutate>
    update_result update(
        const key_type& _k,
        Mutate          _mutate
    )
    {
        return base::template update<reject_rekey>(
            _k, static_cast<Mutate&&>(_mutate));
    }
};


// ===========================================================================
// III. fixed_kv_registry
// ===========================================================================

// fixed_kv_registry
//   alias: a structurally-frozen kv map/multimap -- fixed keys, settable
// values. The direct value setters are available (the value region is disjoint
// from the key); the shape is fixed at construction. The runtime
// value-carrying option_set, as a registry.
template<typename Key,
         typename Value,
         typename KeyCompare = std::less<>,
         bool     UniqueKeys = true>
using fixed_kv_registry = fixed_registry<kv_pair<Key, Value>,
                                        kv_key,
                                        kv_value,
                                        KeyCompare,
                                        UniqueKeys,
                                        /*KeyDisjoint=*/true>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_REGISTRY_FIXED_REGISTRY_HPP
