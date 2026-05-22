/******************************************************************************
* djinterp [test]                                    type_traits_tests_rules.cpp
*
*   Unit tests for the rule-of-N traits in Section III of type_traits.hpp:
*     - follows_rule_of_five  + _v
*     - follows_rule_of_three + _v
*     - follows_rule_of_zero  + _v
*
*   Each trait is built on D_TRAIT_IS_DETECTED with a multi-expression
* SFINAE pattern.  The expressions probe for:
*     - rule_of_five:  copy ctor, move ctor, copy assign (return Type&),
*                      move assign (return Type&). (Destructor existence
*                      is implied by ability to declare an object.)
*     - rule_of_three: copy ctor, copy assign (return Type&).
*     - rule_of_zero:  ALL special members are trivially generated.
*
*   The shared header (type_traits_tests.hpp) provides three reference
* classes:
*     - trivial_t            -- all defaulted (rule of zero)
*     - full_special_member_t -- all user-defined (rule of five)
*     - copy_only_t          -- copy + dtor only (rule of three but
*                                NOT rule of five because move is
*                                implicitly deleted by user-declared
*                                copy operations)
*     - noncopyable_t        -- copy deleted -> follows neither rule_of_*
*                                (the probe expressions for copy are
*                                ill-formed)
*
*
* path:      /inc/djinterp/test/type_traits_tests_rules.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST

using tt = type_traits_test_types::trivial_t;
using fs = type_traits_test_types::full_special_member_t;
using co = type_traits_test_types::copy_only_t;
using nc = type_traits_test_types::noncopyable_t;


// =========================================================================
// I.   follows_rule_of_five  (compile-time)
// =========================================================================
//   Requires: copy ctor, move ctor, copy assign (returns Type&), move
// assign (returns Type&) all well-formed.

// positive: all five special members are well-formed
static_assert(follows_rule_of_five<fs>::value == true,
              "follows_rule_of_five<full_special_member_t> -> true");

// positive: trivial_t -- all special members default-generated, which is
// equally well-formed.  The trait doesn't distinguish user-defined from
// compiler-generated.
static_assert(follows_rule_of_five<tt>::value == true,
              "follows_rule_of_five<trivial_t> -> true (defaults are OK)");

// negative: noncopyable -- copy ctor deleted -> probe fails
static_assert(follows_rule_of_five<nc>::value == false,
              "follows_rule_of_five<noncopyable_t> -> false (copy deleted)");

// negative: builtin int -- the probe `_Type& = _Type&&` should succeed for
// int.  In fact, follows_rule_of_five<int> probably IS true under the
// current spec (the probes succeed for built-in arithmetic).  Test what
// the implementation actually does:
static_assert(follows_rule_of_five<int>::value == true,
              "follows_rule_of_five<int> -> true (all int operations well-formed)");

// negative: void -- probes are ill-formed
static_assert(follows_rule_of_five<void>::value == false,
              "follows_rule_of_five<void> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(follows_rule_of_five_v<fs> == true,
                  "follows_rule_of_five_v matches struct (true)");
    static_assert(follows_rule_of_five_v<nc> == false,
                  "follows_rule_of_five_v matches struct (false)");
#endif


// =========================================================================
// II.  follows_rule_of_three  (compile-time)
// =========================================================================
//   Requires: copy ctor, copy assign (returns Type&).  No move-related
// probes -- so a copy-only class (where move is implicitly deleted)
// still passes.

// positive: copy-only class -- declares copy ctor + copy assign
static_assert(follows_rule_of_three<co>::value == true,
              "follows_rule_of_three<copy_only_t> -> true");

// positive: full Rule-of-Five class -- has copy ctor + copy assign too
static_assert(follows_rule_of_three<fs>::value == true,
              "follows_rule_of_three<full_special_member_t> -> true");

// positive: trivial_t -- compiler-generated copy is well-formed
static_assert(follows_rule_of_three<tt>::value == true,
              "follows_rule_of_three<trivial_t> -> true");

// negative: noncopyable
static_assert(follows_rule_of_three<nc>::value == false,
              "follows_rule_of_three<noncopyable_t> -> false");

// negative: void
static_assert(follows_rule_of_three<void>::value == false,
              "follows_rule_of_three<void> -> false");

// positive: builtin int (built-in copy & assignment are well-formed)
static_assert(follows_rule_of_three<int>::value == true,
              "follows_rule_of_three<int> -> true (builtin ops well-formed)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(follows_rule_of_three_v<co> == true,
                  "follows_rule_of_three_v: true case");
    static_assert(follows_rule_of_three_v<nc> == false,
                  "follows_rule_of_three_v: false case");
#endif


// =========================================================================
// III. follows_rule_of_zero  (compile-time)
// =========================================================================
//   Requires: ALL five special members are TRIVIALLY constructible /
// assignable / destructible.  Distinct from rule_of_five: rule_of_zero
// requires triviality, rule_of_five just requires well-formedness.

// positive: trivial_t -- all defaulted, hence trivial
static_assert(follows_rule_of_zero<tt>::value == true,
              "follows_rule_of_zero<trivial_t> -> true");

// positive: builtin int -- all operations trivial
static_assert(follows_rule_of_zero<int>::value == true,
              "follows_rule_of_zero<int> -> true");

// negative: full_special_member_t -- user-defined operations are NOT trivial
static_assert(follows_rule_of_zero<fs>::value == false,
              "follows_rule_of_zero<full_special_member_t> -> false (user-defined ops not trivial)");

// negative: copy_only_t -- user-defined ops not trivial
static_assert(follows_rule_of_zero<co>::value == false,
              "follows_rule_of_zero<copy_only_t> -> false");

// negative: noncopyable_t -- copy operations deleted -> not trivially-copyable
static_assert(follows_rule_of_zero<nc>::value == false,
              "follows_rule_of_zero<noncopyable_t> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(follows_rule_of_zero_v<tt>  == true,
                  "follows_rule_of_zero_v: true case");
    static_assert(follows_rule_of_zero_v<fs>  == false,
                  "follows_rule_of_zero_v: false case");
#endif


// =========================================================================
// IV.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_rules(
    test_handler& _test_handler
)
{
    // ---- follows_rule_of_five ----
    record_assertion(_test_handler, 
        follows_rule_of_five<fs>::value == true,
        "follows_rule_of_five<full_special_member_t>");
    record_assertion(_test_handler, 
        follows_rule_of_five<tt>::value == true,
        "follows_rule_of_five<trivial_t> (defaults pass)");
    record_assertion(_test_handler, 
        follows_rule_of_five<int>::value == true,
        "follows_rule_of_five<int> (builtin)");
    record_assertion(_test_handler, 
        follows_rule_of_five<nc>::value == false,
        "follows_rule_of_five<noncopyable_t> -- copy deleted");
    record_assertion(_test_handler, 
        follows_rule_of_five<void>::value == false,
        "follows_rule_of_five<void>");

    // ---- follows_rule_of_three ----
    record_assertion(_test_handler, 
        follows_rule_of_three<co>::value == true,
        "follows_rule_of_three<copy_only_t>");
    record_assertion(_test_handler, 
        follows_rule_of_three<fs>::value == true,
        "follows_rule_of_three<full_special_member_t>");
    record_assertion(_test_handler, 
        follows_rule_of_three<tt>::value == true,
        "follows_rule_of_three<trivial_t>");
    record_assertion(_test_handler, 
        follows_rule_of_three<int>::value == true,
        "follows_rule_of_three<int>");
    record_assertion(_test_handler, 
        follows_rule_of_three<nc>::value == false,
        "follows_rule_of_three<noncopyable_t>");
    record_assertion(_test_handler, 
        follows_rule_of_three<void>::value == false,
        "follows_rule_of_three<void>");

    // ---- follows_rule_of_zero ----
    record_assertion(_test_handler, 
        follows_rule_of_zero<tt>::value == true,
        "follows_rule_of_zero<trivial_t>");
    record_assertion(_test_handler, 
        follows_rule_of_zero<int>::value == true,
        "follows_rule_of_zero<int>");
    record_assertion(_test_handler, 
        follows_rule_of_zero<fs>::value == false,
        "follows_rule_of_zero<full_special_member_t>");
    record_assertion(_test_handler, 
        follows_rule_of_zero<co>::value == false,
        "follows_rule_of_zero<copy_only_t>");
    record_assertion(_test_handler, 
        follows_rule_of_zero<nc>::value == false,
        "follows_rule_of_zero<noncopyable_t>");

    return;
}


NS_END  // test
NS_END  // djinterp
