// djinterp [test]  semigroup_tests_mappend.cpp
//   Section II -- mappend, the associative combine.

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "semigroup_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_mappend_dispatch
  mappend routes to each instance's combine.
  Tests the following:
  - addition, concatenation, max, and left-projection each behave as defined
*/
bool
tests_mappend_dispatch()
{
    bool ok = true;

    ok = ok && (mappend(sg_sum{2}, sg_sum{3}).v == 5);
    ok = ok && (mappend(sg_string{"a"}, sg_string{"b"}).s == std::string("ab"));
    ok = ok && (mappend(sg_max{3}, sg_max{7}).v == 7);
    ok = ok && (mappend(sg_first{4}, sg_first{9}).v == 4);

    return ok;
}


/*
tests_mappend_associativity
  The defining law: mappend(a, mappend(b, c)) == mappend(mappend(a, b), c),
  checked across instances -- including the non-commutative ones, where the law
  is the substantive claim.
  Tests the following:
  - associativity holds for concatenation, addition, max, projection, modular
*/
bool
tests_mappend_associativity()
{
    bool ok = true;

    // concatenation.
    {
        sg_string a{"x"}, b{"y"}, c{"z"};
        ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));
    }
    // addition.
    {
        sg_sum a{2}, b{3}, c{4};
        ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));
    }
    // max.
    {
        sg_max a{3}, b{7}, c{5};
        ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));
    }
    // left projection.
    {
        sg_first a{1}, b{2}, c{3};
        ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));
    }
    // modular family.
    {
        z3 a{2}, b{1}, c{2};
        ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));
    }

    return ok;
}


/*
tests_mappend_order_preserved
  mappend passes its operands to combine in order -- so on a non-commutative
  instance the two orderings differ, and the left-to-right result is the one
  produced.
  Tests the following:
  - concatenation yields "ab", not "ba"
  - left projection returns the first operand, not the second
  - both are order-sensitive (a<>b != b<>a)
*/
bool
tests_mappend_order_preserved()
{
    bool ok = true;

    ok = ok && (mappend(sg_string{"a"}, sg_string{"b"}).s == std::string("ab"));
    ok = ok && (mappend(sg_string{"a"}, sg_string{"b"}).s !=
                mappend(sg_string{"b"}, sg_string{"a"}).s);

    ok = ok && (mappend(sg_first{4}, sg_first{9}).v == 4);
    ok = ok && (mappend(sg_first{4}, sg_first{9}).v !=
                mappend(sg_first{9}, sg_first{4}).v);

    return ok;
}


/*
tests_mappend_idempotent_max
  The max instance is idempotent: combining a value with itself returns it.
  Tests the following:
  - mappend(a, a) == a for several values
*/
bool
tests_mappend_idempotent_max()
{
    bool ok = true;

    ok = ok && (mappend(sg_max{5}, sg_max{5}).v == 5);
    ok = ok && (mappend(sg_max{0}, sg_max{0}).v == 0);
    ok = ok && (mappend(sg_max{-3}, sg_max{-3}).v == -3);

    return ok;
}


/*
tests_mappend_constexpr
  mappend folds inside a constant expression whenever the instance's combine is
  constexpr over a literal type -- the header's conditional-constexpr contract.
  Tests the following:
  - each constexpr instance combines within static_assert
*/
bool
tests_mappend_constexpr()
{
    static_assert(mappend(sg_sum{2}, sg_sum{3}) == sg_sum{5}, "sum");
    static_assert(mappend(sg_max{3}, sg_max{7}) == sg_max{7}, "max");
    static_assert(mappend(sg_first{4}, sg_first{9}) == sg_first{4}, "first");
    static_assert(mappend(z3{2}, z3{2}) == z3{1}, "z3");
    static_assert(mappend(z5{4}, z5{3}) == z5{2}, "z5");

    // a nested fold is also a single constant expression.
    static_assert(
        mappend(sg_sum{1}, mappend(sg_sum{2}, sg_sum{3})) == sg_sum{6},
        "nested sum");

    return true;
}


