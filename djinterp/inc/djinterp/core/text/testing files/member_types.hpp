/******************************************************************************
* djinterp [meta]                                              member_types.hpp
*
*   The framework's shared nested-TYPEDEF detectors - the concrete "does _Type
* expose a `::X` typedef?" traits that recur across more than one subsystem -
* plus the "extract a typedef or fall back" helper.  Replaces member_traits.hpp.
*
*   WHAT MOVED, AND WHY:
*   member_traits.hpp carried its own copy of the nested-typedef detection
* macro (D_DEFINE_HAS_MEMBER_TYPE) and the extract-or-fallback macro
* (D_DEFINE_MEMBER_TYPE_OR).  The detection macro differed from type_traits'
* D_TRAIT_HAS_TYPE only by stripping cv-ref through clean_t first - a single
* resolution difference, not a second mechanism.  Both macro families now live
* once in trait_detect.hpp (the canonical D_TYPE_TRAIT_HAS_TYPE adopts the
* clean_t behavior; D_TYPE_TRAIT_MEMBER_TYPE_OR is the extractor).  This header
* keeps only the CONCRETE traits those macros produce.
*
*   SCOPE - typedef detectors only:
*   These detect nested TYPE aliases.  Method and value/constexpr probes
* (has_size() callable, constexpr-evaluable size(), ...) are a different shape
* and are container-domain; they live with their domain (container_traits.hpp),
* not here.  The line this header holds is: a `::X` typedef that two or more
* unrelated subsystems care about.
*
*
* path:      /inc/djinterp/core/meta/member_types.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_META_MEMBER_TYPES_
#define DJINTERP_META_MEMBER_TYPES_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./trait_detect.hpp"   // D_TYPE_TRAIT_HAS_TYPE, _MEMBER_TYPE_OR


NS_DJINTERP


// ===========================================================================
// I.   shared nested-typedef detectors
// ===========================================================================
//   Defining each here once means a fix to the detection idiom (in
// trait_detect.hpp) propagates to every consumer instead of needing an edit
// per subsystem.  Each emits `has_<name>` and (on C++14+) `has_<name>_v`.

// --- parser / scanner / token / pattern contracts -------------------------

// has_input_type
//   trait: detects a nested `input_type` typedef (element type of an input
// stream; parser and scanner contracts).
D_TYPE_TRAIT_HAS_TYPE(has_input_type, input_type)

// has_result_type
//   trait: detects a nested `result_type` typedef (type produced on success;
// parser and scanner contracts).
D_TYPE_TRAIT_HAS_TYPE(has_result_type, result_type)

// has_item_type
//   trait: detects a nested `item_type` typedef (a discovered element; the
// scanner contract).
D_TYPE_TRAIT_HAS_TYPE(has_item_type, item_type)

// has_kind_type
//   trait: detects a nested `kind_type` typedef (a discriminator; the token
// contract).
D_TYPE_TRAIT_HAS_TYPE(has_kind_type, kind_type)

// --- std-style container / associative contracts -------------------------

// has_value_type
//   trait: detects a nested `value_type` typedef (a payload; the token
// contract and most containers).
D_TYPE_TRAIT_HAS_TYPE(has_value_type, value_type)

// has_key_type
//   trait: detects a nested `key_type` typedef (associative containers and any
// keyed entry).
D_TYPE_TRAIT_HAS_TYPE(has_key_type, key_type)

// has_mapped_type
//   trait: detects a nested `mapped_type` typedef (map-like associative
// containers).
D_TYPE_TRAIT_HAS_TYPE(has_mapped_type, mapped_type)

// has_size_type
//   trait: detects a nested `size_type` typedef.
D_TYPE_TRAIT_HAS_TYPE(has_size_type, size_type)

// has_difference_type
//   trait: detects a nested `difference_type` typedef.
D_TYPE_TRAIT_HAS_TYPE(has_difference_type, difference_type)

// has_allocator_type
//   trait: detects a nested `allocator_type` typedef (dynamic-storage
// containers).
D_TYPE_TRAIT_HAS_TYPE(has_allocator_type, allocator_type)


// ===========================================================================
// II.  extract-a-typedef-or-fall-back
// ===========================================================================

NS_INTERNAL

    // pick_member_type
    //   trait: chooses between an extracted member type and a fallback, given a
    // detector value.  Primary template (member absent): yields the fallback.
    //   Retained for callers that compose a detector and an extracted type by
    // hand; the D_TYPE_TRAIT_MEMBER_TYPE_OR macro is the usual front door.
    template<bool     _Present,
             typename _Extracted,
             typename _Fallback>
    struct pick_member_type
    {
        using type = _Fallback;
    };

    // pick_member_type (present)
    //   trait: yields the extracted member type when the detector reported
    // presence.
    template<typename _Extracted,
             typename _Fallback>
    struct pick_member_type<true, _Extracted, _Fallback>
    {
        using type = _Extracted;
    };

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_META_MEMBER_TYPES_
