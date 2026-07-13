/******************************************************************************
* djinterp [option]                                            option_diff.hpp
*
*   Diff and merge utilities for option_set<>, expressible in BOTH of the
* two shapes option_set itself dispatches over (see option_set.hpp):
*
*     - the TYPE-LEVEL pack form   option_set<opt0, opt1, ...>
*       an immutable compile-time aggregate of option<> types;
*     - the RUNTIME map form       option_set<Key, Value>
*       a key->value container with begin/end, contains, find, insert.
*
*   Accordingly this header is split into two self-contained halves that
* never reference one another:
*
*     PART A - COMPILE-TIME (traits).  Diffs computed entirely at the type
*       level, yielding key_list<...> results and bool/size_t values.  Built
*       on the public query traits (now in option_set.hpp) and the congruity
*       / extractor machinery (option_set_compare.hpp).  "Merge" is the
*       policy-driven option_set_override engine (option_override.hpp);
*       diff_merge_t lets a runtime-style merge_mode name a policy.
*
*     PART B - RUNTIME (functions).  Diffs over any value type exposing the
*       runtime map surface (begin/end, it->key, it->value, contains, find).
*       Returns std::vector<key_type>.  Value-level diffs require mapped_type
*       to have operator==.
*
*   NOTE (2026.06.03 reconciliation):
*   The pre-reconciliation version of this header targeted a runtime
* "option_layer" chain (parent_type / no_parent / is_overridden /
* contains_locally) that never existed in this subframework.  That entire
* layer-diff section has been removed.  The salvageable runtime key/value
* diff + merge survives in PART B, now matched to the real option_set_map
* surface; the compile-time half it always lacked is PART A.
*
*
* path:      /inc/djinterp/core/option/option_diff.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
PART A - COMPILE-TIME (traits over option_set<opt...>)
  A.I.    key-list set operations      (added / removed / common)
  A.II.   key-level diff traits        (option_set_added_keys, ...)
  A.III.  value-level diff traits      (changed / unchanged, extractor-param)
  A.IV.   diff summary                 (diff_count, sets_equal)
  A.V.    merge bridge                 (merge_mode -> policy, diff_merge_t)

PART B - RUNTIME (functions over the option_set_map surface)
  B.I.    key-level diff               (keys_in, added, removed, common)
  B.II.   value-level diff             (changed, unchanged, diff_count, equal)
  B.III.  merge                        (merge_mode + option_merge)
*/

#ifndef DJINTERP_OPTION_DIFF_
#define DJINTERP_OPTION_DIFF_ 1

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./option.hpp"
#include "./option_set.hpp"            // option_set<> + queries (contains, find, is_option_set)
#include "./option_set_compare.hpp"    // key_list, keys, congruity, extractors
#include "./option_override.hpp"       // option_set_override_t + policies


NS_DJINTERP


// ###########################################################################
// PART A - COMPILE-TIME
// ###########################################################################

// ===========================================================================
// A.I.  key-list set operations
// ===========================================================================
//
//   These operate on the key_list<...> packs produced by
// option_set_keys_t (option_set_compare.hpp).  They are the type-level
// analogs of PART B's runtime key-vector builders.

NS_INTERNAL

    // key_list_append
    //   helper: append one NTTP key to a key_list<...>.
    template<typename _List,
             auto     _Key>
    struct key_list_append;

    template<auto... _Ks,
             auto    _Key>
    struct key_list_append<key_list<_Ks...>, _Key>
    {
        using type = key_list<_Ks..., _Key>;
    };

    // key_list_filter_present
    //   helper: walk _Source keys, keep those whose presence in _Other
    // equals _Want.  With _Want == true  this yields the intersection of
    // _Source with _Other; with _Want == false it yields _Source minus
    // _Other.  _Other is an option_set queried via option_set_contains_v.
    template<typename _Other,
             bool     _Want,
             typename _Acc,
             auto...  _SourceKeys>
    struct key_list_filter_present
    {
        using type = _Acc;
    };

    template<typename _Other,
             bool     _Want,
             typename _Acc,
             auto     _Head,
             auto...  _Tail>
    struct key_list_filter_present<_Other, _Want, _Acc, _Head, _Tail...>
    {
    private:
        static D_CONSTEXPR bool keep =
            (option_set_contains_v<_Other, _Head> == _Want);

        using next_acc = std::conditional_t<
            keep,
            typename key_list_append<_Acc, _Head>::type,
            _Acc>;

    public:
        using type = typename key_list_filter_present<
            _Other, _Want, next_acc, _Tail...>::type;
    };


    // key_list_filter_dispatch
    //   helper: unpack a key_list<...> source into key_list_filter_present.
    template<typename _SourceList,
             typename _Other,
             bool     _Want>
    struct key_list_filter_dispatch;

    template<auto... _SourceKeys,
             typename _Other,
             bool     _Want>
    struct key_list_filter_dispatch<key_list<_SourceKeys...>, _Other, _Want>
    {
        using type = typename key_list_filter_present<
            _Other, _Want, key_list<>, _SourceKeys...>::type;
    };

