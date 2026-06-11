/******************************************************************************
* djinterp [options]                                         option_factory.hpp
*
*   Factories for assembling option<> values - both at the type level
* (`make_option_t`) and at the value level (`make_option()`).  Plus
* `key_carrier<>`, the minimal NTTP-to-type bridge needed to introduce
* a key into a typename pack (e.g. for the partition wrapper in
* option_builder.hpp).
*
*   Nothing in this header imposes meaning on an option's args.  The
* factory exists for three reasons:
*
*     1. Flat schemas need a way to name keys via a type, since the
*        partition wrapper signature (`template<typename...>`) is
*        typename-only.  `key_carrier<auto>` is the framework's
*        canonical bridge; any user-defined type with the same shape
*        (static constexpr `::value` member) works just as well.
*
*     2. Make-this-option call sites read consistently with the rest
*        of the framework when the built-in option<> is the type.
*
*     3. Custom option types - any user type satisfying the option
*        contract from option.hpp - may follow the same
*        naming convention (`make_my_option_t` / `make_my_option()`)
*        without any per-type framework scaffolding.  No central
*        customization point is required, because no central caller
*        needs polymorphism over option types.
*
*
* path:      /inc/djinterp/core/option/option_factory.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.27
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    key_carrier                 (NTTP-to-type bridge)
II.   make_option_t               (compile-time factory)
III.  make_option                 (runtime factory)
*/

#ifndef DJINTERP_OPTION_FACTORY_
#define DJINTERP_OPTION_FACTORY_ 1

// djinterp
#include "../djinterp.hpp"
#include "./option.hpp"


NS_DJINTERP


// ===========================================================================
// I.   key_carrier
// ===========================================================================

// key_carrier
//   type: minimal NTTP-to-type bridge.  Exposes `::value` (the
// carried key) and `::value_type` (the key's type).
//
//   This is the ONLY framework-provided helper that bridges
// "key as NTTP" and "key as typename slot".  It is NOT an
// option tag - it carries no relationship to the args that
// follow the key in an option<>, and the framework never
// inspects what an option carries beyond the key.  Any user-
// defined type exposing the same `static constexpr ::value`
// shape is interchangeable with key_carrier at every call site
// the framework controls.
//
// Usage:
//   key_carrier<my_enum::foo>   // typename slot in a flat schema
template<auto _Value>
struct key_carrier
{
    using value_type = decltype(_Value);

    static D_CONSTEXPR value_type value = _Value;
};


// ===========================================================================
// II.  make_option_t
// ===========================================================================

// make_option_t
//   alias: compile-time option factory.  Yields
// option<_Key, _Args...>.  Stylistic counterpart to the
// runtime factory below; the args are taken verbatim and
// passed straight through to the option<> template.
//
//   The framework imposes no shape on _Args.  See option.hpp
// for the option<> type itself.
template<auto        _Key,
         typename... _Args>
using make_option_t = option<_Key, _Args...>;


// ===========================================================================
// III. make_option
// ===========================================================================

// make_option
//   function: runtime option factory.  Returns a value-
// initialized option<_Key, _Args...>.
//
//   The built-in option<> is stateless, so this is mostly a
// stylistic shorthand.  The function exists primarily as a
// naming-convention anchor: custom option types that DO
// carry state (e.g. a hypothetical valued_option) may ship
// their own `make_X<...>(value)` factory using the same
// convention, and call sites read uniformly across option
// types.
//
// Usage:
//   constexpr auto opt = make_option<my_enum::foo, slot_a, slot_b>();
template<auto        _Key,
         typename... _Args>
D_CONSTEXPR option<_Key, _Args...>
make_option() noexcept
{
    return option<_Key, _Args...>{};
}


NS_END  // djinterp


#endif  // DJINTERP_OPTION_FACTORY_
