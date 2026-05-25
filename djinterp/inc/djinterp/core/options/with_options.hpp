/******************************************************************************
* djinterp [options]                                          with_options.hpp
*
*   Generic options mixin.  Equips any host type with a uniform compile-
* time option-query interface over a normalized option_list.  The host
* may be a container, an algorithm functor, a serialization config, a
* CLI parser, or anything else that wants compile-time configuration -
* this mixin makes no assumption about what the host is.
*   Two forms are provided:
*     with_options<_OptionList>           - takes a pre-normalized list.
*     with_options_pack<_Options...>      - takes raw user arguments and
*                                           normalizes internally.
*
*   The pack form is the convenience host classes use 99% of the time.
* The list form exists for cases where the host wants to compose,
* manipulate, or inspect the list before equipping itself.
*   The mixin is stateless.  It contributes:
*     - public type alias ::options_type
*     - public static constexpr ::option_count
*     - public static constexpr template ::has_option_v<_Key>
*     - public template ::option_t<_Key, _Default = void>
*
*   No virtual methods, no runtime members, no hidden allocations.
* Inherit publicly to surface the interface to consumers, or privately
* to keep it internal.
*
* DEPENDENCIES:
*   djinterp.hpp        - namespaces
*   options.hpp         - option, option_list
*   option_traits.hpp   - normalize_options_t, option_list_contains,
*                         option_list_lookup
*
*
* path:      /inc/djinterp/core/options/with_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    with_options
II.   with_options_pack
*/

#ifndef DJINTERP_OPTIONS_WITH_
#define DJINTERP_OPTIONS_WITH_ 1

// std
#include <cstddef>
// djinterp
#include "../djinterp.hpp"
#include "./options.hpp"
#include "./option_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   with_options
// ===========================================================================

// with_options
//   mixin: equips any host type with a uniform compile-time
// option-query interface over a normalized option_list.
//
//   Inherit (publicly or privately) to gain the surface.  No
// runtime cost - all members are type aliases or static
// constexpr.
//
// Example (generic, non-container host):
//   template<typename... _Options>
//   class my_serializer
//       : public with_options<normalize_options_t<_Options...>>
//   {
//       using base = with_options<normalize_options_t<_Options...>>;
//
//   public:
//       using endian_t =
//           typename base::template option_t<endian_key,
//                                            big_endian_tag>;
//
//       static constexpr bool checks_alignment =
//           base::template has_option_v<alignment_key>;
//   };
template<typename _OptionList>
class with_options
{
    static_assert(is_option_list<_OptionList>::value,
                  "Template parameter `_OptionList` must be a "
                  "normalized option_list<...>.  Use "
                  "with_options_pack<_Options...> if you want "
                  "automatic normalization.");

public:
    // options_type
    //   type: the normalized option_list this host was equipped
    // with.  Surfaced so downstream introspection can walk the
    // list directly.
    using options_type = _OptionList;

    // option_count
    //   value: number of normalized option entries.
    static constexpr std::size_t option_count = _OptionList::size;

    // has_option_v
    //   value: true iff _Key is present in options_type.
    template<typename _Key>
    static constexpr bool has_option_v =
        option_list_contains<_OptionList, _Key>::value;

    // option_t
    //   type: the value_type associated with _Key in
    // options_type, or _Default if _Key is absent.
    template<typename _Key,
             typename _Default = void>
    using option_t =
        option_list_lookup_t<_OptionList, _Key, _Default>;
};


// ===========================================================================
// II.  with_options_pack
// ===========================================================================

// with_options_pack
//   mixin: convenience form of with_options that accepts a raw
// user pack and normalizes it internally.  This is the form
// host classes use in their template signatures.
//
// Example:
//   template<typename    _Type,
//            typename    _Allocator = std::allocator<_Type>,
//            typename... _Options>
//   class my_container
//       : public with_options_pack<_Options...>
//   {
//       // ::options_type, ::has_option_v<>, ::option_t<>
//       // automatically available
//   };
//
// Equivalent to:
//   with_options<normalize_options_t<_Options...>>
template<typename... _Options>
using with_options_pack = with_options<normalize_options_t<_Options...>>;


NS_END  // djinterp


#endif  // DJINTERP_OPTIONS_WITH_