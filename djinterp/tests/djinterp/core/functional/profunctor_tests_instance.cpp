// djinterp [test]  profunctor_tests_instance.cpp
//   Section IV -- the profn<F> instance (and generic dispatch to a second one).

// std
#include <type_traits>
// djinterp
#include "profunctor_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_instance_composition_order
  The profn instance's dimap builds exactly post . fn . pre.
  Tests the following:
  - the result matches that specific order, with ops chosen so any other order
    would give a different number
*/
bool
tests_instance_composition_order()
{
    bool ok = true;

    auto p = make_profn(doubler{});
    auto d = dimap(p, add_one{}, add_ten{});   // ((x + 1) * 2) + 10

    ok = ok && (d(5) == 22);
    ok = ok && (d(1) == 14);
    // a wrong order such as pre . fn . post would give ((5+10)*2)+1 = 31.
    ok = ok && (d(5) != 31);

    return ok;
}


/*
tests_instance_returns_profn
  dimap composes a NEW arrow without unwrapping -- the result is again a profn.
  Tests the following:
  - dimap / lmap / rmap each return a profn
  - that profn is a live, callable profunctor
*/
bool
tests_instance_returns_profn()
{
    bool ok = true;

    auto p = make_profn(doubler{});
    auto d = dimap(p, add_one{}, add_ten{});

    static_assert(is_profn_type<decltype(d)>::value, "dimap returns a profn");
    static_assert(is_profn_type<decltype(lmap(p, add_one{}))>::value,
                  "lmap returns a profn");
    static_assert(is_profn_type<decltype(rmap(p, add_ten{}))>::value,
                  "rmap returns a profn");

    ok = ok && (is_profunctor<decltype(d)>::value);
    ok = ok && (d(5) == 22);

    return ok;
}


/*
tests_instance_composable
  A dimapped arrow is itself a profunctor, so the operations chain.
  Tests the following:
  - dimap of a dimap composes as expected
  - lmap / rmap over a dimapped arrow agree with the nested dimap
*/
bool
tests_instance_composable()
{
    bool ok = true;

    auto p  = make_profn(doubler{});
    auto d1 = dimap(p, add_one{}, add_ten{});       // ((x + 1) * 2) + 10
    auto d2 = dimap(d1, add_one{}, add_ten{});      // add_ten(d1(add_one(x)))

    ok = ok && (d2(5) == 34);                       // add_ten(d1(6)) = add_ten(24)

    auto chained = rmap(lmap(d1, add_one{}), add_ten{});
    ok = ok && (chained(5) == 34);                  // same composition

    ok = ok && (is_profunctor<decltype(d2)>::value);

    return ok;
}


/*
tests_instance_preserves_original
  dimap builds a fresh arrow and leaves the original untouched (a profn holds
  its callable by value).
  Tests the following:
  - the source arrow still computes the same result after being adapted
  - it remains usable in further operations
*/
bool
tests_instance_preserves_original()
{
    bool ok = true;

    auto p = make_profn(doubler{});
    ok = ok && (p(5) == 10);

    auto d = dimap(p, add_one{}, add_ten{});
    (void)d;
    ok = ok && (p(5) == 10);            // unchanged

    auto r = rmap(p, add_ten{});
    ok = ok && (r(5) == 20);
    ok = ok && (p(5) == 10);            // still unchanged

    return ok;
}


/*
tests_instance_is_specialized
  The instance carries the marker, through the explicit <T, void> form.
  Tests the following:
  - profunctor_traits<profn<F>>::is_specialized is std::true_type, for several F
*/
bool
tests_instance_is_specialized()
{
    static_assert(
        std::is_same<profunctor_traits<profn<doubler> >::is_specialized,
                     std::true_type>::value, "doubler arrow");
    static_assert(
        std::is_same<profunctor_traits<profn<add_one> >::is_specialized,
                     std::true_type>::value, "add_one arrow");

    auto p = make_profn([](int _x){ return _x; });
    static_assert(
        std::is_same<profunctor_traits<decltype(p)>::is_specialized,
                     std::true_type>::value, "lambda arrow");
    (void)p;

    return true;
}


/*
tests_instance_generic_dispatch
  The generic operations dispatch through profunctor_traits, so a SECOND,
  unrelated instance (pf_arrow) works identically -- and its results are its own
  carrier, not profn.
  Tests the following:
  - dimap / lmap / rmap over pf_arrow compute the same as over profn
  - the results are pf_arrows (genuinely dispatched), and still profunctors
  - the identity law holds for the second instance
*/
bool
tests_instance_generic_dispatch()
{
    bool ok = true;

    auto q = make_pf_arrow(doubler{});

    auto qd = dimap(q, add_one{}, add_ten{});
    ok = ok && (qd(5) == 22);                       // ((5 + 1) * 2) + 10

    auto ql = lmap(q, add_one{});
    ok = ok && (ql(5) == 12);                       // (5 + 1) * 2

    auto qr = rmap(q, add_ten{});
    ok = ok && (qr(5) == 20);                       // (5 * 2) + 10

    ok = ok && (is_profunctor<decltype(qd)>::value);
    ok = ok && (!is_profn_type<decltype(qd)>::value);   // dispatched to pf_arrow

    auto idf = [](int _x){ return _x; };
    auto qi = dimap(q, idf, idf);
    ok = ok && (qi(7) == q(7));                     // identity law

    return ok;
}


NS_END  // testing
NS_END  // djinterp
