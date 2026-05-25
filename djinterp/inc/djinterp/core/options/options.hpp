/******************************************************************************
* djinterp [options]                                               options.hpp
*
*   Canonical type-level options.  Two types - `option<_Key, _Value>` and
* `option_list<_Options...>` - that form the internal lingua franca of
* the options machinery.  All wire formats (option_pair, option_set,
* bare key-value sequences, etc.) are normalized into
* option_list<option<...>...> by `normalize_options_t` in
* option_traits.hpp; nothing downstream sees anything else.
*   These types carry no values, no constructors, no methods.  They are
* purely shapes for the trait system to inspect.  The runtime equivalents
* (option_pair, option_set) live in their own headers and are unaffected
* by this layer.
*   This header has zero dependencies beyond <cstddef> and djinterp.hpp.
* It is the foundation of the options namespace and may be included by
* anything.
*
*
* path:      /inc/djinterp/core/options/options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
III.  runtime_options_carrier_key
IV.   option_list manipulation
      1. option_list_prepend
      2. option_list_append
      3. option_list_concat
*/

#ifndef DJINTERP_OPTIONS_
#define DJINTERP_OPTIONS_ 1

// std
#include <cstddef>
// djinterp
#include "../djinterp.hpp"
#include "../util/kv_pair.hpp"


NS_DJINTERP

// option
//   trait: a single compile-time key-value option.  _Key and
// _Value are types; runtime values that need to ride compile
// time are wrapped (std::integral_constant, std::bool_constant,
// or a user constexpr-friendly wrapper).
//
//   `option` carries no instance data.  It exists only so that
// the trait system has a single canonical shape for entries
// after normalization.
//
// Example:
//   using my_opt = option<verbose_key, std::true_type>;
//   static_assert(std::is_same<my_opt::key_type,
//                              verbose_key>::value, "");
template<typename    _Key,
         typename... _Value>
struct option;

template<typename _Key>
struct option<_Key>
{
    using key_type = _Key;
};

template<typename _Key,
         typename _Value>
struct option<_Key, _Value>
{
    using key_type      = _Key;
    using value_type    = _Value;
};

template<typename    _Key,
         typename    _Value,
         typename... _Metadata>
struct option<_Key, _Value, _Metadata...>
{
    using key_type      = _Key;
    using value_type    = _Value;
    using metadata_type = std::tuple<_Metadata...>;
};

// option_list
//   trait: canonical compile-time pack of option<...> entries.
// All wire formats normalize into this single shape.
//
//   `option_list` carries no instance data and imposes no
// invariants beyond the parameter pack itself.  Duplicate keys,
// ordering, and any other invariants are concerns for the
// query/merge traits in option_traits.hpp.
template<typename... _Options>
struct option_list
{
    static constexpr std::size_t size = sizeof...(_Options);
};


// ===========================================================================
// III. runtime_options_carrier_key
// ===========================================================================

// runtime_options_carrier_key
//   tag: framework-internal sentinel key used by the normalizer to record 
// set-like and container-of-entries forms passed in `_Options...` template 
// packs.
//   Such forms cannot have their entries enumerated at compile time - 
// the entries are runtime data, not type information.  When the normalizer 
// encounters one it emits a single canonical option:
//
//     option<runtime_options_carrier_key, _TheSetOrContainerType>
//
//   Hosts query option_t<runtime_options_carrier_key> to discover the user's
// chosen runtime carrier type.
//   This is the one and only sentinel the framework reserves; all other keys 
// belong to user code.
struct runtime_options_carrier_key
{};


// ===========================================================================
// IV.  option_list manipulation
// ===========================================================================

// option_list_prepend
//   trait: prepends _Option to an option_list.
template<typename _Option,
         typename _List>
struct option_list_prepend;

template<typename    _Option,
         typename... _Existing>
struct option_list_prepend<_Option, option_list<_Existing...>>
{
    using type = option_list<_Option, _Existing...>;
};

// option_list_prepend_t
//   type: convenience alias for option_list_prepend<...>::type.
template<typename _Option,
         typename _List>
using option_list_prepend_t = typename option_list_prepend<_Option, _List>::type;

// option_list_append
//   trait: appends _Option to an option_list.
template<typename _List,
         typename _Option>
struct option_list_append;

template<typename    _Option,
         typename... _Existing>
struct option_list_append<option_list<_Existing...>, _Option>
{
    using type = option_list<_Existing..., _Option>;
};

// option_list_append_t
//   type: convenience alias for option_list_append<...>::type.
template<typename _List,
         typename _Option>
using option_list_append_t = typename option_list_append<_List, _Option>::type;


// option_list_concat
//   trait: concatenates two option_lists.
template<typename _ListA,
         typename _ListB>
struct option_list_concat;

template<typename... _A,
         typename... _B>
struct option_list_concat<option_list<_A...>, option_list<_B...>>
{
    using type = option_list<_A..., _B...>;
};

// option_list_concat_t
//   type: convenience alias for option_list_concat<...>::type.
template<typename _ListA,
         typename _ListB>
using option_list_concat_t = typename option_list_concat<_ListA, _ListB>::type;


NS_END  // djinterp


#endif  // DJINTERP_OPTIONS_