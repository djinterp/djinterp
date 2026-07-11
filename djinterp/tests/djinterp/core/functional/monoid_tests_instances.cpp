// djinterp [test]  monoid_tests_instances.cpp
//   Section III -- INSTANCES (each type's combine + identity + monoid laws).

// std
#include <limits>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_string_instance
  Exercises the std::string instance (concatenation, identity "").
  Tests the following:
  - semigroup_traits<std::string>::combine is concatenation
  - monoid_traits<std::string>::empty is the empty string
  - left / right identity laws hold
  - associativity holds
  - the empty <> empty edge case
*/
bool
tests_string_instance()
{
    bool ok = true;

    const std::string a = "foo";
    const std::string b = "bar";
    const std::string c = "baz";
    const std::string e = monoid_traits<std::string>::empty();

    // combine is concatenation; empty is "".
    ok = ok && (semigroup_traits<std::string>::combine(a, b) == "foobar");
    ok = ok && (e.empty());
    ok = ok && (mappend(a, b) == "foobar");

    // identity laws: e <> x == x == x <> e.
    ok = ok && (mappend(e, a) == a);
    ok = ok && (mappend(a, e) == a);

    // associativity: (a <> b) <> c == a <> (b <> c).
    ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));

    // edge: both empty combine to empty.
    ok = ok && (mappend(std::string(), std::string()).empty());

    return ok;
}


/*
tests_vector_instance
  Exercises the std::vector<T> instance (concatenation, identity {}).
  Tests the following:
  - combine concatenates in order and preserves size
  - empty is the empty vector
  - left / right identity laws hold
  - associativity holds
  - the empty <> empty edge case
  - a non-trivial element type (std::vector<std::string>)
*/
bool
tests_vector_instance()
{
    bool ok = true;

    typedef std::vector<int> vi;

    const vi a = { 1, 2 };
    const vi b = { 3 };
    const vi c = { 4, 5 };
    const vi e = monoid_traits<vi>::empty();

    // combine is concatenation; empty is {}.
    ok = ok && (semigroup_traits<vi>::combine(a, b) == (vi{ 1, 2, 3 }));
    ok = ok && (e.empty());
    ok = ok && (mappend(a, c) == (vi{ 1, 2, 4, 5 }));

    // identity laws.
    ok = ok && (mappend(e, a) == a);
    ok = ok && (mappend(a, e) == a);

    // associativity.
    ok = ok && (mappend(mappend(a, b), c) == mappend(a, mappend(b, c)));

    // concatenation preserves size and order.
    const vi ab = mappend(a, b);
    ok = ok && (ab.size() == a.size() + b.size());

    // edge: empty <> empty == empty.
    ok = ok && (mappend(vi(), vi()).empty());

    // non-trivial element type.
    typedef std::vector<std::string> vs;
    const vs sa = { "x" };
    const vs sb = { "y", "z" };
    ok = ok && (mappend(sa, sb) == (vs{ "x", "y", "z" }));

    return ok;
}


/*
tests_sum_instance
  Exercises the sum<T> instance (addition, identity 0).
  Tests the following:
  - combine is +
  - empty is 0
  - left / right identity laws hold
  - associativity holds
  - negative summands
  - a floating element type
  - the algebra is a constant expression
*/
bool
tests_sum_instance()
{
    bool ok = true;

    const sum<int> a(2);
    const sum<int> b(3);
    const sum<int> c(5);
    const sum<int> e = monoid_traits<sum<int> >::empty();

    // combine is +; empty is 0.
    ok = ok && (semigroup_traits<sum<int> >::combine(a, b).value == 5);
    ok = ok && (e.value == 0);
    ok = ok && (mappend(a, b).value == 5);

    // identity laws: 0 + x == x == x + 0.
    ok = ok && (mappend(e, a).value == a.value);
    ok = ok && (mappend(a, e).value == a.value);

    // associativity.
    ok = ok && (mappend(mappend(a, b), c).value ==
                mappend(a, mappend(b, c)).value);

    // negative summands.
    ok = ok && (mappend(sum<int>(-4), sum<int>(10)).value == 6);

    // floating element type.
    ok = ok && close_enough(
                   mappend(sum<double>(1.5), sum<double>(2.25)).value, 3.75);
    ok = ok && close_enough(monoid_traits<sum<double> >::empty().value, 0.0);

    // constexpr algebra.
    static_assert(mappend(sum<int>(2), sum<int>(3)).value == 5, "sum ce");
    static_assert(mempty<sum<int> >().value == 0, "sum empty ce");

    return ok;
}


