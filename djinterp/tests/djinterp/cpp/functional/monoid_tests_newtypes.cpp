/*******************************************************************************
* djinterp [cpp]                                       monoid_tests_newtypes.cpp
*
*
* path:      /tests/djinterp/cpp/functional/monoid_tests_newtypes.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  monoid_tests_newtypes.cpp
//   Section I -- MONOID NEWTYPES (sum / product / all / any / min / max).

// std
#include <type_traits>
// djinterp
#include "monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_sum_newtype
  Exercises the additive newtype sum<T>.
  Tests the following:
  - default construction seeds the additive identity 0
  - the explicit value ctor stores its argument verbatim
  - the wrapped `value` is a public, mutable member
  - the same behaviour over a floating element type
  - the value ctor is explicit (no implicit T -> sum<T>)
  - construction is usable in a constant expression
*/
bool tests_sum_newtype()
{
    bool ok = true;

    // default constructs to the additive identity, 0.
    DSum<int> a;
    ok = ok && (a.value == 0);

    // explicit ctor stores the value verbatim.
    DSum<int> b(7);
    ok = ok && (b.value == 7);

    // the wrapped value is a public, mutable member.
    b.value = -3;
    ok = ok && (b.value == -3);

    // works over a floating element type; default is 0.0.
    DSum<double> c;
    ok = ok && close_enough(c.value, 0.0);

    DSum<double> d(2.5);
    ok = ok && close_enough(d.value, 2.5);

    // the value ctor is explicit.
    ok = ok && (std::is_constructible<DSum<int>, int>::value);
    ok = ok && (!std::is_convertible<int, DSum<int> >::value);

    // constexpr-constructible.
    static_assert(DSum<int>(4).value == 4, "sum<int> explicit ctor is constexpr");
    static_assert(DSum<int>().value  == 0, "sum<int> default is constexpr 0");

    return ok;
}


/*
tests_product_newtype
  Exercises the multiplicative newtype product<T>.
  Tests the following:
  - default construction seeds the multiplicative identity 1 (NOT 0)
  - the explicit value ctor stores its argument
  - the wrapped `value` is public and mutable
  - the same over a floating element type (default 1.0)
  - the value ctor is explicit
  - construction is usable in a constant expression
*/
bool tests_product_newtype()
{
    bool ok = true;

    // default constructs to the multiplicative identity, 1 -- the defining
    // difference from sum<T>.
    DProduct<int> a;
    ok = ok && (a.value == 1);

    // explicit ctor stores the value.
    DProduct<int> b(6);
    ok = ok && (b.value == 6);

    // public, mutable value.
    b.value = 9;
    ok = ok && (b.value == 9);

    // floating element type; default is 1.0.
    DProduct<double> c;
    ok = ok && close_enough(c.value, 1.0);

    DProduct<double> d(3.5);
    ok = ok && close_enough(d.value, 3.5);

    // explicit value ctor.
    ok = ok && (std::is_constructible<DProduct<int>, int>::value);
    ok = ok && (!std::is_convertible<int, DProduct<int> >::value);

    // constexpr.
    static_assert(DProduct<int>(4).value == 4, "product explicit ctor constexpr");
    static_assert(DProduct<int>().value  == 1, "product default is constexpr 1");

    return ok;
}


/*
tests_all_newtype
  Exercises the conjunctive newtype all.
  Tests the following:
  - default construction seeds the conjunctive identity, true
  - the explicit ctor stores true / false
  - the wrapped `value` is a public, mutable bool
  - the ctor from bool is explicit
  - construction is usable in a constant expression
*/
bool tests_all_newtype()
{
    bool ok = true;

    // default is the conjunctive identity, true.
    DAll a;
    ok = ok && (a.value == true);

    // explicit ctor stores the flag.
    DAll t(true);
    DAll f(false);
    ok = ok && (t.value == true);
    ok = ok && (f.value == false);

    // public, mutable bool.
    f.value = true;
    ok = ok && (f.value == true);

    // explicit ctor from bool.
    ok = ok && (std::is_constructible<DAll, bool>::value);
    ok = ok && (!std::is_convertible<bool, DAll>::value);

    // constexpr.
    static_assert(DAll().value       == true,  "all default is constexpr true");
    static_assert(DAll(false).value  == false, "all explicit ctor is constexpr");

    return ok;
}


/*
tests_any_newtype
  Exercises the disjunctive newtype any.
  Tests the following:
  - default construction seeds the disjunctive identity, false
  - the explicit ctor stores true / false
  - the wrapped `value` is a public, mutable bool
  - the ctor from bool is explicit
  - construction is usable in a constant expression
*/
bool tests_any_newtype()
{
    bool ok = true;

    // default is the disjunctive identity, false.
    DAny a;
    ok = ok && (a.value == false);

    // explicit ctor stores the flag.
    DAny t(true);
    DAny f(false);
    ok = ok && (t.value == true);
    ok = ok && (f.value == false);

    // public, mutable bool.
    t.value = false;
    ok = ok && (t.value == false);

    // explicit ctor from bool.
    ok = ok && (std::is_constructible<DAny, bool>::value);
    ok = ok && (!std::is_convertible<bool, DAny>::value);

    // constexpr.
    static_assert(DAny().value      == false, "any default is constexpr false");
    static_assert(DAny(true).value  == true,  "any explicit ctor is constexpr");

    return ok;
}


