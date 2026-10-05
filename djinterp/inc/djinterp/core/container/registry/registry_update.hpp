/*******************************************************************************
* djinterp [core]                                            registry_update.hpp
*
*   The COMMON FUNCTIONAL CORE shared by every registry flavour -- the single
* body through which a value edit passes, and the two strategies that plug
* into
* its one variation point. A registry is a `map`/`multimap` overlay (keyed
* eta,
* unique/multi key-multiplicity mu on E_key, sorted varsigma) whose KEY COLUMN
* is immutable (the Mutability axis's key-const access restriction). Editing a
* value can, in the whole-record flavour, move the projected key; every other
* flavour carves the key OUT of the value region, so it cannot. Both cases are
* the SAME fold:
*
*     1. read  k_old = key(record)
*     2. hand the caller a mutable handle onto the VALUE region; let them
*   mutate
*     3. read  k_new = key(record)
*     4. k_old ~ k_new  : commit in place        (varsigma and mu still hold)
*     5. otherwise       : invoke the rekey strategy
*
*   Only two knobs vary across the trichotomy (kv_pair rows / projected-key
* records / whole-record value): the VALUE PROJECTION (which sub-region the
* mutable handle exposes) and the REKEY STRATEGY (step 5).  When the value
* region is disjoint from the key, step 5 is DEAD CODE and the recompute is
* skipped entirely -- a static short-circuit, not a runtime branch.
*
*   THE TWO STRATEGIES (step 5):
*     reject_rekey      guarded  -- a key change is refused; the entry is left
*                                   exactly as it was (the mutation is
*                                 performed
*                                   on a copy, so "reject" truly un-rings it).
*     reposition_rekey  relaxed  -- the stale slot is erased and the mutated
*                                   record reinserted at its new key's sorted
*                                   position (a re-sort + mu re-validation,
*                                   composed from the erase/insert
*                                 primitives).
*
*   REUSE.  key equivalence is `equal_under` (comparator.hpp): a ~ b iff
* !cmp(a,b) && !cmp(b,a) -- the E_key of the vocabulary, not a re-rolled test.
* The sorted-key kernel (lower/upper bound, find, ordered insert) is the
* runtime
* counterpart of the sorted invariant and is the only search a registry needs.
*
*   PORTABILITY:
*   C++17 (std::invoke for projection by functor OR pointer-to-member; the
* `if constexpr` static short-circuit).
*
*
* path:      /inc/djinterp/core/container/registry/registry_update.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    value / key projections     (tuple_at, whole_record -- convenience)
      -------------------------------------------------------------------

II.   key_equivalent              (E_key, over equal_under)
      -----------------------------------------------------

III.  sorted-key kernel           (reg_lower_bound / _upper_bound / _find /
      ---------------------------------------------------------------------

      _insert / _erase_at -- runtime, internal)

IV.   rekey vocabulary            (update_status / update_result / strategies)
      ------------------------------------------------------------------------

V.    update_at                   (the single shared fold)
      ----------------------------------------------------
*/

#ifndef DJINTERP_CONTAINER_REGISTRY_REGISTRY_UPDATE_HPP
#define DJINTERP_CONTAINER_REGISTRY_REGISTRY_UPDATE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <functional>   // std::invoke
#include <tuple>        // std::get (tuple_at projection)
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"                   // NS_*, D_NODISCARD, D_CONSTEXPR, clean_t
#include "../../functional/comparator.hpp"      // equal_under (E_key), by_key


NS_DJINTERP


// ===========================================================================
// I.   value / key projections   (convenience)
// ===========================================================================
//   A projection is any callable usable through std::invoke that yields a
// CONST-CORRECT reference into a record: applied to a `record&` it returns a
// mutable reference, applied to a `const record&` a const one.  Pointers-to-
// data-member satisfy this natively (std::invoke(&pair::m_value, rec)); the
// small functors below cover the tuple and whole-record cases.

// tuple_at
//   projection: reads element I of a tuple-like record via std::get, keeping
// the argument's const-ness and value category (a forwarding call operator).
// Serves as a key projection (key at any index) or a value projection.
template<std::size_t I>
struct tuple_at
{
    template<typename Tuple>
    D_CONSTEXPR
    auto operator()(
        Tuple&& _t
    ) const
    -> decltype(std::get<I>(static_cast<Tuple&&>(_t)))
    {
        return std::get<I>(static_cast<Tuple&&>(_t));
    }
};

// whole_record
//   projection: the identity value projection -- the value region IS the
// entire record. This is trichotomy option (C); paired with a key projection
// whose
// coordinate lies inside the record, it is the flavour in which a value edit
// can move the key, so it must be used with KeyDisjoint == false.
struct whole_record
{
    template<typename Record>
    D_CONSTEXPR
    Record& operator()(
        Record& _r
    ) const
    {
        return _r;
    }

    template<typename Record>
    D_CONSTEXPR
    const Record& operator()(
        const Record& _r
    ) const
    {
        return _r;
    }
};


// ===========================================================================
// II.  key_equivalent
// ===========================================================================