/*
tests_product_instance
  Exercises the product<T> instance (multiplication, identity 1).
  Tests the following:
  - combine is *
  - empty is 1
  - left / right identity laws hold
  - associativity holds
  - the absorbing element 0
  - a floating element type
  - the algebra is a constant expression
*/
bool
tests_product_instance()
{
    bool ok = true;

    const product<int> a(2);
    const product<int> b(3);
    const product<int> c(4);
    const product<int> e = monoid_traits<product<int> >::empty();

    // combine is *; empty is 1.
    ok = ok && (semigroup_traits<product<int> >::combine(a, b).value == 6);
    ok = ok && (e.value == 1);
    ok = ok && (mappend(a, b).value == 6);

    // identity laws: 1 * x == x == x * 1.
    ok = ok && (mappend(e, a).value == a.value);
    ok = ok && (mappend(a, e).value == a.value);

    // associativity.
    ok = ok && (mappend(mappend(a, b), c).value ==
                mappend(a, mappend(b, c)).value);

    // absorbing element: anything times 0 is 0.
    ok = ok && (mappend(a, product<int>(0)).value == 0);

    // floating element type.
    ok = ok && close_enough(
                   mappend(product<double>(2.0), product<double>(2.5)).value,
                   5.0);
    ok = ok && close_enough(monoid_traits<product<double> >::empty().value,
                            1.0);

    // constexpr algebra.
    static_assert(mappend(product<int>(3), product<int>(4)).value == 12,
                  "product ce");
    static_assert(mempty<product<int> >().value == 1, "product empty ce");

    return ok;
}


/*
tests_all_instance
  Exercises the all instance (logical AND, identity true).
  Tests the following:
  - combine is &&, verified over the full truth table
  - empty is true
  - identity law x && true == x (both directions)
  - associativity holds
  - the algebra is a constant expression
*/
bool
tests_all_instance()
{
    bool ok = true;

    const all e = monoid_traits<all>::empty();

    // empty is true.
    ok = ok && (e.value == true);

    // combine is &&: full truth table.
    ok = ok && (mappend(all(true),  all(true)).value  == true);
    ok = ok && (mappend(all(true),  all(false)).value == false);
    ok = ok && (mappend(all(false), all(true)).value  == false);
    ok = ok && (mappend(all(false), all(false)).value == false);

    // identity: x && true == x, both directions.
    ok = ok && (mappend(all(true),  e).value == true);
    ok = ok && (mappend(all(false), e).value == false);
    ok = ok && (mappend(e, all(false)).value == false);

    // associativity.
    ok = ok && (mappend(mappend(all(true), all(false)), all(true)).value ==
                mappend(all(true), mappend(all(false), all(true))).value);

    // constexpr.
    static_assert(mappend(all(true), all(false)).value == false, "all ce");
    static_assert(mempty<all>().value == true, "all empty ce");

    return ok;
}


/*
tests_any_instance
  Exercises the any instance (logical OR, identity false).
  Tests the following:
  - combine is ||, verified over the full truth table
  - empty is false
  - identity law x || false == x (both directions)
  - associativity holds
  - the algebra is a constant expression
*/
bool
tests_any_instance()
{
    bool ok = true;

    const any e = monoid_traits<any>::empty();

    // empty is false.
    ok = ok && (e.value == false);

    // combine is ||: full truth table.
    ok = ok && (mappend(any(true),  any(true)).value  == true);
    ok = ok && (mappend(any(true),  any(false)).value == true);
    ok = ok && (mappend(any(false), any(true)).value  == true);
    ok = ok && (mappend(any(false), any(false)).value == false);

    // identity: x || false == x, both directions.
    ok = ok && (mappend(any(true),  e).value == true);
    ok = ok && (mappend(any(false), e).value == false);
    ok = ok && (mappend(e, any(true)).value  == true);

    // associativity.
    ok = ok && (mappend(mappend(any(false), any(true)), any(false)).value ==
                mappend(any(false), mappend(any(true), any(false))).value);

    // constexpr.
    static_assert(mappend(any(false), any(true)).value == true, "any ce");
    static_assert(mempty<any>().value == false, "any empty ce");

    return ok;
}


/*
tests_min_instance
  Exercises the min<T> instance (minimum, identity numeric_limits::max()).
  Tests the following:
  - combine keeps the smaller operand
  - empty is the largest representable value
  - left / right identity laws hold, including the operand-equals-identity edge
  - a tie resolves to a stable value
  - associativity holds
  - negative operands and a floating element type
  - the algebra is a constant expression
*/
bool
tests_min_instance()
{
    bool ok = true;

    const min<int> e = monoid_traits<min<int> >::empty();

    // empty is the largest representable value (never wins a min).
    ok = ok && (e.value == (std::numeric_limits<int>::max)());

    // combine keeps the smaller operand.
    ok = ok && (mappend(min<int>(3), min<int>(5)).value == 3);
    ok = ok && (mappend(min<int>(5), min<int>(3)).value == 3);
    ok = ok && (mappend(min<int>(-2), min<int>(4)).value == -2);

    // a tie resolves to the (equal) value.
    ok = ok && (mappend(min<int>(7), min<int>(7)).value == 7);

    // identity: min(x, +inf) == x == min(+inf, x).
    ok = ok && (mappend(min<int>(9), e).value == 9);
    ok = ok && (mappend(e, min<int>(9)).value == 9);
    ok = ok && (mappend(min<int>(-100), e).value == -100);

    // identity edge: operand already equal to the identity.
    ok = ok && (mappend(e, e).value == (std::numeric_limits<int>::max)());

    // associativity.
    ok = ok && (mappend(mappend(min<int>(4), min<int>(1)), min<int>(9)).value ==
                mappend(min<int>(4), mappend(min<int>(1), min<int>(9))).value);

    // floating element type.
    ok = ok && close_enough(
                   mappend(min<double>(1.5), min<double>(0.5)).value, 0.5);

    // constexpr.
    static_assert(mappend(min<int>(3), min<int>(5)).value == 3, "min ce");
    static_assert(mempty<min<int> >().value ==
                  (std::numeric_limits<int>::max)(), "min empty ce");

    return ok;
}


