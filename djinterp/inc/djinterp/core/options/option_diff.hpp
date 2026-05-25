/******************************************************************************
* djinterp [options]                                          option_diff.hpp
*
* Comparison and diff utilities for option sets and layers.
*   Provides free functions for computing the difference between two
* option sets or layers: which keys differ, which are added, which are
* removed, and which are unchanged.
*
*   All functions operate on any pair of types that satisfy the
* option_set structural contract (is_option_set_like).  They work
* equally on option_set, option_layer, std::map, or any user type
* that exposes the required surface.
*
*   RESULTS:
*   Diff results are returned as std::vector<key_type> containing the
* keys in each category.  For value-level comparison, the sets must
* have a mapped_type with operator==.
*
*   LAYER-SPECIFIC UTILITIES:
*   For option layers, additional functions determine the origin of
* each key's effective value, whether a key is locally overridden,
* and how many overrides a layer contributes relative to its parent.
*
* DEPENDENCIES:
*   djinterp.hpp       - D_CONSTEXPR, namespaces
*   option_set.hpp     - option_set structural contract
*
* TABLE OF CONTENTS
* =================
* I.    Key-Level Diff
* II.   Value-Level Diff
* III.  Layer Diff
* IV.   Merge
*
*
* path:      /inc/djinterp/options/option_diff.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

#ifndef DJINTERP_OPTION_DIFF_
#define DJINTERP_OPTION_DIFF_ 1

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Key-Level Diff
// ===========================================================================
// These functions compare two option sets by key presence only,
// without examining values.

// option_keys_in
//   function: returns all keys present in _set.
template<typename _Set>
std::vector<typename _Set::key_type>
option_keys_in(const _Set& _set)
{
    using key_type = typename _Set::key_type;

    std::vector<key_type> result;

    for (auto it = _set.begin(); it != _set.end(); ++it)
    {
        result.push_back(it->key);
    }

    return result;
}

// option_added_keys
//   function: returns keys that exist in _derived but not
// in _base.  These are "new" keys introduced by the derived
// set.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_added_keys(const _SetA& _base,
                  const _SetB& _derived)
{
    using key_type = typename _SetA::key_type;

    std::vector<key_type> result;

    for (auto it = _derived.begin();
         it != _derived.end();
         ++it)
    {
        if (!_base.contains(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}

// option_removed_keys
//   function: returns keys that exist in _base but not in
// _derived.  These are keys that were dropped.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_removed_keys(const _SetA& _base,
                    const _SetB& _derived)
{
    using key_type = typename _SetA::key_type;

    std::vector<key_type> result;

    for (auto it = _base.begin();
         it != _base.end();
         ++it)
    {
        if (!_derived.contains(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}

// option_common_keys
//   function: returns keys that exist in both _a and _b.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_common_keys(const _SetA& _a,
                   const _SetB& _b)
{
    using key_type = typename _SetA::key_type;

    std::vector<key_type> result;

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
// II.  Value-Level Diff
// ===========================================================================
// These functions compare two option sets by both key AND value.
// Requires mapped_type to support operator==.

// option_changed_keys
//   function: returns keys that exist in both _base and
// _derived but whose values differ.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_changed_keys(const _SetA& _base,
                    const _SetB& _derived)
{
    using key_type = typename _SetA::key_type;

    std::vector<key_type> result;

    for (auto it = _base.begin();
         it != _base.end();
         ++it)
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
//   function: returns keys that exist in both _base and
// _derived with identical values.
template<typename _SetA,
         typename _SetB>
std::vector<typename _SetA::key_type>
option_unchanged_keys(const _SetA& _base,
                      const _SetB& _derived)
{
    using key_type = typename _SetA::key_type;

    std::vector<key_type> result;

    for (auto it = _base.begin();
         it != _base.end();
         ++it)
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
//   function: returns the number of keys that differ between
// _base and _derived (changed + added + removed).
template<typename _SetA,
         typename _SetB>
std::size_t
option_diff_count(const _SetA& _base,
                  const _SetB& _derived)
{
    std::size_t count = 0;

    // count changed values
    for (auto it = _base.begin();
         it != _base.end();
         ++it)
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

    // count added keys (in derived but not in base)
    for (auto it = _derived.begin();
         it != _derived.end();
         ++it)
    {
        if (!_base.contains(it->key))
        {
            ++count;
        }
    }

    return count;
}

// option_sets_equal
//   function: returns true if both sets have the same keys
// with the same values.
template<typename _SetA,
         typename _SetB>
bool
option_sets_equal(const _SetA& _a,
                  const _SetB& _b)
{
    return (option_diff_count(_a, _b) == 0);
}


// ===========================================================================
// III. Layer Diff
// ===========================================================================
// Utilities specific to option_layer chains.

// option_layer_overrides
//   function: returns the keys in a layer that shadow an
// inherited value (i.e. the key exists both locally and in
// the parent chain).
template<typename _Layer>
std::vector<typename _Layer::key_type>
option_layer_overrides(const _Layer& _layer)
{
    using key_type = typename _Layer::key_type;

    std::vector<key_type> result;

    for (auto it = _layer.begin();
         it != _layer.end();
         ++it)
    {
        if (_layer.is_overridden(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}

// option_layer_additions
//   function: returns the keys in a layer that are new (not
// present in the parent chain).
template<typename _Layer>
std::vector<typename _Layer::key_type>
option_layer_additions(const _Layer& _layer)
{
    using key_type = typename _Layer::key_type;

    std::vector<key_type> result;

    for (auto it = _layer.begin();
         it != _layer.end();
         ++it)
    {
        if (!_layer.is_overridden(it->key) &&
            _layer.contains_locally(it->key))
        {
            result.push_back(it->key);
        }
    }

    return result;
}

// option_layer_effective_keys
//   function: collects all effective keys visible through
// a layer chain, starting at _layer and walking up to the
// root.  Each key appears once - the first encounter (most
// specific layer) wins.
//
// Dispatch is by internal tag: layers whose parent_type is
// no_parent are roots; all others recurse into the parent.

NS_INTERNAL

    // effective_keys_impl (chained)
    //   helper: collects local keys then recurses into the
    // parent, deduplicating.
    template<typename _Layer>
    std::vector<typename _Layer::key_type>
    effective_keys_impl(const _Layer& _layer,
                        std::false_type /*is_root*/)
    {
        using key_type = typename _Layer::key_type;

        // start with local keys
        std::vector<key_type> result;

        for (auto it = _layer.begin();
             it != _layer.end();
             ++it)
        {
            result.push_back(it->key);
        }

        // collect parent keys, skipping duplicates
        auto parent_keys =
            option_layer_effective_keys(_layer.parent());

        for (std::size_t i = 0;
             i < parent_keys.size();
             ++i)
        {
            bool found = false;

            for (std::size_t j = 0;
                 j < result.size();
                 ++j)
            {
                if (result[j] == parent_keys[i])
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                result.push_back(parent_keys[i]);
            }
        }

        return result;
    }

    // effective_keys_impl (root)
    //   helper: root layer - just return the local keys.
    template<typename _Layer>
    std::vector<typename _Layer::key_type>
    effective_keys_impl(const _Layer& _layer,
                        std::true_type /*is_root*/)
    {
        return option_keys_in(_layer);
    }

