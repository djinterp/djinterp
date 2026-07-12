// djinterp [test]  profunctor_tests_arrow.cpp
//   Section I -- profn<F>, the arrow wrapper, and make_profn.

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "profunctor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_profn_construct
  A profn wraps a callable and exposes it as the fn member.
  Tests the following:
  - direct construction stores the callable
  - construction through make_profn yields a working arrow
*/
bool
tests_profn_construct()
{
    bool ok = true;

    profn<doubler> p{ doubler{} };
    ok = ok && (p.fn(5) == 10);          // the stored callable is the doubler

    auto q = make_profn(add_one{});
    ok = ok && (q(4) == 5);

    return ok;
}


/*
tests_profn_call
  operator() forwards to the wrapped callable, so a profn is used exactly like
  the function it wraps.
  Tests the following:
  - the arrow computes what the underlying callable would
*/
bool
tests_profn_call()
{
    bool ok = true;

    auto len = make_profn([](const std::string& _s){ return (int)_s.size(); });
    ok = ok && (len(std::string("hello")) == 5);

    auto neg = make_profn([](int _x){ return -_x; });
    ok = ok && (neg(7) == -7);

    ok = ok && (make_profn(doubler{})(21) == 42);

    return ok;
}


/*
tests_make_profn_decay
  make_profn deduces and DECAYS the callable type, stripping references and
  cv-qualifiers.
  Tests the following:
  - lvalue, const lvalue, rvalue, and xvalue operands all yield profn<F>
*/
bool
tests_make_profn_decay()
{
    doubler       d;
    const doubler cd{};

    static_assert(std::is_same<decltype(make_profn(d)),
                               profn<doubler> >::value, "lvalue decays");
    static_assert(std::is_same<decltype(make_profn(cd)),
                               profn<doubler> >::value, "const lvalue decays");
    static_assert(std::is_same<decltype(make_profn(doubler{})),
                               profn<doubler> >::value, "rvalue decays");
    static_assert(std::is_same<decltype(make_profn(std::move(d))),
                               profn<doubler> >::value, "xvalue decays");

    return true;
}


/*
tests_profn_forwarding
  operator() perfectly forwards its argument, preserving its reference category.
  Tests the following:
  - lvalue, const lvalue, and rvalue arguments each select the matching overload
*/
bool
tests_profn_forwarding()
{
    bool ok = true;

    auto p = make_profn(fwd_probe{});

    int       x  = 0;
    const int cx = 0;

    ok = ok && (p(x) == 1);              // lvalue
    ok = ok && (p(cx) == 3);             // const lvalue
    ok = ok && (p(std::move(x)) == 2);   // xvalue -> rvalue overload
    ok = ok && (p(5) == 2);              // prvalue -> rvalue overload

    return ok;
}


/*
tests_profn_constexpr
  profn's constructor and call operator are usable in a constant expression.
  Tests the following:
  - a make_profn call folds at compile time
  - a constexpr-constructed arrow is callable at compile time
*/
bool
tests_profn_constexpr()
{
    static_assert(make_profn(doubler{})(21) == 42, "constexpr call");

    constexpr profn<add_one> p{ add_one{} };
    static_assert(p(9) == 10, "constexpr member call");

    return true;
}


/*
tests_profn_return_type
  The call operator's result type is exactly the wrapped callable's return type.
  Tests the following:
  - arrows returning int, std::string, and double report those result types
*/
bool
tests_profn_return_type()
{
    auto pi = make_profn([](int _x){ return _x * 2; });
    auto ps = make_profn([](int _n){ return std::string(_n, 'a'); });
    auto pd = make_profn([](int _x){ return static_cast<double>(_x) / 2.0; });

    static_assert(std::is_same<decltype(pi(1)), int>::value, "int result");
    static_assert(std::is_same<decltype(ps(1)), std::string>::value,
                  "string result");
    static_assert(std::is_same<decltype(pd(1)), double>::value, "double result");

    (void)pi;
    (void)ps;
    (void)pd;
    return true;
}


NS_END  // testing
NS_END  // djinterp