/*
tests_max_instance
  Exercises the max<T> instance (maximum, identity numeric_limits::lowest()).
  Tests the following:
  - combine keeps the larger operand
  - empty is the smallest representable value
  - left / right identity laws hold, including the operand-equals-identity edge
  - a tie resolves to a stable value
  - associativity holds
  - negative operands and a floating element type
  - the algebra is a constant expression
*/
bool
tests_max_instance()
{
    bool ok = true;

    const max<int> e = monoid_traits<max<int> >::empty();

    // empty is the smallest representable value (never wins a max).
    ok = ok && (e.value == (std::numeric_limits<int>::lowest)());

    // combine keeps the larger operand.
    ok = ok && (mappend(max<int>(3), max<int>(5)).value == 5);
    ok = ok && (mappend(max<int>(5), max<int>(3)).value == 5);
    ok = ok && (mappend(max<int>(-2), max<int>(-9)).value == -2);

    // a tie.
    ok = ok && (mappend(max<int>(7), max<int>(7)).value == 7);

    // identity: max(x, -inf) == x == max(-inf, x).
    ok = ok && (mappend(max<int>(9), e).value == 9);
    ok = ok && (mappend(e, max<int>(9)).value == 9);
    ok = ok && (mappend(max<int>(-100), e).value == -100);

    // identity edge.
    ok = ok && (mappend(e, e).value == (std::numeric_limits<int>::lowest)());

    // associativity.
    ok = ok && (mappend(mappend(max<int>(4), max<int>(1)), max<int>(9)).value ==
                mappend(max<int>(4), mappend(max<int>(1), max<int>(9))).value);

    // floating element type.
    ok = ok && close_enough(
                   mappend(max<double>(1.5), max<double>(0.5)).value, 1.5);

    // constexpr.
    static_assert(mappend(max<int>(3), max<int>(5)).value == 5, "max ce");
    static_assert(mempty<max<int> >().value ==
                  (std::numeric_limits<int>::lowest)(), "max empty ce");

    return ok;
}


/*
tests_instance_is_specialized
  Confirms the is_specialized markers on both trait families for every instance,
  and that the marker is exactly std::true_type.
  Tests the following:
  - semigroup_traits<T>::is_specialized for all instances
  - monoid_traits<T>::is_specialized for all instances
  - the marker's exact type
*/
bool
tests_instance_is_specialized()
{
    // semigroup_traits<T>::is_specialized for every instance.
    static_assert(semigroup_traits<std::string>::is_specialized::value,
                  "string sg specialized");
    static_assert(semigroup_traits<std::vector<int> >::is_specialized::value,
                  "vector sg specialized");
    static_assert(semigroup_traits<sum<int> >::is_specialized::value,
                  "sum sg specialized");
    static_assert(semigroup_traits<product<int> >::is_specialized::value,
                  "product sg specialized");
    static_assert(semigroup_traits<all>::is_specialized::value,
                  "all sg specialized");
    static_assert(semigroup_traits<any>::is_specialized::value,
                  "any sg specialized");
    static_assert(semigroup_traits<min<int> >::is_specialized::value,
                  "min sg specialized");
    static_assert(semigroup_traits<max<int> >::is_specialized::value,
                  "max sg specialized");

    // monoid_traits<T>::is_specialized for every instance.
    static_assert(monoid_traits<std::string>::is_specialized::value,
                  "string mn specialized");
    static_assert(monoid_traits<std::vector<int> >::is_specialized::value,
                  "vector mn specialized");
    static_assert(monoid_traits<sum<int> >::is_specialized::value,
                  "sum mn specialized");
    static_assert(monoid_traits<product<int> >::is_specialized::value,
                  "product mn specialized");
    static_assert(monoid_traits<all>::is_specialized::value,
                  "all mn specialized");
    static_assert(monoid_traits<any>::is_specialized::value,
                  "any mn specialized");
    static_assert(monoid_traits<min<int> >::is_specialized::value,
                  "min mn specialized");
    static_assert(monoid_traits<max<int> >::is_specialized::value,
                  "max mn specialized");

    // the marker is exactly std::true_type.
    static_assert(
        std::is_same<monoid_traits<sum<int> >::is_specialized,
                     std::true_type>::value,
        "marker is std::true_type");

    return true;
}


NS_END  // testing
NS_END  // djinterp