NS_END  // internal


// ===========================================================================
// A.II. key-level diff traits
// ===========================================================================

// option_set_added_keys
//   trait: key_list<...> of keys present in _Derived but not in _Base.
// Type-level analog of PART B's option_added_keys.
template<typename _Base,
         typename _Derived>
struct option_set_added_keys
{
    using type = typename internal::key_list_filter_dispatch<
        option_set_keys_t<_Derived>, _Base, false>::type;
};

template<typename _Base,
         typename _Derived>
using option_set_added_keys_t =
    typename option_set_added_keys<_Base, _Derived>::type;


// option_set_removed_keys
//   trait: key_list<...> of keys present in _Base but not in _Derived.
template<typename _Base,
         typename _Derived>
struct option_set_removed_keys
{
    using type = typename internal::key_list_filter_dispatch<
        option_set_keys_t<_Base>, _Derived, false>::type;
};

template<typename _Base,
         typename _Derived>
using option_set_removed_keys_t =
    typename option_set_removed_keys<_Base, _Derived>::type;


// option_set_common_keys
//   trait: key_list<...> of keys present in BOTH _A and _B (drawn from
// _A's order).
template<typename _A,
         typename _B>
struct option_set_common_keys
{
    using type = typename internal::key_list_filter_dispatch<
        option_set_keys_t<_A>, _B, true>::type;
};

template<typename _A,
         typename _B>
using option_set_common_keys_t = typename option_set_common_keys<_A, _B>::type;


// ===========================================================================
// A.III. value-level diff traits
// ===========================================================================
//
//   "Value" here is whatever a unary extractor pulls from each option's
// arg pack - extract_actual (default), extract_default, or extract_effective
// from option_set_compare.hpp, or any user extractor yielding the
// {value_absent | value_present<V>} carrier interface.  Comparison reuses
// internal::carrier_eq.

NS_INTERNAL

    // kl_filter_changed
    //   helper: walk _A's COMMON keys, keep each per whether the
    // extracted carriers in _A and _B differ (_WantChanged == true) or
    // match (_WantChanged == false).  Only keys present in both sets are
    // considered; key-only differences are the domain of added/removed.
    template<typename                 _A,
             typename                 _B,
             template<typename> typename _Extract,
             bool                     _WantChanged,
             typename                 _Acc,
             auto...                  _CommonKeys>
    struct kl_filter_changed
    {
        using type = _Acc;
    };

    template<typename                 _A,
             typename                 _B,
             template<typename> typename _Extract,
             bool                     _WantChanged,
             typename                 _Acc,
             auto                     _Head,
             auto...                  _Tail>
    struct kl_filter_changed<_A, _B, _Extract, _WantChanged,
                             _Acc, _Head, _Tail...>
    {
    private:
        using a_carrier =
            typename _Extract<option_set_find_t<_A, _Head>>::type;
        using b_carrier =
            typename _Extract<option_set_find_t<_B, _Head>>::type;

        static D_CONSTEXPR bool equal =
            carrier_eq<a_carrier, b_carrier>::value;

        // changed == !equal; keep when (changed == _WantChanged).
        static D_CONSTEXPR bool keep = ((!equal) == _WantChanged);

        using next_acc = std::conditional_t<
            keep,
            typename key_list_append<_Acc, _Head>::type,
            _Acc>;

    public:
        using type = typename kl_filter_changed<
            _A, _B, _Extract, _WantChanged, next_acc, _Tail...>::type;
    };


    // kl_changed_dispatch
    //   helper: unpack the common-key list into kl_filter_changed.
    template<typename                 _A,
             typename                 _B,
             template<typename> typename _Extract,
             bool                     _WantChanged,
             typename                 _CommonList>
    struct kl_changed_dispatch;

    template<typename                 _A,
             typename                 _B,
             template<typename> typename _Extract,
             bool                     _WantChanged,
             auto...                  _CommonKeys>
    struct kl_changed_dispatch<_A, _B, _Extract, _WantChanged,
                               key_list<_CommonKeys...>>
    {
        using type = typename kl_filter_changed<
            _A, _B, _Extract, _WantChanged, key_list<>, _CommonKeys...>::type;
    };