// key_equivalent
//   function: the key-equivalence E_key derived from a key comparator, i.e.
// `equal_under(_cmp)` -- a ~ b iff neither key is less than the other. Named
// here so the registry surface speaks the vocabulary directly; it introduces
// no new logic over comparator.hpp's derived predicate.
template<typename KeyCompare>
D_NODISCARD D_CONSTEXPR
auto key_equivalent(
    const KeyCompare& _cmp
)
-> decltype(equal_under(_cmp))
{
    return equal_under(_cmp);
}


// ===========================================================================
// III. sorted-key kernel   (runtime, internal)
// ===========================================================================
//   The runtime realisation of the sorted invariant: a binary search over a
// contiguous record store keyed by a projection.  Shared by the registry's
// lookup surface AND by reposition_rekey, so ordered insert / erase are written
// once.  A record's key is read with std::invoke(_proj, record); order is the
// caller's strict-weak _cmp on the projected keys; equivalence is equal_under.

NS_INTERNAL

    // reg_lower_bound
    //   the first index whose projected key is not less than _k (== _v.size()
    // when every key precedes _k).
    template<typename Vec,
             typename Key,
             typename KeyProj,
             typename Cmp>
    D_NODISCARD std::size_t
    reg_lower_bound(
        const Vec&     _v,
        const Key&     _k,
        const KeyProj& _proj,
        const Cmp&     _cmp
    )
    {
        std::size_t lo = 0;
        std::size_t hi = _v.size();

        while (lo < hi)
        {
            const std::size_t mid = lo + ((hi - lo) / 2);

            // mid's key precedes _k -> the answer is to its right
            if (_cmp(std::invoke(_proj, _v[mid]), _k))
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }

        return lo;
    }

    // reg_upper_bound
    //   the first index whose projected key is strictly greater than _k.
    template<typename Vec,
             typename Key,
             typename KeyProj,
             typename Cmp>
    D_NODISCARD std::size_t
    reg_upper_bound(
        const Vec&     _v,
        const Key&     _k,
        const KeyProj& _proj,
        const Cmp&     _cmp
    )
    {
        std::size_t lo = 0;
        std::size_t hi = _v.size();

        while (lo < hi)
        {
            const std::size_t mid = lo + ((hi - lo) / 2);

            // _k precedes mid's key -> the answer is at or to mid's left
            if (_cmp(_k, std::invoke(_proj, _v[mid])))
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }

        return lo;
    }

    // reg_find
    //   the index of a record whose key is equivalent to _k, or _v.size() on a
    // miss. Equivalence is equal_under (E_key), reused rather than re-spelled.
    template<typename Vec,
             typename Key,
             typename KeyProj,
             typename Cmp>
    D_NODISCARD std::size_t
    reg_find(
        const Vec&     _v,
        const Key&     _k,
        const KeyProj& _proj,
        const Cmp&     _cmp
    )
    {
        const std::size_t pos = reg_lower_bound(_v, _k, _proj, _cmp);
        auto              eq  = equal_under(_cmp);

        // at lower_bound the key is not < _k; a hit needs it not > _k either
        if ( (pos < _v.size()) &&
             (eq(std::invoke(_proj, _v[pos]), _k)) )
        {
            return pos;
        }

        return _v.size();
    }

    // reg_insert
    //   place _rec at its sorted key position, preserving the bundle. Unique
    // (map): a record of an equivalent key is overwritten IN PLACE -- the key
    // is unchanged, so eta/varsigma hold and this is the key-const value
    // write, not a re-key. Non-unique (multimap): inserted after any equal
    // keys (a stable append within the run). Returns the position it occupies.
    template<typename Vec,
             typename Rec,
             typename KeyProj,
             typename Cmp>
    std::size_t
    reg_insert(
        Vec&           _v,
        Rec&&          _rec,
        const KeyProj& _proj,
        const Cmp&     _cmp,
        bool            _unique
    )
    {
        using record_type = clean_t<Rec>;

        auto       eq = equal_under(_cmp);
        const auto k  = std::invoke(_proj, _rec);

        // map: an equivalent key already present is overwritten in place
        if (_unique)
        {
            const std::size_t pos = reg_lower_bound(_v, k, _proj, _cmp);

            if ( (pos < _v.size()) &&
                 (eq(std::invoke(_proj, _v[pos]), k)) )
            {
                _v[pos] = static_cast<record_type&&>(_rec);

                return pos;
            }

            _v.insert(_v.begin() + static_cast<std::ptrdiff_t>(pos),
                      static_cast<record_type&&>(_rec));

            return pos;
        }

        // multimap: insert after any equal keys
        const std::size_t pos = reg_upper_bound(_v, k, _proj, _cmp);

        _v.insert(_v.begin() + static_cast<std::ptrdiff_t>(pos),
                  static_cast<record_type&&>(_rec));

        return pos;
    }

    // reg_erase_at
    //   remove the record at position _pos (leaves the rest sorted).
    template<typename Vec>
    void
    reg_erase_at(
        Vec&       _v,
        std::size_t _pos
    )
    {
        _v.erase(_v.begin() + static_cast<std::ptrdiff_t>(_pos));

        return;
    }