/*
tests_min_newtype
  Exercises the minimum newtype min<T>.
  Tests the following:
  - the explicit value ctor stores its argument
  - the wrapped `value` is public and mutable
  - min<T> has NO default ctor (the explicit ctor suppresses it)
  - the value ctor is explicit
  - the same over a floating element type
  - construction is usable in a constant expression
*/
bool tests_min_newtype()
{
    bool ok = true;

    // explicit ctor stores the value.
    DMin<int> a(5);
    ok = ok && (a.value == 5);

    // public, mutable value.
    a.value = -1;
    ok = ok && (a.value == -1);

    // floating element type.
    DMin<double> b(2.5);
    ok = ok && close_enough(b.value, 2.5);

    // NO default ctor -- min<T> is value-only.
    ok = ok && (!std::is_default_constructible<DMin<int> >::value);

    // explicit value ctor.
    ok = ok && (std::is_constructible<DMin<int>, int>::value);
    ok = ok && (!std::is_convertible<int, DMin<int> >::value);

    // constexpr.
    static_assert(DMin<int>(5).value == 5, "min explicit ctor is constexpr");

    return ok;
}


/*
tests_max_newtype
  Exercises the maximum newtype max<T>.
  Tests the following:
  - the explicit value ctor stores its argument
  - the wrapped `value` is public and mutable
  - max<T> has NO default ctor
  - the value ctor is explicit
  - the same over a floating element type
  - construction is usable in a constant expression
*/
bool tests_max_newtype()
{
    bool ok = true;

    // explicit ctor stores the value.
    DMax<int> a(9);
    ok = ok && (a.value == 9);

    // public, mutable value.
    a.value = 42;
    ok = ok && (a.value == 42);

    // floating element type.
    DMax<double> b(3.5);
    ok = ok && close_enough(b.value, 3.5);

    // NO default ctor.
    ok = ok && (!std::is_default_constructible<DMax<int> >::value);

    // explicit value ctor.
    ok = ok && (std::is_constructible<DMax<int>, int>::value);
    ok = ok && (!std::is_convertible<int, DMax<int> >::value);

    // constexpr.
    static_assert(DMax<int>(9).value == 9, "max explicit ctor is constexpr");

    return ok;
}


/*
tests_newtype_default_constructibility
  Contrasts the default-constructibility of the six newtypes.
  Tests the following:
  - sum / product / all / any ARE default-constructible (identity-seeding ctor)
  - min / max are NOT (their explicit ctor suppresses the implicit default)
  - min / max value ctors are explicit
*/
bool tests_newtype_default_constructibility()
{
    bool ok = true;

    // identity-seeding default ctors.
    ok = ok && (std::is_default_constructible<DSum<int> >::value);
    ok = ok && (std::is_default_constructible<DProduct<int> >::value);
    ok = ok && (std::is_default_constructible<DAll>::value);
    ok = ok && (std::is_default_constructible<DAny>::value);

    // value-only newtypes: no default ctor.
    ok = ok && (!std::is_default_constructible<DMin<int> >::value);
    ok = ok && (!std::is_default_constructible<DMax<int> >::value);

    // their value ctors are explicit.
    ok = ok && (std::is_constructible<DMin<int>, int>::value);
    ok = ok && (!std::is_convertible<int, DMin<int> >::value);
    ok = ok && (std::is_constructible<DMax<int>, int>::value);
    ok = ok && (!std::is_convertible<int, DMax<int> >::value);

    return ok;
}


/*
tests_newtype_constexpr_construction
  Confirms every newtype is constructible in a constant expression, with the
  correct identity default where one exists.
  Tests the following:
  - explicit and (where present) default ctors of all six newtypes are constexpr
  - the default values are the intended monoid identities
*/
bool tests_newtype_constexpr_construction()
{
    // every newtype is usable in a constant expression.
    static_assert(DSum<int>(3).value      == 3,     "sum ce");
    static_assert(DSum<int>().value       == 0,     "sum default ce");
    static_assert(DProduct<int>(4).value  == 4,     "product ce");
    static_assert(DProduct<int>().value   == 1,     "product default ce");
    static_assert(DAll(false).value       == false, "all ce");
    static_assert(DAll().value            == true,  "all default ce");
    static_assert(DAny(true).value        == true,  "any ce");
    static_assert(DAny().value            == false, "any default ce");
    static_assert(DMin<int>(5).value      == 5,     "min ce");
    static_assert(DMax<int>(9).value      == 9,     "max ce");

    return true;
}


NS_END  // testing
NS_END  // djinterp