NS_END  // internal


// option_set_changed_keys
//   trait: key_list<...> of keys present in BOTH _Base and _Derived
// whose extracted value differs.  Extractor-parameterized; defaults to
// extract_actual.
template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
struct option_set_changed_keys
{
    using type = typename internal::kl_changed_dispatch<
        _Base, _Derived, _Extract, true,
        option_set_common_keys_t<_Base, _Derived>>::type;
};

template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
using option_set_changed_keys_t =
    typename option_set_changed_keys<_Base, _Derived, _Extract>::type;


// option_set_unchanged_keys
//   trait: key_list<...> of keys present in both sets whose extracted
// value is identical.
template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
struct option_set_unchanged_keys
{
    using type = typename internal::kl_changed_dispatch<
        _Base, _Derived, _Extract, false,
        option_set_common_keys_t<_Base, _Derived>>::type;
};

template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
using option_set_unchanged_keys_t =
    typename option_set_unchanged_keys<_Base, _Derived, _Extract>::type;


// ===========================================================================
// A.IV. diff summary
// ===========================================================================

// option_set_diff_count
//   trait: number of keys that differ between _Base and _Derived under
// _Extract: changed (common keys with differing values) + added + removed.
// Mirrors PART B's option_diff_count.
template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
struct option_set_diff_count
{
    static D_CONSTEXPR std::size_t value =
        ( option_set_changed_keys_t<_Base, _Derived, _Extract>::size +
          option_set_added_keys_t<_Base, _Derived>::size            +
          option_set_removed_keys_t<_Base, _Derived>::size );
};

template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
D_CONSTEXPR_INLINE std::size_t option_set_diff_count_v =
    option_set_diff_count<_Base, _Derived, _Extract>::value;


// option_set_value_equal
//   trait: true iff _Base and _Derived have an empty diff under _Extract
// (same keys, same extracted values everywhere).  This is the diff-side
// spelling of option_set_value_eq (option_set_compare.hpp); kept here so
// "are these two diffs empty" reads naturally next to the count.
template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
struct option_set_value_equal
    : std::integral_constant<bool,
        (option_set_diff_count<_Base, _Derived, _Extract>::value == 0)>
{};

template<typename                 _Base,
         typename                 _Derived,
         template<typename> typename _Extract = extract_actual>
D_CONSTEXPR_INLINE bool option_set_value_equal_v =
    option_set_value_equal<_Base, _Derived, _Extract>::value;


// ===========================================================================
// A.V.  merge bridge
// ===========================================================================
//
//   Type-level "merge" is the policy-driven option_set_override engine.
// To let the runtime-style vocabulary (PART B's merge_mode) name a
// compile-time policy, merge_mode_policy maps each mode to the matching
// override policy, and diff_merge_t applies it.

// merge_mode
//   enum: conflict-resolution vocabulary shared by both halves.  In
// PART A it selects an override policy via merge_mode_policy; in PART B
// it selects an insert strategy in option_merge.
enum class merge_mode
{
    // overwrite existing values with the delta's values
    overwrite,

    // keep existing values; add only keys absent from the base
    add_new_only,

    // alias of add_new_only - keep existing, never overwrite
    keep_existing
};