NS_END  // internal

template<typename _Layer>
std::vector<typename _Layer::key_type>
option_layer_effective_keys(const _Layer& _layer)
{
    using is_root = std::is_same<
        typename _Layer::parent_type, no_parent>;

    return internal::effective_keys_impl(
        _layer, is_root{});
}

// option_layer_effective_size
//   function: returns the total number of unique keys
// visible through the layer chain.
template<typename _Layer>
std::size_t
option_layer_effective_size(const _Layer& _layer)
{
    return option_layer_effective_keys(_layer).size();
}


// ===========================================================================
// IV.  Merge
// ===========================================================================
// Merges entries from one set into another, with
// configurable conflict resolution.

// option_merge_mode
//   enum: how to handle key conflicts during merge.
enum class option_merge_mode
{
    // keep existing values; skip conflicting keys
    keep_existing,

    // overwrite existing values with source values
    overwrite,

    // skip keys that already exist; only add new keys
    add_new_only
};

// option_merge
//   function: merges entries from _source into _target
// according to the merge mode.
//
// Returns the number of entries that were inserted or
// modified.
template<typename _Target,
         typename _Source>
std::size_t
option_merge(_Target&           _target,
             const _Source&      _source,
             option_merge_mode  _mode = option_merge_mode::overwrite)
{
    std::size_t count = 0;

    for (auto it = _source.begin();
         it != _source.end();
         ++it)
    {
        bool exists = _target.contains(it->key);

        if (_mode == option_merge_mode::add_new_only)
        {
            if (!exists)
            {
                _target.insert(it->key, it->value);
                ++count;
            }
        }
        else if (_mode == option_merge_mode::keep_existing)
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
