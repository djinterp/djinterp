/******************************************************************************
* djinterp [options]                                                option.hpp
*
*   A single compile-time option: one key plus zero or more "columns" of
* additional information.  The first template argument is the key - a
* VALUE, not a type.  Everything after it is a column; columns may be
* types (the value type, in the simple form) or wrapped values (defaults
* via value_v<>).  Later layers will introduce additional columns
* (CLI symbol, verification function, description, ...) without breaking
* the shapes defined here.
*
*   The key is an `auto` NTTP, so its TYPE is inferred via `decltype`.
* Writers don't have to spell it twice, and an option_set built from
* options whose keys all live in the same enum / scope automatically
* knows its `key_type`.
*
*   This header defines only the simplest forms:
*
*     option<_Key>                  - unary; just a key.
*     option<_Key, _Value>          - typed; value_type = _Value.
*     option<_Key, value_v<_Def>>   - typed with default; value_type is
*                                     inferred from the default value.
*
*   No instance data, no constructors, no methods.  These are pure shapes
* for the trait system to inspect.  Runtime carriers (option_set itself,
* if you choose to make it stateful later) layer on top.
*
*
* path:      /inc/djinterp/core/options/option.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    value_v
II.   option
      1. option<_Key>                  (unary)
      2. option<_Key, _Value>          (typed)
      3. option<_Key, value_v<_Def>>   (typed with default)
*/

#ifndef DJINTERP_OPTION_
#define DJINTERP_OPTION_ 1

// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
// I.   value_v
// ===========================================================================

// value_v
//   helper: wraps a non-type template argument (NTTP) in a
// type so it can travel through type-only parameter packs.
// The wrapped value's type is recovered via ::type; the value
// itself via ::value.
//
// Example:
//   using v = value_v<42>;
//   static_assert(v::value == 42, "");
//   // v::type is int
template<auto _Value>
struct value_v
{
    using type = decltype(_Value);

    static constexpr type value = _Value;
};


// ===========================================================================
// II.  option
// ===========================================================================

// option
//   trait: a single compile-time option.  First parameter is
// the key (a value); subsequent parameters are columns of
// additional information.  No instance data; the trait system
// inspects ::key, ::key_type, and the per-column accessors.
//
//   The primary template is intentionally left undefined - the
// usable forms are the partial specializations below.  This
// makes ill-formed shapes (e.g. accidentally passing a type
// where a key was meant) hard errors at instantiation rather
// than silent matches.
template<auto       _Key,
         typename... _Columns>
struct option;

// option<_Key>
//   trait: unary form.  Just a key, no columns.  Useful for
// presence-flagged options (a key whose mere presence in an
// option_set is the entire signal).
//
// Example:
//   option<foo_options::bar>
template<auto _Key>
struct option<_Key>
{
    using key_type = decltype(_Key);

    static constexpr key_type key = _Key;

    static constexpr bool has_value_type = false;
    static constexpr bool has_default    = false;
};

// option<_Key, _Value>
//   trait: typed form.  Key plus value type, no default
// value.  The second argument is the type the option carries
// at runtime (when carried at runtime at all).
//
// Example:
//   option<foo_options::foo, bool>
template<auto     _Key,
         typename _Value>
struct option<_Key, _Value>
{
    using key_type   = decltype(_Key);
    using value_type = _Value;

    static constexpr key_type key = _Key;

    static constexpr bool has_value_type = true;
    static constexpr bool has_default    = false;
};

// option<_Key, value_v<_Default>>
//   trait: typed form with a default value.  More specific
// than option<_Key, _Value>, so this partial specialization
// wins whenever the second argument is a value_v<...> wrapper.
// value_type is inferred from the wrapped value.
//
// Example:
//   option<foo_options::derp,      value_v<42>>
//   // value_type = int, default_value = 42
//
//   option<foo_options::something, value_v<some_enum::monkey>>
//   // value_type = some_enum, default_value = some_enum::monkey
template<auto _Key,
         auto _Default>
struct option<_Key, value_v<_Default>>
{
    using key_type   = decltype(_Key);
    using value_type = decltype(_Default);

    static constexpr key_type   key           = _Key;
    static constexpr value_type default_value = _Default;

    static constexpr bool has_value_type = true;
    static constexpr bool has_default    = true;
};


NS_END  // djinterp


#endif  // DJINTERP_OPTION_