NS_INTERNAL

    // merge_mode_policy
    //   helper: map a merge_mode to the override policy that realizes
    // it at the type level.
    //     overwrite                  -> override_replace  (delta wins)
    //     add_new_only/keep_existing -> override_keep      (base wins)
    // In every case delta-only keys are added (extension is allowed),
    // matching the runtime option_merge, which always inserts new keys.
    template<merge_mode _Mode>
    struct merge_mode_policy
    {
        using type = override_replace;
    };

    template<>
    struct merge_mode_policy<merge_mode::add_new_only>
    {
        using type = override_keep;
    };

    template<>
    struct merge_mode_policy<merge_mode::keep_existing>
    {
        using type = override_keep;
    };

NS_END  // internal


// diff_merge_t
//   trait: merge _Derived into _Base at the type level under a runtime-
// style merge_mode, yielding a new option_set.  Sugar over
// option_set_override_t with the mode mapped to a policy.  For full
// control (value_only_delta, strict, arg_union_delta, ...) call
// option_set_override_t directly.
//
// Usage:
//   using merged = diff_merge_t<base_set, delta_set>;  // overwrite
//   using added  = diff_merge_t<base_set, delta_set, merge_mode::add_new_only>;
template<typename   _Base,
         typename   _Derived,
         merge_mode _Mode = merge_mode::overwrite>
using diff_merge_t = option_set_override_t<_Base,
                                           _Derived,
                           typename internal::merge_mode_policy<_Mode>::type>;


// ###########################################################################
// PART B - RUNTIME
// ###########################################################################
//
//   Functions over any value exposing the runtime option_set_map surface
// (the option_set<Key, Value> form, or any type with the same shape):
//     ::key_type, ::mapped_type,
//     begin()/end() over entries with public .key / .value,
//     contains(key), find(key), insert(k,v), insert_or_assign(k,v).
//
//   These never touch the type-level pack form; they are ordinary
// runtime algorithms.

// ===========================================================================
// B.I.  key-level diff
// ===========================================================================

// option_keys_in
//   function: returns all keys present in _set.
//
// Parameter(s):
//   _set: the option set/map to enumerate.
// Return:
//   a std::vector<key_type> of every key in _set, in iteration order.
template<typename _Set>
std::vector<typename _Set::key_type>
option_keys_in(
    const _Set& _set
)
{
    typedef typename _Set::key_type key_type;

    std::vector<key_type> result;

    for (auto it = _set.begin(); it != _set.end(); ++it)
    {
        result.push_back(it->key);
    }

    return result;
}


