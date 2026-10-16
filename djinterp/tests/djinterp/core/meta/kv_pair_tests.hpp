/*******************************************************************************
* djinterp [testing]                                           kv_pair_tests.hpp
*
* Unit test suite for kv_pair.hpp -- the minimal key-value pair.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_KV_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       kv_pair_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  kv_pair<K, V> is a small aggregate-like value type with
* two public members (m_key, m_value), the nested typedefs key_type / value_type,
* four constructors (default, const-ref key+value, rvalue key+value), the three
* comparison operators, and the free make_kv factory.  It satisfies the
* is_option_entry structural contract (a key and a value).
*
*   THE ONE SEMANTIC THAT MATTERS.  Comparison is KEY-ONLY.  operator==,
* operator!= and operator< all consult m_key alone and ignore m_value entirely.
* So two pairs with the same key but different values compare EQUAL, and a
* sorted option_set orders purely by key.  This is the property most worth
* pinning, and the fixtures below are built to make "same key, different value"
* and "different key, same value" both expressible.
*
*   CONSTEXPR.  The value/rvalue constructors and all three operators are
* constexpr, so a kv_pair can live in a constant expression; make_kv is
* D_CONSTEXPR (constexpr under the normal build).  The tests assert several of
* these facts at compile time with static_assert in addition to at runtime.
*
*   THE FIXTURE VOCABULARY (normal mode only)
*     ii              a shorthand for kv_pair<int, int>, the common case.
*     tracked         a move-observing value type (counts how many times it was
*                     moved-constructed) so the rvalue constructor can be shown
*                     to actually MOVE its operands rather than copy them.
*     Key literals are just ints; the "same key / different value" and "different
*     key / same value" pairs are formed inline in each body.
*
*   BUILD PREREQUISITE: none.  kv_pair.hpp compiles as shipped.
*
* CONTENTS
*   I.    CONSTRUCTION   (kv_pair_tests_construction.cpp)
*   II.   COMPARISON     (kv_pair_tests_comparison.cpp)
*
*
* path:      /tests/djinterp/core/meta/kv_pair_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_KV_PAIR_TESTS_
#define DJINTERP_TESTS_KV_PAIR_TESTS_ 1

// std
#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/meta/kv_pair.hpp"                          // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// kv_pair_check
//   function: routes one D_KV_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
kv_pair_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_KV_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `KV` suffix is
// this suite's unique two letters.
#define D_KV_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::kv_pair_check(                              \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   SHORTHANDS                                            ///
///////////////////////////////////////////////////////////////////////////////

// ii
//   alias: the common kv_pair<int, int>, so a body reads ii(1, 2) rather than
// restating the type each time.
using ii = kv_pair<int, int>;


///////////////////////////////////////////////////////////////////////////////
///             F.2   MOVE-OBSERVING VALUE                                  ///
///////////////////////////////////////////////////////////////////////////////

// tracked
//   struct: a value type that counts move-constructions in a member, so the
// rvalue kv_pair constructor can be shown to MOVE its operands.  Comparable by
// its tag so it can serve as a key.
struct tracked
{
    int  tag   = 0;
    int  moves = 0;

    tracked() = default;
    explicit tracked(int _t) : tag(_t), moves(0) {}

    tracked(const tracked& _o) : tag(_o.tag), moves(_o.moves) {}
    tracked(tracked&& _o) noexcept : tag(_o.tag), moves(_o.moves + 1) {}

    tracked& operator=(const tracked&) = default;
    tracked& operator=(tracked&&) noexcept = default;

    bool operator==(const tracked& _o) const { return tag == _o.tag; }
    bool operator!=(const tracked& _o) const { return tag != _o.tag; }
    bool operator<(const tracked& _o) const  { return tag < _o.tag; }
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   CONSTRUCTION   (kv_pair_tests_construction.cpp)
bool tests_construction_member_typedefs();
bool tests_construction_value_constructor_sets_members();
bool tests_construction_default_constructor();
bool tests_construction_rvalue_constructor_moves();
bool tests_construction_make_kv_deduces_decayed_types();
bool tests_construction_is_constexpr_constructible();

// II.  COMPARISON   (kv_pair_tests_comparison.cpp)
bool tests_comparison_equality_is_key_only();
bool tests_comparison_inequality_is_key_only();
bool tests_comparison_ordering_is_by_key();
bool tests_comparison_value_is_ignored_entirely();
bool tests_comparison_operators_are_constexpr();
bool tests_comparison_with_string_keys();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// kv_pair_spec
//   function: the authoritative description of this suite.
inline dt::module_spec
kv_pair_spec()
{
    return dt::module_spec{
        "kv_pair",
        "The minimal key-value pair kv_pair<K, V>: two public members, the "
        "key_type / value_type typedefs, four constructors (default, const-ref, "
        "rvalue), three comparison operators, and the make_kv factory. The one "
        "semantic that matters is that comparison is KEY-ONLY -- operator==, != "
        "and < all consult the key and ignore the value, so equal-keyed pairs "
        "compare equal and a sorted option_set orders by key alone. The "
        "constructors and operators are constexpr. This suite pins construction "
        "and member layout, make_kv's decayed-type deduction, the move behaviour "
        "of the rvalue constructor, and -- centrally -- the key-only comparison "
        "across value and key variation.",
        {
            dt::block_spec{
                "construction",
                "Building a kv_pair and its type surface: the nested typedefs, "
                "the value and default constructors, the moving rvalue "
                "constructor, make_kv's decayed deduction, and constexpr "
                "constructibility.",
                {
                    { "tests_construction_member_typedefs",
                      "kv_pair<K, V> exposes key_type = K and value_type = V.",
                      &tests_construction_member_typedefs },
                    { "tests_construction_value_constructor_sets_members",
                      "The const-ref constructor stores the key and value in "
                      "m_key and m_value.",
                      &tests_construction_value_constructor_sets_members },
                    { "tests_construction_default_constructor",
                      "The default constructor yields a pair whose members are "
                      "then assignable and read back.",
                      &tests_construction_default_constructor },
                    { "tests_construction_rvalue_constructor_moves",
                      "The rvalue constructor MOVES its operands rather than "
                      "copying them, observed through a move-counting value "
                      "type.",
                      &tests_construction_rvalue_constructor_moves },
                    { "tests_construction_make_kv_deduces_decayed_types",
                      "make_kv deduces decayed key and value types -- a string "
                      "literal and a const int& become std::string and int -- "
                      "and builds the corresponding kv_pair.",
                      &tests_construction_make_kv_deduces_decayed_types },
                    { "tests_construction_is_constexpr_constructible",
                      "A kv_pair<int, int> can be built and read in a constant "
                      "expression.",
                      &tests_construction_is_constexpr_constructible },
                }
            },
            dt::block_spec{
                "comparison",
                "The key-only comparison semantics: equality, inequality and "
                "ordering all consult the key alone, the value is ignored "
                "entirely, the operators are constexpr, and the behaviour holds "
                "for non-trivial key types.",
                {
                    { "tests_comparison_equality_is_key_only",
                      "Two pairs with the same key compare equal even when their "
                      "values differ; pairs with different keys do not.",
                      &tests_comparison_equality_is_key_only },
                    { "tests_comparison_inequality_is_key_only",
                      "operator!= is the negation of the key-only equality: true "
                      "iff the keys differ, regardless of the values.",
                      &tests_comparison_inequality_is_key_only },
                    { "tests_comparison_ordering_is_by_key",
                      "operator< orders by key: a smaller key is less regardless "
                      "of values, and equal keys are not less than one another.",
                      &tests_comparison_ordering_is_by_key },
                    { "tests_comparison_value_is_ignored_entirely",
                      "Holding the key fixed and varying only the value changes "
                      "no comparison result -- ==, != and < are all invariant in "
                      "the value.",
                      &tests_comparison_value_is_ignored_entirely },
                    { "tests_comparison_operators_are_constexpr",
                      "==, != and < are usable in constant expressions.",
                      &tests_comparison_operators_are_constexpr },
                    { "tests_comparison_with_string_keys",
                      "The key-only comparison works for a non-trivial key type "
                      "(std::string): equal string keys compare equal despite "
                      "differing values, and ordering follows the string key.",
                      &tests_comparison_with_string_keys },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_KV_PAIR_TESTS_
