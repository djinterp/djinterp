/******************************************************************************
* djinterp [meta]                                            member_traits.hpp
*
*   DEPRECATED - TRANSITIONAL SHIM.  The contents of this header moved:
*     - the nested-typedef detection macro and the extract-or-fallback macro
*       are now the canonical D_TYPE_TRAIT_HAS_TYPE / D_TYPE_TRAIT_MEMBER_TYPE_OR
*       in trait_detect.hpp;
*     - the concrete shared detectors (has_input_type, has_result_type,
*       has_item_type, has_value_type, has_kind_type, ...) and the
*       pick_member_type helper are now in member_types.hpp.
*
*   This file remains ONLY so that existing includers (parser_traits.hpp,
* scanner_traits.hpp, token_traits.hpp, ...) keep compiling while they are
* repointed.  It pulls in the new headers and restores the two old macro
* SPELLINGS as one-line forwards to their canonical replacements.  These
* forwards are deliberately confined to this deprecated file - the canonical
* headers carry no aliases.
*
*   MIGRATION (then delete this file):
*     #include ".../member_traits.hpp"   ->   #include ".../member_types.hpp"
*     D_DEFINE_HAS_MEMBER_TYPE(name)     ->   D_TYPE_TRAIT_HAS_TYPE(has_name, name)
*     D_DEFINE_MEMBER_TYPE_OR(t, m, f)   ->   D_TYPE_TRAIT_MEMBER_TYPE_OR(t, m, f)
*
*
* path:      /inc/djinterp/core/meta/member_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.29
******************************************************************************/

#ifndef DJINTERP_META_MEMBER_TRAITS_
#define DJINTERP_META_MEMBER_TRAITS_ 1

// djinterp
#include "../djinterp.hpp"
#include "./trait_detect.hpp"   // canonical macro homes
#include "./member_types.hpp"   // concrete detectors + pick_member_type


// D_DEFINE_HAS_MEMBER_TYPE  (deprecated)
//   macro: forwards to the canonical D_TYPE_TRAIT_HAS_TYPE, auto-deriving the
// trait name `has_<NAME>` as the old macro did.
#define D_DEFINE_HAS_MEMBER_TYPE(NAME)                                        \
    D_TYPE_TRAIT_HAS_TYPE(has_##NAME, NAME)

// D_DEFINE_MEMBER_TYPE_OR  (deprecated)
//   macro: forwards verbatim to the canonical D_TYPE_TRAIT_MEMBER_TYPE_OR.
#define D_DEFINE_MEMBER_TYPE_OR(TRAIT, MEMBER, FALLBACK)                      \
    D_TYPE_TRAIT_MEMBER_TYPE_OR(TRAIT, MEMBER, FALLBACK)


#endif  // DJINTERP_META_MEMBER_TRAITS_
