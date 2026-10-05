/*******************************************************************************
* djinterp [core]                                              array_options.hpp
*
*   Convenience facade for constructing `array<...>` instances from a
* template option pack.  Three things live here:
*
*     1. The canonical-form option aliases (`array_opt_type<T>`,
*        `array_opt_extent<N>`).  These wrap the array-specific keys
*        defined in array.hpp into fully-formed `option<...>` entries
*        so users do not have to spell out `std::integral_constant`
*        wrappers.
*     2. The trait `array_from_options<...>` and its `_t` alias,
*        which expose the resolved `array<...>` instantiation as a
*        public type.  Since `array<>` is itself an options-pack
*        primary template, the trait is essentially an identity -
*        but it remains useful as a documentation surface and as a
*        place for future helper logic.
*     3. The factory function `make_array_from_options(...)`, which
*        constructs an array and infers extent from the argument
*        count if no `array_extent_key` is present in the pack.
*
*   Everything in this header is a thin wrapper over machinery in
* `array.hpp` and `container_options.hpp`; both must be included
* (they are pulled in transitively).  No new resolver lives here -
* axis resolution is owned by `internal::array_axes_resolver` in
* array.hpp.
*
* DEPENDENCIES:
*   djinterp.hpp          - namespace macros, D_CONSTEXPR
*   options/option.hpp    - option<...>
*   container_options.hpp - container_lifetime,
*                           container_iterability, keys,
*                           container_opt_* aliases
*   array.hpp             - array<...>, array_type_key,
*                           array_extent_key,
*                           internal::array_axes_resolver
*
*
* path:      /inc/djinterp/core/container/array/array_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.05
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    canonical-form option aliases (array-specific)
      ----------------------------------------------

2.    array_from_options trait
      ------------------------

3.    make_array_from_options factory
      -------------------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_ARRAY_OPTIONS_HPP
#define DJINTERP_CONTAINER_ARRAY_ARRAY_OPTIONS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../option/option.hpp"
#include "../container_options.hpp"
#include "./array.hpp"


NS_DJINTERP

// ===========================================================================
// 1.   canonical-form option aliases (array-specific)
// ===========================================================================
// Convenience aliases that wrap an array-specific axis value into a
// fully-formed `option<key, value>`.  For the universal axes (lifetime
// and iterability) use `container_opt_lifetime<>` and
// `container_opt_iterability<>` from container_options.hpp.

// array_opt_type
//   alias: `option<array_type_key, Type>` for the element type axis.
template<typename Type>
using array_opt_type = option<array_type_key, Type>;

// array_opt_extent
//   alias: `option<array_extent_key, integral_constant<size_t, N>>` for the
// static-extent axis.
template<std::size_t N>
using array_opt_extent = option<array_extent_key,
                                std::integral_constant<std::size_t, N>>;


// ===========================================================================
// 2.   array_from_options trait
// ===========================================================================

// array_from_options
//   trait: exposes the `array<...>` instantiation produced by a given options
// pack as `::type`. Since `array<>` is itself options-pack-shaped, this trait
// is almost an identity - but it is useful as a self-documenting alias and as
// an extension point for future post-processing (e.g. sanity checks,
// normalization of the pack).
template<typename... Options>
struct array_from_options
{
    using type = array<Options...>;
};

// array_from_options_t
//   type: convenience alias for array_from_options<...>::type.
template<typename... Options>
using array_from_options_t = typename array_from_options<Options...>::type;


// ===========================================================================
// 3.   make_array_from_options factory
// ===========================================================================

// make_array_from_options
//   factory: constructs an `array<...>` from a template option pack and a
// runtime element-initializer pack. When `array_extent_key` is absent from
// `Options...`, the factory injects an extent equal to `sizeof...(Args)` so
// that callers can omit extent altogether and let the argument count drive it
// - mirroring the deduction `make_array` already performs.
//
// Implementation note: the function builds a fresh axes resolver
// with the appropriate fallback extent, looks up which extent would actually
// apply, and instantiates `array<...>` with an `array_opt_extent<>` injection
// if needed. When the pack already contains an extent option, the user's value
// wins and the injected option is shadowed by `option_list_lookup_t`'s
// first-match rule. Example (extent deduced from args):
//   auto a = make_array_from_options<array_opt_type<int>>(1, 2, 3);
//   // a is array<array_opt_type<int>, ...> with extent 3 Example (extent
// supplied explicitly):
//   auto b = make_array_from_options<
//                array_opt_type<int>,
//                array_opt_extent<8>,
//                container_opt_lifetime<container_lifetime::immutable>>();
//   // b is the immutable, extent-8 array
template<typename... Options,
         typename... Args>
D_CONSTEXPR
array<Options...,
      array_opt_extent<sizeof...(Args)>>
make_array_from_options(
    Args&&... _args
)
{
    // Append an extent option carrying sizeof...(Args). If the
    // user already supplied an extent, theirs comes first in the pack and
    // `option_list_lookup_t` will return it; the trailing injection becomes
    // unreachable.
    return array<Options...,
                 array_opt_extent<sizeof...(Args)>>(
        std::forward<Args>(_args)...);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_ARRAY_OPTIONS_HPP