// option_added_keys
//   function: keys present in _derived but not in _base (keys the
// derived set introduces).
//
// Parameter(s):
//   _base:    the baseline set.
//   _derived: the set compared against the baseline.
// Return:
//   a std::vector<key_type> of the added keys.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_added_keys(
    const _SetA& _base,
    const _SetB& _derived
)
{
    typedef typename _SetA::key_type key_type;

    std::vector<key_type> result;

    // collect derived keys absent from base
    for (auto it = _derived.begin(); it != _derived.end(); ++it)
    {
        if (!_base.contains(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}


// option_removed_keys
//   function: keys present in _base but not in _derived (keys that were
// dropped).
//
// Parameter(s):
//   _base:    the baseline set.
//   _derived: the set compared against the baseline.
// Return:
//   a std::vector<key_type> of the removed keys.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_removed_keys(
    const _SetA& _base,
    const _SetB& _derived
)
{
    typedef typename _SetA::key_type key_type;

    std::vector<key_type> result;

    // collect base keys absent from derived
    for (auto it = _base.begin(); it != _base.end(); ++it)
    {
        if (!_derived.contains(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}


// option_common_keys
//   function: keys present in both _a and _b.
//
// Parameter(s):
//   _a: the first set (supplies iteration order).
//   _b: the second set.
// Return:
//   a std::vector<key_type> of the shared keys.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_common_keys(
    const _SetA& _a,
    const _SetB& _b
)
{
    typedef typename _SetA::key_type key_type;

    std::vector<key_type> result;

    // collect a's keys that also appear in b
    for (auto it = _a.begin(); it != _a.end(); ++it)
    {
        if (_b.contains(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}


// ===========================================================================
// B.II. value-level diff
// ===========================================================================
// Require mapped_type to support operator==.

// option_changed_keys
//   function: keys present in both sets whose values differ.
//
// Parameter(s):
//   _base:    the baseline set.
//   _derived: the set compared against the baseline.
// Return:
//   a std::vector<key_type> of the common keys whose values differ.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_changed_keys(
    const _SetA& _base,
    const _SetB& _derived
)
{
    typedef typename _SetA::key_type key_type;

    std::vector<key_type> result;

    // for each base key also in derived, compare values
    for (auto it = _base.begin(); it != _base.end(); ++it)
    {
        auto derived_it = _derived.find(it->key);

        if (derived_it != _derived.end())
        {
            if (!(it->value == derived_it->value))
            {
                result.push_back(it->key);
            }
        }
    }

    return result;
}


// option_unchanged_keys
//   function: keys present in both sets with identical values.
//
// Parameter(s):
//   _base:    the baseline set.
//   _derived: the set compared against the baseline.
// Return:
//   a std::vector<key_type> of the common keys whose values match.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_unchanged_keys(
    const _SetA& _base,
    const _SetB& _derived
)
{
    typedef typename _SetA::key_type key_type;

    std::vector<key_type> result;

    // for each base key also in derived, compare values
    for (auto it = _base.begin(); it != _base.end(); ++it)
    {
        auto derived_it = _derived.find(it->key);

        if (derived_it != _derived.end())
        {
            if (it->value == derived_it->value)
            {
                result.push_back(it->key);
            }
        }
    }

    return result;
}

// option_diff_count
//   function: number of keys that differ between _base and _derived
// (changed + added + removed).
//
// Parameter(s):
//   _base:    the baseline set.
//   _derived: the set compared against the baseline.
// Return:
//   the count of differing keys as a std::size_t.
template<typename _SetA,
         typename _SetB>
std::size_t
option_diff_count(
    const _SetA& _base,
    const _SetB& _derived
)
{
    std::size_t count;

    count = 0;

    // count removed + changed against base
    for (auto it = _base.begin(); it != _base.end(); ++it)
    {
        auto derived_it = _derived.find(it->key);

        if (derived_it == _derived.end())
        {
            // removed
            ++count;
        }
        else if (!(it->value == derived_it->value))
        {
            // changed
            ++count;
        }
    }

    // count added (in derived, absent from base)
    for (auto it = _derived.begin(); it != _derived.end(); ++it)
    {
        if (!_base.contains(it->key))
        {
            ++count;
        }
    }

    return count;
}


// option_sets_equal
//   function: true iff both sets have the same keys with the same values.
//
// Parameter(s):
//   _a: the first set.
//   _b: the second set.
// Return:
//   true iff the diff count between _a and _b is zero; false otherwise.
template<typename _SetA,
         typename _SetB>
bool
option_sets_equal(
    const _SetA& _a,
    const _SetB& _b
)
{
    return (option_diff_count(_a, _b) == 0);
}


// ===========================================================================
// B.III. merge
// ===========================================================================

// option_merge
//   function: merges entries from _source into _target per _mode.
//   overwrite     - assign every source key into target (insert or update).
//   add_new_only  - insert only keys absent from target.
//   keep_existing - identical to add_new_only (never overwrite).
//
// Parameter(s):
//   _target: the set receiving entries; modified in place.
//   _source: the set whose entries are merged in.
//   _mode:   the conflict-resolution strategy (default overwrite).
// Return:
//   the number of entries inserted or modified as a std::size_t.
template<typename _Target,
         typename _Source>
std::size_t
option_merge(
    _Target&         _target,
    const _Source&   _source,
    merge_mode       _mode = merge_mode::overwrite
)
{
    std::size_t count;

    count = 0;

    // fold each source entry into target per the mode
    for (auto it = _source.begin(); it != _source.end(); ++it)
    {
        bool exists = _target.contains(it->key);

        if ( (_mode == merge_mode::add_new_only) ||
             (_mode == merge_mode::keep_existing) )
        {
            if (!exists)
            {
                _target.insert(it->key, it->value);
                ++count;
            }
        }
        else  // overwrite
        {
            _target.insert_or_assign(it->key, it->value);
            ++count;
        }
    }

    return count;
}


NS_END  // djinterp


#endif  // DJINTERP_OPTION_DIFF_