/*
tests_mappend_family_modular
  The family instance computes modular addition, with the modulus taken from the
  operand type, wrapping correctly.
  Tests the following:
  - Z/3Z and Z/5Z each wrap at their own modulus
  - the same specialisation serves both types
*/
bool
tests_mappend_family_modular()
{
    bool ok = true;

    ok = ok && (mappend(z3{2}, z3{2}).v == 1);   // 4 mod 3
    ok = ok && (mappend(z3{1}, z3{2}).v == 0);   // 3 mod 3
    ok = ok && (mappend(z3{0}, z3{1}).v == 1);   // 1 mod 3

    ok = ok && (mappend(z5{4}, z5{3}).v == 2);   // 7 mod 5
    ok = ok && (mappend(z5{2}, z5{2}).v == 4);   // 4 mod 5
    ok = ok && (mappend(z5{3}, z5{3}).v == 1);   // 6 mod 5

    return ok;
}


/*
tests_mappend_generic_thrice
  A function generic over the Semigroup protocol works for every instance.
  Tests the following:
  - x <> (x <> x) yields 3x, threefold concatenation, and (idempotent) max
*/
bool
tests_mappend_generic_thrice()
{
    bool ok = true;

    ok = ok && (thrice(sg_sum{4}).v == 12);
    ok = ok && (thrice(sg_string{"ab"}).s == std::string("ababab"));
    ok = ok && (thrice(sg_max{5}).v == 5);
    ok = ok && (thrice(z3{1}).v == 0);           // (1+1+1) mod 3

    return ok;
}


/*
tests_mappend_cvref_args
  mappend decays its operand type, so const and lvalue operands are accepted.
  Tests the following:
  - const lvalues combine
  - the same lvalue may be passed for both operands
  - const and non-const lvalues combine together
*/
bool
tests_mappend_cvref_args()
{
    bool ok = true;

    const sg_sum a{2};
    const sg_sum b{3};
    ok = ok && (mappend(a, b).v == 5);

    sg_sum c{1};
    ok = ok && (mappend(c, c).v == 2);
    ok = ok && (mappend(a, c).v == 3);

    return ok;
}


/*
tests_mappend_return_type
  mappend returns the semigroup type (the decltype of combine).
  Tests the following:
  - the result type matches the operand type for several instances
*/
bool
tests_mappend_return_type()
{
    static_assert(
        std::is_same<decltype(mappend(std::declval<sg_sum>(),
                                      std::declval<sg_sum>())),
                     sg_sum>::value, "sum result");
    static_assert(
        std::is_same<decltype(mappend(std::declval<sg_string>(),
                                      std::declval<sg_string>())),
                     sg_string>::value, "string result");
    static_assert(
        std::is_same<decltype(mappend(std::declval<z3>(),
                                      std::declval<z3>())),
                     z3>::value, "z3 result");

    return true;
}


/*
tests_mappend_requires_same_type
  mappend deduces one semigroup type from both operands, so mismatched operand
  types are not callable.
  Tests the following:
  - same-type calls are well-formed
  - mixed-type calls (even between two semigroups) are rejected
*/
bool
tests_mappend_requires_same_type()
{
    static_assert(can_mappend<sg_sum, sg_sum>::value, "sum/sum ok");
    static_assert(can_mappend<sg_string, sg_string>::value, "string/string ok");

    static_assert(!can_mappend<sg_sum, sg_max>::value, "sum/max rejected");
    static_assert(!can_mappend<z3, z5>::value, "z3/z5 rejected");
    static_assert(!can_mappend<sg_sum, int>::value, "sum/int rejected");

    return true;
}


/*
tests_mappend_runtime_domain
  mappend also runs at runtime -- the other half of the dual domain -- over
  operands whose values are not known at compile time.
  Tests the following:
  - a sum built from runtime ints combines correctly
  - string concatenation (inherently runtime) combines correctly
*/
bool
tests_mappend_runtime_domain()
{
    bool ok = true;

    int x = 2;
    int y = 3;
    sg_sum a{x};
    sg_sum b{y};
    ok = ok && (mappend(a, b).v == 5);

    std::string p = "foo";
    std::string q = "bar";
    ok = ok && (mappend(sg_string{p}, sg_string{q}).s == std::string("foobar"));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