NS_END  // internal


// ===========================================================================
// IV.  rekey vocabulary
// ===========================================================================

// update_status
//   enum: how an update_at resolved.
//     committed -- the value was written (key unchanged, or provably
// immovable).
//     rejected -- a key change was refused (reject_rekey); store untouched.
//     moved -- a key change was honoured (reposition_rekey); entry relocated.
enum class update_status
{
    committed,
    rejected,
    moved
};

// update_result
//   struct: the outcome of update_at -- the status and the entry's index
// afterwards (its unchanged position on commit/reject, its new position on a
// move; lookup_absent-style sentinels are the registry's concern, not this
// kernel's).
struct update_result
{
    update_status status;
    std::size_t   index;
};

// reject_rekey
//   strategy (guarded): a value edit that would move the key is refused. The
// edit is applied to a copy and, on a key change, the copy is discarded, so
// the stored entry is left exactly as it was. The
// invariant-free-by-construction default.
struct reject_rekey
{};

// reposition_rekey
//   strategy (relaxed): a value edit that moves the key is honoured by erasing
// the stale slot and reinserting the mutated record at its new sorted position
// (with mu re-validated for the map). A new sequencing of the erase/insert
// primitives -- no new structural machinery.
struct reposition_rekey
{};


NS_INTERNAL

    // apply_rekey (reject)
    //   discard the mutated copy; the store is already untouched.
    template<typename Vec,
             typename Rec,
             typename KeyProj,
             typename Cmp>
    update_result
    apply_rekey(
        reject_rekey    /*strategy*/,
        Vec&           /*_v*/,
        std::size_t     _pos,
        Rec&&          /*_rec*/,
        const KeyProj& /*_proj*/,
        const Cmp&     /*_cmp*/,
        bool            /*_unique*/
    )
    {
        return update_result{ update_status::rejected, _pos };
    }

    // apply_rekey (reposition)
    //   erase the stale slot, reinsert the mutated record at its new key.
    template<typename Vec,
             typename Rec,
             typename KeyProj,
             typename Cmp>
    update_result
    apply_rekey(
        reposition_rekey /*strategy*/,
        Vec&            _v,
        std::size_t      _pos,
        Rec&&           _rec,
        const KeyProj&  _proj,
        const Cmp&      _cmp,
        bool             _unique
    )
    {
        reg_erase_at(_v, _pos);

        const std::size_t np =
            reg_insert(_v, static_cast<Rec&&>(_rec), _proj, _cmp, _unique);

        return update_result{ update_status::moved, np };
    }

NS_END  // internal


// ===========================================================================
// V.   update_at   (the single shared fold)
// ===========================================================================

// update_at
//   the one body every value edit passes through. _mutate receives a mutable
// reference to the VALUE region (via _vproj) and may change it; the KEY column
// is never handed out. Dispatch:
//
//     KeyDisjoint == true -- the value region cannot reach the key, so the
//                               edit is applied IN PLACE and the recompute /
//                               strategy branch is elided (a compile-time
//                               short-circuit: models A/B pay nothing extra).
//     KeyDisjoint == false -- the edit is applied to a COPY (strong
// guarantee);
//                               if the projected key is unchanged the copy is
//                               committed, otherwise Strategy decides (reject
//                               or reposition). This is trichotomy option (C).
//
//   Parameters: _v the record store, _pos the target position (must be in
// range), _vproj / _kproj the value / key projections, _cmp the key order,
// _unique the map(true)/multimap(false) flag.
template<typename Strategy,
         bool     KeyDisjoint,
         typename Vec,
         typename ValueProj,
         typename KeyProj,
         typename Cmp,
         typename Mutate>
update_result
update_at(
    Vec&            _v,
    std::size_t      _pos,
    const ValueProj& _vproj,
    const KeyProj&   _kproj,
    const Cmp&       _cmp,
    bool              _unique,
    Mutate            _mutate
)
{
    if constexpr (KeyDisjoint)
    {
        // value region disjoint from the key: mutate in place, key cannot move
        _mutate(std::invoke(_vproj, _v[_pos]));

        return update_result{ update_status::committed, _pos };
    }
    else
    {
        using record_type = clean_t<decltype(_v[_pos])>;

        // edit a copy so a refusal can leave the stored entry untouched
        record_type tmp = _v[_pos];
        _mutate(std::invoke(_vproj, tmp));

        auto eq = equal_under(_cmp);

        // key unchanged -> commit in place (varsigma / mu preserved)
        if ( eq(std::invoke(_kproj, _v[_pos]),
                std::invoke(_kproj, tmp)) )
        {
            _v[_pos] = static_cast<record_type&&>(tmp);

            return update_result{ update_status::committed, _pos };
        }

        // key moved -> the strategy decides (reject / reposition)
        return internal::apply_rekey(
            Strategy{}, _v, _pos,
            static_cast<record_type&&>(tmp), _kproj, _cmp, _unique);
    }
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_REGISTRY_REGISTRY_UPDATE_HPP
