/******************************************************************************
* djinterp [test]                                  option_set_compare_tests.hpp
*
*   Declarations, fixtures, and section block-providers for the
* option_set_compare.hpp unit suite.
*
*   The header is a set of compile-time comparison traits over option_set<>,
* entirely type-level and available at the framework's base standard (fold
* expressions and inline-variable shorthands; no concepts, no value face), so
* the suite is UNCONDITIONAL - every test compiles and runs at both C++17 and
* C++20 with no version gating.
*
*   NOTE ON THE CORRECTED HEADER:
*   The shipped header's Section V ("extractors": extract_actual / extract_default
* / extract_effective) referenced the retired actual<> / default_ option-carrier
* vocabulary (option_actual_tag_t / option_default_tag_t / actual<> / default_),
* which option_override.hpp documents as "no longer defined anywhere in the
* option layer".  That section made the whole header fail to compile.  The
* corrected header removes Section V and drops the now-dangling
* `= extract_actual` default on option_set_value_eq (so _Extract is a REQUIRED
* parameter).  The value-equality trait is still fully covered here: it is
* extractor-parameterized by design, so the suite exercises it through a
* self-contained custom extractor (extract_cmp_val below) over a local value
* carrier - no retired vocabulary required.
*
*   VERIFICATION MODEL:
*   Every facet is a compile-time trait, pinned with a `static_assert` on a
* `constexpr bool` that is then returned for the runner to record (the assert is
* the real check; the return drives instantiation and yields a verdict).
*
*   LAYOUT:
*     - one .hpp (this file): declarations + fixtures + block-providers,
*     - one .cpp per section group of option_set_compare.hpp:
*         option_set_compare_tests_keylist.cpp   (I + II. key lists + operations)
*         option_set_compare_tests_congruity.cpp (III.   key / type congruity)
*         option_set_compare_tests_carriers.cpp  (IV.    value carriers + carrier_eq)
*         option_set_compare_tests_value_eq.cpp  (V.     option_set_value_eq)
*     - one runner (option_set_compare_tests_runner.cpp) with main().
*
*   All unit tests live FLAT in namespace djinterp::testing.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES              (key enum / expander / value carrier + extractor)
* II.   TEST DECLARATIONS     (per section, flat in djinterp::testing)
* III.  BLOCK PROVIDERS        (one block_spec per section group)
*
*
* path:      /tests/djinterp/core/option/option_set_compare_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_OPTION_SET_COMPARE_TESTS_
#define DJINTERP_OPTION_SET_COMPARE_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp (resolved via the project include path)
#include "djinterp/test/test_defaults.hpp"             // module_spec / block_spec / test_spec, run_module
#include "djinterp/core/option/option_set_compare.hpp" // key_list, congruity, value carriers, value_eq


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   FIXTURES
// ===========================================================================
//   One scoped key enum for the members of any one set (option_set enforces a
// uniform key_type, and the congruity traits compare within that type).  The
// expander drives option_set's flat view so the "flat-view aware" claim can be
// checked.  cmp_val is a local stand-in for a compile-time value carried in an
// option's args, and extract_cmp_val is a self-contained extractor over it -
// this is what exercises option_set_value_eq without the retired actual<> /
// default_ carriers (see the header note above).

enum class cmp_key { a, b, c, d };

// multi-expander: contributes options for keys a and b through ::expanded_t
struct cmp_expander_ab
{
    using expanded_t = std::tuple<option<cmp_key::a>, option<cmp_key::b>>;
};

// cmp_val
//   a local compile-time value carrier, meant to sit in an option's args as
// option<K, cmp_val<V>>.  Stands in for whatever value carrier a real caller
// would use now that actual<> / default_ are gone.
template<auto _V>
struct cmp_val
{};

// extract_cmp_val
//   a self-contained unary extractor in the {value_absent | value_present<V>}
// interface: value_present<V> when the option leads with cmp_val<V>, else
// value_absent.  The primary handles every other operand - crucially including
// lookup_not_found, which option_set_value_eq feeds it on a key miss.
template<typename _Opt>
struct extract_cmp_val
{
    using type = value_absent;
};

template<auto _Key,
         auto _V,
         typename... _Rest>
struct extract_cmp_val<option<_Key, cmp_val<_V>, _Rest...>>
{
    using type = value_present<_V>;
};


// ===========================================================================
// II.  TEST DECLARATIONS
// ===========================================================================

// -- I + II.  key lists + operations --
bool keylist_key_list_size();
bool keylist_option_set_keys();
bool keylist_value_in_pack();
bool keylist_subset();
bool keylist_equal();

// -- III.  congruity --
bool congruity_key();
bool congruity_type();
bool congruity_type_stronger_than_key();

// -- IV.  value carriers --
bool carriers_value_absent();
bool carriers_value_present();
bool carriers_carrier_eq();

// -- V.  option_set_value_eq --
bool value_eq_equal();
bool value_eq_value_differs();
bool value_eq_keys_differ();
bool value_eq_absent_carrier();
bool value_eq_empty();


// ===========================================================================
// III. BLOCK PROVIDERS
// ===========================================================================

::djinterp::test::block_spec option_set_compare_keylist_block();
::djinterp::test::block_spec option_set_compare_congruity_block();
::djinterp::test::block_spec option_set_compare_carriers_block();
::djinterp::test::block_spec option_set_compare_value_eq_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_COMPARE_TESTS_
